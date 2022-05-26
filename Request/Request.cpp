#include "Request.h"


Lock* Request::mLock = new Lock(); 


void OnSIGSEGV(int signum, siginfo_t *info, void *ptr)
{
    abort();

}


int Request::epollFD = 0;


Request::Request(){

}

Request::~Request(){

}


void Request::EpollMod(int listen){
    epoll_event event;  //建立事件
    event.data.fd=fd;   //修改文件描述符
    //事件分别为可写(读)、错误、关闭、半关、只被一个线程使用、边缘触发
    event.events = listen | EPOLLERR| EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT |EPOLLET;
    epoll_ctl(Request::epollFD,EPOLL_CTL_MOD,fd,&event);//这次是修改文件描述符表
}


void Request::Init(int acceptFD,sockaddr_in clientAddress){
    fd=acceptFD;    //当前请求的文件描述符
    cAddress=clientAddress; //当前请求的客户地址

    //设置端口复用
    int optVal=1; 
    setsockopt(acceptFD,SOL_SOCKET,SO_REUSEADDR,&optVal,sizeof(optVal));

    //设置非阻塞
    int oldFL = fcntl(fd,F_GETFL);
    int newFL = oldFL | O_NONBLOCK;
    fcntl(fd,F_SETFL,newFL);


    //对epoll添加该文件描述符
    epoll_event event;
    event.data.fd=fd;
    //事件分别为可读、错误、关闭、半关、只被一个线程使用、边缘触发
    event.events = EPOLLIN | EPOLLERR| EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT |EPOLLET;
    epoll_ctl(Request::epollFD,EPOLL_CTL_ADD,fd,&event);//对epoll添加文件描述符

    //一些初始化
    index = 0;  //当前缓冲区读到的索引值置0
}

void Request::Close(){
    epoll_ctl(Request::epollFD,EPOLL_CTL_DEL,fd,0);   //epoll删除
    close(fd);                          //关闭fd
    index=0;        //索引置0
    parseIndex=0;   //清空解析索引值
}


bool Request::Read(){
    if(index>REQUEST_BUFF_SIZE){
        return false;
    }//请求数据超过缓冲区删除数据

    int retRecv = 0;
    while(true){    
        retRecv = recv(fd,buff+index,REQUEST_BUFF_SIZE-index,MSG_DONTWAIT);//非阻塞读
        if(retRecv == -1){
            if(errno == EAGAIN | errno == EWOULDBLOCK ){    //错误号为再次读取或者可能阻塞
                break;  //这种说明是数据读完了                                  
            }

            return false;  //如果不满足上面的错误号说明读取错误 关闭连接
        }

        if(retRecv == 0){   //说明对方关闭连接 直接返回错误我们也关闭连接
            return false;
        }

        //说明读取成功 索引加上成功读取的字节
        index += retRecv;
    }
    
    return true;
}

bool Request::Write(){
    //往Socket里面写数据
    if((index-writeIndex)==0){   //无需写入数据
        Close();    //因为是非持久连接 直接关闭即可
        return true;
    }

    while(true){        //分散写数据
        int retSend=send(fd,buff+writeIndex,index-writeIndex,0);
        if(retSend<=-1){
            if( errno == EAGAIN || errno == EWOULDBLOCK) {
                EpollMod(EPOLLOUT); //说明缓冲区不够了 等会继续来读
                return true;
            }
            return false; //如果不是缓冲区不够说明是其他错误 直接关闭连接
        }

        if(retSend==0){ //对方关闭连接
            return false;  //直接关闭连接即可
        }

        writeIndex+=retSend;
        break;
    }
    
    //写完了
    Close();
    return true;
}

bool Request::CorrectlyURL(string& temp){
    return true;            //判断URL日后再写
}

    
PARSE Request::GetRequestLine(){
    string temp;    //temp表示当前正在读取的字符缓冲

    //通过循环找到第一个空格
    for(;parseIndex<index && buff[parseIndex]!=' ';++parseIndex){
        temp.push_back(buff[parseIndex]);
    }

    if(parseIndex==index){   //到达末尾 说明HTTP请求有误
        return WRONG;
    }

    //这边开始判断方法
    if(temp=="GET" || temp =="POST" ){
        Method=temp;
    }else{
        return WRONG;
    }
    //这边如果方法不对 就G    

    //获取URL地址
    ++parseIndex;
    temp.clear();
    for(;parseIndex<index && buff[parseIndex]!=' ';++parseIndex){
        temp.push_back(buff[parseIndex]);
    }
    if(parseIndex==index){   //到达末尾 说明HTTP请求有误
        return WRONG;
    }
    if(CorrectlyURL(temp)){
        URL=temp;
    }else{
        return WRONG;
    }

    //判断HTTP版本
    ++parseIndex;
    temp.clear();
    for(;parseIndex<index && buff[parseIndex]!='\r';++parseIndex){
        temp.push_back(buff[parseIndex]);
    }

    if(++parseIndex<index && buff[parseIndex]=='\n' && temp=="HTTP/1.1"){
        ++parseIndex;//换成Header解析 暂时只支持1.1
        Version = temp;
    }else{
        return WRONG;
    }

    return NEXT;    //代表请求行解析成功
}



PARSE Request::GetHEADER(){
    string key;    //temp表示当前正在读取的字符缓冲
    string value;
    while(true){
        if(parseIndex+1>=index){
            return WRONG;
        }
        if((buff[parseIndex]=='\r' && buff[parseIndex+1]=='\n')){
            parseIndex+=2;  //准备读取Body
            return NEXT;
        }

        key.clear();
        value.clear();
        //通过循环找到第一个:读取key
        for(;parseIndex<index && buff[parseIndex]!=':';++parseIndex){
            key.push_back(buff[parseIndex]);
        }
        parseIndex+=2;//跳过空格
        for(;parseIndex<index && buff[parseIndex]!='\r';++parseIndex){
            value.push_back(buff[parseIndex]);
        }
        if(++parseIndex<index && buff[parseIndex]=='\n'){//说明准备换行
            ++parseIndex;//换行
            if(key=="Host"){    //Host:
                Host=value;
                continue;
            }
            if(key=="Connection"){//Connection:
                Connection=value;
                continue;
            }
        }else{
            return WRONG;
        }
    }

    return WRONG;    //意外跳出
}


PARSE Request::GetMessageBody(){
    string body;
    for(;parseIndex<index;++parseIndex){
        body.push_back(buff[parseIndex]);
    }
    Body=body;
    return NEXT;
}


RESULT Request::RequestParse(){
    //当前状态为读取请求行
    PARSE state = REQUEST_LINE;
    
    parseIndex = 0; //当前解析的index设置为0

    //通过有限状态机写入数据
    while(state != WRONG && state!= FINISHED){
        switch (state)
        {
        case REQUEST_LINE://解析首行
            if(GetRequestLine()==NEXT){
                state=HEADER;
            }else{
                state=WRONG;
            }
            break;
        case HEADER://解析请求头
            if(GetHEADER()==NEXT){
                state=MESSAGE_BODY;
            }else{
                state=WRONG;
            }
            break;
        case MESSAGE_BODY://解析请求头
            if(GetMessageBody()==NEXT){
                state=FINISHED;
            }else{
                state=WRONG;
            }
            break;
        default:
            state=WRONG;
            break;
        }
    }

    if(state == WRONG){
        return UNFINISHED;
    }//解析错误

    if(state == FINISHED){
        return COMPLETE;//解析成功
    }

    return UNFINISHED;  //未知情况 返回失败
}


//在这写上返回的错误码 方便参考


//通过这个函数返回错误码
int Request::HttpResponseCode(){
    struct stat statBuff;   //存储想获取的文件属性
    URL = WEB_ROOT_DIR+ URL;    //根据用户获取用户请求的文件地址
    int retStat=stat(URL.c_str(),&statBuff);   //获取文件属性
    
    if(retStat == -1 ){     //说明未找到该文件
        return 404;       //404 NOFOUND
    }

    if(!S_ISREG(statBuff.st_mode)){ //判断想获取的是否是普通文件
        return 400;     //400 请求错误
    }

    if(! (statBuff.st_mode & S_IROTH) ){    //判断文件是否有权限获取
        return 403;     //403 没有权限获取
    }

    //打开文件

    int fileFD = open(URL.c_str(), O_RDONLY);
    //创建私有仅可读内存映射        
    if(fileFD >0 ){//文件描述符判断
        memoryMapAddress=(char*)mmap(NULL,statBuff.st_size,PROT_READ,MAP_PRIVATE,fileFD,0);
        memoryMapSize=statBuff.st_size; //记录文件大小
        close(fileFD);
        return 200;     //允许获取
    }else{
        close(fileFD);
        return 404;
    }
}


void Request::HttpResponseWrite_ALL(string& temp,int code){
    HttpResponseWrite_RequestLine(temp,code);
    HttpResponseWrite_HEADER(temp,code);
    HttpResponseWrite_MessageBody(temp,code);
}

void Request::HttpResponseWrite_RequestLine(string& temp,int code){
    //版本号
    temp+=Version;
    temp+=" ";

    switch (code)
    {
    case 200:  //正常返回
        temp+="200 OK";
        break;
    case 400:           //请求错误
        temp+="400 Bad Request";
        break;
    case 403:           //请求没有权限
        temp+="403 Forbidden";
        break;
    case 404:           //notfound
        temp+="404 Not Found";
        break;
    default:
        break;
    }

    temp+="\r\n";
}
void Request::HttpResponseWrite_HEADER(string& temp,int code){
    temp+="Connection: close\r\n";      //采用非持久连接
    temp+="Content-Length: ";

    
    if(code==200){      //只有正常才有body
        temp.append(std::to_string(memoryMapSize));//加入消息长度 
    }else{
        temp.append("0");//消息长度为0
    }
    temp+="\r\n";       //换行
    temp+="\r\n";       //换行
}
void Request::HttpResponseWrite_MessageBody(string& temp,int code){
    if(code==200){      //只有正常才有body
        temp+=memoryMapAddress;     //加入内存中的文件 加入完成了以后就可以解除内存映射了
        int munmapret=munmap(memoryMapAddress,memoryMapSize);//解除内存映射
    }

}




RESULT Request::RequestWrite(){
    
    
    int Repose = HttpResponseCode();    //获取返回的错误码
    
    string httpRepose;
    index=0;    //清空buff 准备往缓冲区里面写数据
    
    switch (Repose)
    {
    case 200:           //正常
        HttpResponseWrite_ALL(httpRepose,200);
        break;
    case 400:           //请求错误
        HttpResponseWrite_ALL(httpRepose,400);
        break;
    case 403:           //请求没有权限
        HttpResponseWrite_ALL(httpRepose,403);
        break;
    case 404:           //notfound
        HttpResponseWrite_ALL(httpRepose,404);
        break;
    default:
        return UNFINISHED;
    }
    
    //再将httpRepose写到buff中
    for(index=0;index<httpRepose.size();++index){
        buff[index]=httpRepose[index];
    }
    buff[++index]='0';//标志在buff中写完
    return COMPLETE;        //完成
    
}



//线程做任务
void Request::Process(){
    mLock->MutexLock();///////////////////////此处有string的大坑
    
    //从buff中解析HTTP请求 获取请求结果
    int retParse = RequestParse();
    if(retParse != COMPLETE){
        Close();
    }

    
    //往buff里面写HTTP响应

    int retWrite = RequestWrite();
    


    if(retWrite != COMPLETE){
        Close();
    }
   
    EpollMod(EPOLLOUT); //等待Epoll可写
    writeIndex=0;   //初始化已经写的Index


    mLock->MutexUnlock();/////////////////////此处有string的大坑
    return; //做完任务了
}
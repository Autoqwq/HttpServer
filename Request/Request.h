#ifndef REQUEST_H
#define REQUEST_H

#include "../Exception/Exception.h"
#include "../Lock/Lock.h"


#include <iostream>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <string>
#include <netinet/ip.h>
#include <errno.h>
#include <sys/stat.h>
#include <map>
#include <sys/mman.h>




using std::map;
using std::string;

#define REQUEST_BUFF_SIZE 2048  //每个HTTP请求的缓冲区
#define WEB_ROOT_DIR "/home/qwq/Server/Resources"   //存放的网站目录


enum METHOD {GET,POST}; //请求的方法
enum PARSE {REQUEST_LINE,HEADER,MESSAGE_BODY,WRONG,NEXT,FINISHED};  //当前正在读取什么请求
enum RESULT {COMPLETE,UNFINISHED};  //请求的结果
//enum RESPONSE {REQUESTERROR,REQUESTSUCCESS};    //HTTP的响应 本来再写个enum的 但是好像没用到....




//处理请求
class Request{
public:
    Request();
    ~Request();
    
    //初始化当前请求的一些值
    void Init(int acceptFD,sockaddr_in clientAddress);

    //关闭当前的连接
    void Close();

    bool Read();    //从系统缓冲区中读数据
    bool Write();   //在系统缓冲区中写数据




    //根据请求处理
    void Process();


    //修改请求类的epollFD
    inline static void changeEpollFD(int newFD){
        Request::epollFD=newFD;
    }
private:
    static int epollFD; //请求类共享的epoll文件描述符


    int fd; //当前请求的文件描述符
    sockaddr_in cAddress;   //当前请求的客户地址
    char buff[REQUEST_BUFF_SIZE];   //请求的缓冲区大小
    int index;            //当前缓冲区读到的索引值
    int parseIndex;        //解析http时的索引值
    int writeIndex;         //写Socket时的索引值

    char memoryBuff[2048];

    //一些HTTP的请求信息
    string Method;  //请求的方法
    string URL;     //请求的URL
    string Version; //请求的版本
    string Host;    //请求的Host名
    string Connection;  //是否持久连接
    string Body;    //请求的Body
    void EpollMod(int event);   //修改Epoll事件

    RESULT RequestParse();    //解析buff中的数据
    RESULT RequestWrite();    //往buff中写入数据

    PARSE GetRequestLine();     //解析http首行
    PARSE GetHEADER();          //解析http头
    PARSE GetMessageBody();     //解析Body
    bool CorrectlyURL(string& temp);        //判断URL是否正确

    int HttpResponseCode();     //通过这个函数返回Http响应码

    //通过这三个函数向响应文本中写东西
    void HttpResponseWrite_ALL(string& temp,int code);
    void HttpResponseWrite_RequestLine(string& temp,int code);
    void HttpResponseWrite_HEADER(string& temp,int code);
    void HttpResponseWrite_MessageBody(string& temp,int code);

    //内存映射地址
    char* memoryMapAddress;
    int memoryMapSize;

    static Lock *mLock;
};


#endif
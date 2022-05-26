#include "Exception.h"

Error::Error(std::string r){
    this->reason=reason;
}

Error::~Error(){

}

std::string Error::What(){
    return this->reason;
}
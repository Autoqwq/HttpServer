#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <exception>
#include <string>

class Error : public std::exception{
public:
    Error(std::string reason);
    ~Error();
    std::string What();
private:
    std::string reason;
    char buff[64];
};
#endif
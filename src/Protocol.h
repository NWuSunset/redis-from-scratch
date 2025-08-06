#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <vector>
#include <string>

class Protocol
{
private:
    /* data */
public:
    Protocol(/* args */);

    std::vector<std::string> parse(std::string);

    ~Protocol();
};

Protocol::Protocol(/* args */)
{
}

Protocol::~Protocol()
{
}



#endif
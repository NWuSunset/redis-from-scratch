#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <vector>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <functional>
#include <poll.h>
#include <iostream>

class Protocol
{
private:
    /* data */
public:
    Protocol(/* args */);

    std::vector<std::string> parse(std::string);
    void executeCommand(std::vector<std::string> command, pollfd client_fd);

    ~Protocol();
};





#endif
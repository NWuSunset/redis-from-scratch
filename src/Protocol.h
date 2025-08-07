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
    std::unordered_map<std::string, std::string> kv_store; //key value store for SET and GET
    std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, int)>> command_table;
    
public:
    Protocol(/* args */);

    //cmd handlers
    void handle_ping(const std::vector<std::string> & cmd, int fd);
    void handle_echo(const std::vector<std::string> & cmd, int fd);
    void handle_set(const std::vector<std::string> & cmd, int fd);
    void handle_get(const std::vector<std::string> & cmd, int fd);

    std::vector<std::string> parse(std::string);
    void executeCommand(std::vector<std::string> command, pollfd client_fd);

    ~Protocol();
};


#endif
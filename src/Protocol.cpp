#include "Protocol.h"

Protocol::Protocol()
{
}

//Command handlers:
void handle_ping(const std::vector<std::string> & cmd, int fd) {
  std::string response = "+PONG\r\n";
  send(fd, response.c_str(), response.size(), 0);
}

//respondes with the arg as a RESP encoded bulk string (ex: 'ECHO hey' --> server returns '$3\r\nhey\r\n')
void handle_echo(const std::vector<std::string>& cmd, int fd) {
  if (cmd.size() < 2) {
    std::string response = "-ERR wrong number of arguments for 'echo' command\r\n";
    send(fd, response.c_str(), response.size(), 0);
    return;
  }

  const std::string& arg = cmd[1];
  std::string result = "$" + std::to_string(arg.length()) + "\r\n" + arg + "\r\n";
  send(fd, result.c_str(), result.size(), 0);
}

//current list of function names (fix to work with parsed commands) 
std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, int)>> command_table = {
  {"PING", handle_ping}, 
  {"ECHO", handle_echo}
};


 //Takes bytes encoded by RESP from client and parses the bytes to extract the comamnds + args (note: client sends as array of bulk strings, each with the RESP encryption applied)
std::vector<std::string> Protocol::parse(std::string input) {
    std::vector<std::string> result; 

    //*2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n. 
    //["ECHO", "hey"] encoded with RESP


    size_t pos = 0; //the pos we are in the string

    if (input[pos] != '*') {
        return result; //if not passed in as an array we have problem
    }
    pos++; //skip the inital array *

    //find array len 
    size_t crlf = input.find("\r\n"); //crlf terminator '\r\n' for the RESP protocol
    if (crlf == std::string::npos) return result; //no arr length indicator
    int arrayLen = std::stoi(input.substr(pos, crlf - pos)); 
    pos = crlf + 2; //move onto $ sign before first bulk string

    for (int i = 0; i < arrayLen; i++) {
        if (input[pos] != '$') {
            return result;
        }
        pos++;
        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) return result; //no str length
        int stringLength = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2; //move to the bulk string

        //get the string and add it to the vecotr
        std::string arg = input.substr(pos, stringLength);
        result.push_back(arg);
        pos += stringLength + 2; //skip the current string and the crlf terminator.
    }
    return result;
}

void Protocol::executeCommand(std::vector<std::string> command, pollfd client_fd) {
    if (command.empty()) {
        std::string response = "-ERR empty command\r\n";
        send(client_fd.fd, response.c_str(), response.size(), 0);
        return;
    }

    auto it = command_table.find(command[0]); //find command string (PING, ECHO, etc.)
    if (it != command_table.end()) {
        it->second(command, client_fd.fd); //go to corresponding command function
    } else {
        std::string response = "-ERR unknown command\r\n";
        send(client_fd.fd, response.c_str(), response.size(), 0);
    } 
}

Protocol::~Protocol()
{
}
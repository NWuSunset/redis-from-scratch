#include "Protocol.h"

Protocol::Protocol()
{
    command_table = {
        {"PING", [this](const std::vector<std::string>& cmd, int fd) { handle_ping(cmd, fd); }},
        {"ECHO", [this](const std::vector<std::string>& cmd, int fd) { handle_echo(cmd, fd); }},
        {"SET",  [this](const std::vector<std::string>& cmd, int fd) { handle_set(cmd, fd); }},
        {"GET",  [this](const std::vector<std::string>& cmd, int fd) { handle_get(cmd, fd); }}
    };
}

//Command handlers:
void Protocol::handle_ping(const std::vector<std::string> & cmd, int fd) {
  std::string response = "+PONG\r\n";
  send(fd, response.c_str(), response.size(), 0);
}

//respondes with the arg as a RESP encoded bulk string (ex: 'ECHO hey' --> server returns '$3\r\nhey\r\n')
void Protocol::handle_echo(const std::vector<std::string>& cmd, int fd) {
  if (cmd.size() < 2) {
    std::string response = "-ERR wrong number of arguments for 'echo' command\r\n";
    send(fd, response.c_str(), response.size(), 0);
    return;
  }

 

  const std::string& arg = cmd[1];
  std::string response = "$" + std::to_string(arg.length()) + "\r\n" + arg + "\r\n";
  send(fd, response.c_str(), response.size(), 0);
}

//sets a key to a value

/* Note: when adding keys with experation times:

Redis keys are expired in two ways: a passive way and an active way.

A key is passively expired when a client tries to access it and the key is timed out.

However, this is not enough as there are expired keys that will never be accessed again. 
These keys should be expired anyway, so periodically, Redis tests a few keys at random amongst the set of keys with an expiration. 
All the keys that are already expired are deleted from the keyspace.

Active expiry will not be included at this time.

*/
void Protocol::handle_set(const std::vector<std::string>& cmd, int fd) {
    if (cmd.size() < 3) {
        std::string response = "-ERR wrong number of arguments for 'set' command\r\n";
        send(fd, response.c_str(), response.size(), 0);
        return;
    }

    std::string key = cmd[1];
    std::string value = cmd[2];

    if (cmd.size() >= 5 && cmd[3] == "PX") {
        int px_value = std::stoi(cmd[4]);
        //set expiry (set time + expiry time). When expiry happens the current time will be greater than this value 
        expiry_store[key] = std::chrono::steady_clock::now() + std::chrono::milliseconds(px_value);
    } 

    kv_store[key] = value; //link the key to the value
    std::string response = "+OK\r\n";
    send(fd, response.c_str(), response.size(), 0);
}

//get the referenced key
void Protocol::handle_get(const std::vector<std::string>& cmd, int fd) {
    if (cmd.size() < 2) {
         std::string response = "-ERR wrong number of arguments for 'set' command\r\n";
         send(fd, response.c_str(), response.size(), 0);
         return;
    }
    std::string key = cmd[1];
    auto exp_it = expiry_store.find(key);
    auto kv_it = kv_store.find(key);
    //If passed expiry (current time > time when key was set + expiry time)
    if (exp_it != expiry_store.end() && std::chrono::steady_clock::now() > exp_it->second) {
        kv_store.erase(kv_it);
        expiry_store.erase(exp_it);
    }

    
    if (kv_it != kv_store.end()) {
        std::string value = kv_it->second;
        std::string response = "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
        send(fd, response.c_str(), response.size(), 0);
    } else {
        std::string response = "$-1\r\n"; //return a null bulk string
        send(fd, response.c_str(), response.size(), 0); 
    }
}

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

    //redis is case-insensitive so convert to uppercase
    std::string cmd_upper = command[0];
    std::transform(cmd_upper.begin(), cmd_upper.end(), cmd_upper.begin(), ::toupper); //!! note that the entire command will become uppercase, check if this is a problem later?


    auto it = command_table.find(cmd_upper); //find command string (PING, ECHO, etc.)
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
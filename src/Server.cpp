#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <algorithm>
#include <poll.h>
#include <unordered_map>
#include <functional>

#include "Protocol.h"
/*
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif */




int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

//fd , file descrptor int representing open file (like client or server)
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::cout << "Waiting for a client to connect...\n";

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  
  //Implimenting multiple client support
  std::vector<pollfd> poll_fds; //file descriptors vector

  Protocol protocol;
  
  /*
  self note:
    struct pollfd {
    int   fd;        // File descriptor to poll
    short events;    // Events to look for (e.g., POLLIN)
    short revents;   // Events returned (set by poll())
}; */
  

  poll_fds.push_back({server_fd, POLLIN, 0});
  while (true) {
    //checking (polling) for new activity
    int activity = poll(poll_fds.data(), poll_fds.size(), -1);
    if (activity < 0) {
      std::cerr << "poll failed\n";
      break;
    }

    //if cleint connection
    if (poll_fds[0].revents & POLLIN) {
      int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
      poll_fds.push_back({client_fd, POLLIN, 0});
      std::cout << "Client connected\n";
    }

    //loop through file decriptors and find ones where clients sent data
    for (int i = 1; i < poll_fds.size(); i++) {
      char buffer[2048];
      if (poll_fds[i].revents & POLLIN) { 
        int bytes_read = read(poll_fds[i].fd, buffer, sizeof(buffer));

        if (bytes_read <= 0) {
         std::cerr << "failed to read bytes by user\n";
         close(poll_fds[i].fd);
         poll_fds.erase(poll_fds.begin() + i);
        } else {  
        //handle client commands
        std::string request(buffer);
        std::vector<std::string> command = protocol.parse(request); //note: impliment parse (will return a parsed command which is an array of bulk strings encoded with the redis protocol)
        protocol.executeCommand(command, poll_fds[i]);

        /*
        if (request.find("PING") != std::string::npos) { //note to self: if not find .find returns string::npos
          std::string response = "+PONG\r\n";
          send(poll_fds[i].fd, response.c_str(), response.size(), 0);
        }*/
       }
      } 
    }
  } 
  close(server_fd); 

  return 0;
}





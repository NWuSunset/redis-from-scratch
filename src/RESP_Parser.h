#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <vector>
#include <string>

class RESP_Parser {
    public:
     RESP_Parser();
     std::vector<std::string> parse(std::string);
};

#endif
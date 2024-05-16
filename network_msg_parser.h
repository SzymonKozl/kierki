//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_NETWORK_MSG_PARSER_H
#define KIERKI_NETWORK_MSG_PARSER_H

#include "vector"
#include "string"

using resp_array = std::vector<std::pair<std::string, std::string>>;

resp_array parse_msg(std::string m, bool server_side);

#endif //KIERKI_NETWORK_MSG_PARSER_H

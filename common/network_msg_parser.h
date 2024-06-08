//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_NETWORK_MSG_PARSER_H
#define KIERKI_NETWORK_MSG_PARSER_H

#include "vector"
#include "string"

using ParseResp = std::vector<std::pair<std::string, std::string>>;

ParseResp parseNetMsg(std::string m, bool sererSide);

#endif //KIERKI_NETWORK_MSG_PARSER_H

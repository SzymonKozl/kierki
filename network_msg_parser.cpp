//
// Created by szymon on 12.05.24.
//


#include "network_msg_parser.h"
#include "game_rules.h"

#include "string"
#include "vector"
#include "regex"
#include "string_view"

#define _CARD "(([2-9]|10|J|Q|K|A)(C|D|H|S))"
#define _SIDE "[NSEW]"
#define _RN "([1-9]|1[0123])"

constexpr std::string_view patterns[] = {
        "IAM" _SIDE,
        "BUSY" _SIDE "{1,4}",
        "DEAL[1-7]" _SIDE _CARD "{13}",
        "TRICK" _RN _CARD "{0,3}",
        "TRICK" _RN _CARD,
        "WRONG" _RN,
        "TAKEN" _RN _CARD "{4}" _SIDE,
        "SCORE(" _SIDE "(0|([1-9][0-9]*))){4}",
        "TOTAL(" _SIDE "(0|([1-9][0-9]*))){4}"
};

constexpr int PATTERN_NO = 9;

resp_array parse_msg(std::string msg, bool server_side) {
    std::regex reg;
    std::smatch match;
    resp_array res;
    for (int i = 0; i < PATTERN_NO; i ++) {
        if (server_side) {
            if ((i != 0) && (i != 4)) continue;
        }
        else {
            if (i == 0 || i == 4) continue;
        }
        reg = std::regex(std::string(patterns[i]));
        if (std::regex_match(msg, reg)) {
            size_t itr;
            int start;
            switch (i) {
                case 0:
                    res.push_back(std::make_pair("type", "IAM"));
                    res.push_back(std::make_pair("side", msg.substr(3,1)));
                    break;
                case 1:
                    res.push_back(std::make_pair("type", "BUSY"));
                    for (size_t j = 4; j < msg.size(); j ++) {
                        res.push_back(std::make_pair("side", msg.substr(j, 1)));
                    }
                    break;
                case 2:
                    res.emplace_back("type", "DEAL");
                    res.emplace_back("round_mode", msg.substr(4, 1));
                    res.emplace_back("side", msg.substr(5, 1));
                    itr = 6;
                    for (int j = 0; j < TRICKS_PER_ROUND; j ++){
                        std::string card;
                        if (msg.at(itr) == '1') {
                            card = msg.substr(itr, 3);
                            itr += 1;
                        }
                        else card = msg.substr(itr, 2);
                        itr += 2;
                        res.emplace_back("card", card);
                    }
                    break;
                case 3:
                    itr = 7;
                    res.emplace_back("type", "TRICK_S");
                    if (msg.size() <= 7) {
                        res.emplace_back("rn", msg.substr(5));
                    }
                    else {
                        char ind = msg.at(7);
                        if (ind == 'C' || ind == 'H' || ind == 'D' || ind == 'S' || ind == '0') {
                            res.emplace_back("rn", msg.substr(5, 1));
                            itr -= 1;
                        }
                        else res.emplace_back("rn", msg.substr(5, 2));
                    }
                    while (itr < msg.size()) {
                        std::string card;
                        if (msg.at(itr) == '1') {
                            card = msg.substr(itr, 3);
                            itr += 1;
                        }
                        else card = msg.substr(itr, 2);
                        itr += 2;
                        res.emplace_back("card", card);
                    }
                    break;
                case 4:
                    if (msg.at(msg.size() - 2) == '0') {
                        res.emplace_back("card", msg.substr(msg.size() - 3, 3));
                        itr = msg.size() - 4;
                    }
                    else {
                        res.emplace_back("card", msg.substr(msg.size() - 2, 2));
                        itr = msg.size() - 3;
                    }
                    res.emplace_back("rn", msg.substr(5, itr - 4));
                    res.emplace_back("type", "TRICK_C");
                    std::reverse(res.begin(), res.end());
                    break;
                case 5:
                    res.emplace_back("type", "WRONG");
                    res.emplace_back("rn", msg.substr(5));
                    break;
                case 6:
                    res.emplace_back("side", msg.substr(msg.size() - 1));
                    itr = msg.size() - 2;
                    for (int j = 0; j < 4; j++ ) {
                        if (msg.at(itr - 1) == '0') {
                            res.emplace_back("card", msg.substr(itr - 2, 3));
                            itr -= 1;
                        }
                        else res.emplace_back("card", msg.substr(itr - 1, 2));
                        itr -= 2;
                    }
                    res.emplace_back("rn", msg.substr(5, itr - 4));
                    res.emplace_back("type", "TAKEN");
                    std::reverse(res.begin(), res.end());
                    break;
                case 7:
                    res.emplace_back("type", "SCORE");
                    itr = 5;
                    for (int j = 0; j < 4; j ++) {
                        res.emplace_back("side", msg.substr(itr ++, 1));
                        start = itr;
                        while (itr + 1 < msg.size() && msg.at(itr + 1) >= '0' && msg.at(itr + 1) <= '9') itr ++;
                        res.emplace_back("score", msg.substr(start, itr - start + 1));
                    }
                    break;
                case 8:
                    res.emplace_back("type", "TOTAL");
                    itr = 5;
                    for (int j = 0; j < 4; j ++) {
                        res.emplace_back("side", msg.substr(itr ++, 1));
                        start = itr;
                        while (itr + 1 < msg.size() && msg.at(itr + 1) >= '0' && msg.at(itr + 1) <= '9') itr ++;
                        res.emplace_back("score", msg.substr(start, itr - start + 1));
                    }
                    break;
            }
            break;
        }
    }
    return res;
}

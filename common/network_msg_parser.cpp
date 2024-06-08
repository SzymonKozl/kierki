//
// Created by szymon on 12.05.24.
//


#include "network_msg_parser.h"
#include "constants.h"
#include "utils.h"

#include "string"
#include "vector"
#include "regex"
#include "string_view"
#include "cassert"

#define CARD_ "(([2-9]|10|J|Q|K|A)(C|D|H|S))"
#define SIDE_ "[NSEW]"
#define RN_ "([1-9]|1[0123])"

constexpr std::string_view patterns[] = {
        "IAM" SIDE_,
        "BUSY" SIDE_ "{1,4}",
        "DEAL[1-7]" SIDE_ CARD_ "{13}",
        "TRICK" RN_ CARD_ "{0,3}",
        "TRICK" RN_ CARD_,
        "WRONG" RN_,
        "TAKEN" RN_ CARD_ "{4}" SIDE_,
        "SCORE(" SIDE_ "(0|([1-9][0-9]*))){4}",
        "TOTAL(" SIDE_ "(0|([1-9][0-9]*))){4}"
};

constexpr int PATTERN_NO = 9;

ParseResp parseNetMsg(std::string m, bool sererSide) {
    std::regex reg;
    std::smatch match;
    ParseResp res;
    for (int i = 0; i < PATTERN_NO; i ++) {
        if (sererSide) {
            if ((i != 0) && (i != 4)) continue;
        }
        else {
            if (i == 0 || i == 4) continue;
        }
        std::string str(patterns[i]);
        reg = std::regex(str);
        if (std::regex_match(m, reg)) {
            size_t itr, start;
            switch (i) {
                case 0:
                    res.emplace_back("type", "IAM");
                    res.emplace_back("side", m.substr(3, 1));
                    break;
                case 1:
                    res.emplace_back("type", "BUSY");
                    for (size_t j = 4; j < m.size(); j ++) {
                        res.emplace_back("side", m.substr(j, 1));
                    }
                    break;
                case 2:
                    res.emplace_back("type", "DEAL");
                    res.emplace_back("round_mode", m.substr(4, 1));
                    res.emplace_back("side", m.substr(5, 1));
                    itr = 6;
                    for (int j = 0; j < TRICKS_PER_ROUND; j ++){
                        std::string card;
                        if (m.at(itr) == '1') {
                            card = m.substr(itr, 3);
                            itr += 1;
                        }
                        else card = m.substr(itr, 2);
                        itr += 2;
                        res.emplace_back("card", card);
                    }
                    break;
                case 3:
                    itr = 7;
                    res.emplace_back("type", "TRICK_S");
                    if (m.size() <= 7) {
                        res.emplace_back("rn", m.substr(5));
                    }
                    else {
                        char ind = m.at(7);
                        if (ind == 'C' || ind == 'H' || ind == 'D' || ind == 'S' || ind == '0') {
                            res.emplace_back("rn", m.substr(5, 1));
                            itr -= 1;
                        }
                        else res.emplace_back("rn", m.substr(5, 2));
                    }
                    while (itr < m.size()) {
                        std::string card;
                        if (m.at(itr) == '1') {
                            card = m.substr(itr, 3);
                            itr += 1;
                        }
                        else card = m.substr(itr, 2);
                        itr += 2;
                        res.emplace_back("card", card);
                    }
                    break;
                case 4:
                    if (m.at(m.size() - 2) == '0') {
                        res.emplace_back("card", m.substr(m.size() - 3, 3));
                        itr = m.size() - 4;
                    }
                    else {
                        res.emplace_back("card", m.substr(m.size() - 2, 2));
                        itr = m.size() - 3;
                    }
                    res.emplace_back("rn", m.substr(5, itr - 4));
                    res.emplace_back("type", "TRICK_C");
                    std::reverse(res.begin(), res.end());
                    break;
                case 5:
                    res.emplace_back("type", "WRONG");
                    res.emplace_back("rn", m.substr(5));
                    break;
                case 6:
                    res.emplace_back("side", m.substr(m.size() - 1));
                    itr = m.size() - 2;
                    for (int j = 0; j < 4; j++ ) {
                        if (m.at(itr - 1) == '0') {
                            res.emplace_back("card", m.substr(itr - 2, 3));
                            itr -= 1;
                        }
                        else res.emplace_back("card", m.substr(itr - 1, 2));
                        itr -= 2;
                    }
                    res.emplace_back("rn", m.substr(5, itr - 4));
                    res.emplace_back("type", "TAKEN");
                    std::reverse(res.begin(), res.end());
                    break;
                case 7:
                    res.emplace_back("type", "SCORE");
                    itr = 5;
                    for (int j = 0; j < 4; j ++) {
                        res.emplace_back("side", m.substr(itr ++, 1));
                        start = itr;
                        while (itr + 1 < m.size() && m.at(itr + 1) >= '0' && m.at(itr + 1) <= '9') itr ++;
                        res.emplace_back("score", m.substr(start, itr - start + 1));
                        itr ++;
                    }
                    break;
                case 8:
                    res.emplace_back("type", "TOTAL");
                    itr = 5;
                    for (int j = 0; j < 4; j ++) {
                        res.emplace_back("side", m.substr(itr ++, 1));
                        start = itr;
                        while (itr + 1 < m.size() && m.at(itr + 1) >= '0' && m.at(itr + 1) <= '9') itr ++;
                        res.emplace_back("score", m.substr(start, itr - start + 1));
                        itr ++;
                    }
                    break;
                default:
                    ASSERT_UNREACHABLE;
                    break;
            }
            break;
        }
    }
    return res;
}

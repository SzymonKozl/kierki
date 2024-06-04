//
// Created by szymon on 11.05.24.
//

#include "send_job.h"

#include <utility>

#include "iostream"

SendJob::SendJob(std::string &&msg_prefix):
        msg_prefix(msg_prefix)
{}

SendJobBusy::SendJobBusy(const std::vector<Side> &taken):
        SendJob("BUSY"),
        taken(taken)
{}

std::string SendJobBusy::genMsg() const {
    std::string res = msg_prefix;
    for (Side s: taken) res += (char) s;
    return res + "\r\n";
}

SendJobIntro::SendJobIntro(Side s):
        SendJob("IAM"),
        s(s)
{}

std::string SendJobIntro::genMsg() const {
    std::string res = msg_prefix;
    res += (char) s;
    return res + "\r\n";
}

SendJobScore::SendJobScore(const std::unordered_map<Side, int>& scores):
        SendJob("SCORE"),
        scores(scores)
{}

std::string SendJobScore::genMsg() const {
    std::string res = msg_prefix;
    for (auto itr = scores.begin(); itr != scores.end(); itr ++) {
        res += (char) itr->first;
        res += std::to_string(itr->second);
    }
    return res + "\r\n";
}

SendJobTaken::SendJobTaken(Table cards, Side s, int trickNo):
        SendJob("TAKEN"),
        s(s),
        trickNo(trickNo),
        table(std::move(cards))
{}

std::string SendJobTaken::genMsg() const {
    std::string res = msg_prefix + std::to_string(trickNo);
    for (const Card& c: table) {
        res += c.toString();
    }
    res += (char) s;
    return res + "\r\n";
}

SendJobTotal::SendJobTotal(const std::unordered_map<Side, int>& scores):
        SendJob("TOTAL"),
        scores(scores)
{}

std::string SendJobTotal::genMsg() const {
    std::string res = msg_prefix;
    for (auto score : scores) {
        res += (char) score.first;
        res += std::to_string(score.second);
    }
    return res + "\r\n";
}

SendJobTrick::SendJobTrick(Table table, size_t trickNo):
        SendJob("TRICK"),
        tableState(std::move(table)),
        trickNo(trickNo)
{}

std::string SendJobTrick::genMsg() const {
    std::string res = msg_prefix + std::to_string(trickNo);
    for (const Card& c: tableState) {
        res += c.toString();
    }
    return res + "\r\n";
}

SendJobWrong::SendJobWrong(size_t trick_no):
        SendJob("WRONG"),
        trick_no(trick_no)
{}

std::string SendJobWrong::genMsg() const {
    return msg_prefix + std::to_string(trick_no) + "\r\n";
}

SendDealJob::SendDealJob(RoundType roundType, Side starting, Hand cards):
        SendJob("DEAL"),
        roundType(roundType),
        starting(starting),
        cards(std::move(cards))
{}

std::string SendDealJob::genMsg() const {
    std::string res = msg_prefix + std::to_string(roundType);
    res += (char) starting;
    for (const Card& c: cards) res += c.toString();
    return res + "\r\n";
}

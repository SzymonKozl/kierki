//
// Created by szymon on 11.05.24.
//

#include "send_job.h"

#include <utility>

#include "iostream"

SendJob::SendJob(std::string &&msgPrefix):
        msgPrefix(msgPrefix)
{}

SendJobBusy::SendJobBusy(const std::vector<Side> &taken):
        SendJob("BUSY"),
        taken(taken)
{}

std::string SendJobBusy::genMsg() const {
    std::string res = msgPrefix;
    for (Side s: taken) res += static_cast<char>(s);
    return res + "\r\n";
}

SendJobIntro::SendJobIntro(Side s):
        SendJob("IAM"),
        s(s)
{}

std::string SendJobIntro::genMsg() const {
    std::string res = msgPrefix;
    res += static_cast<char>(s);
    return res + "\r\n";
}

SendJobScore::SendJobScore(const std::unordered_map<Side, int>& scores):
        SendJob("SCORE"),
        scores(scores)
{}

std::string SendJobScore::genMsg() const {
    std::string res = msgPrefix;
    for (auto score : scores) {
        res += static_cast<char>(score.first);
        res += std::to_string(score.second);
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
    std::string res = msgPrefix + std::to_string(trickNo);
    for (const Card& c: table) {
        res += c.toString();
    }
    res += static_cast<char>(s);
    return res + "\r\n";
}

SendJobTotal::SendJobTotal(const std::unordered_map<Side, int>& scores):
        SendJob("TOTAL"),
        scores(scores)
{}

std::string SendJobTotal::genMsg() const {
    std::string res = msgPrefix;
    for (auto score : scores) {
        res += static_cast<char>(score.first);
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
    std::string res = msgPrefix + std::to_string(trickNo);
    for (const Card& c: tableState) {
        res += c.toString();
    }
    return res + "\r\n";
}

SendJobWrong::SendJobWrong(size_t trickNo):
        SendJob("WRONG"),
        trickNo(trickNo)
{}

std::string SendJobWrong::genMsg() const {
    return msgPrefix + std::to_string(trickNo) + "\r\n";
}

SendDealJob::SendDealJob(RoundType roundType, Side starting, Hand cards):
        SendJob("DEAL"),
        roundType(roundType),
        starting(starting),
        cards(std::move(cards))
{}

std::string SendDealJob::genMsg() const {
    std::string res = msgPrefix + std::to_string(roundType);
    res += static_cast<char>(starting);
    for (const Card& c: cards) res += c.toString();
    return res + "\r\n";
}

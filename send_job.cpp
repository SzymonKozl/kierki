//
// Created by szymon on 11.05.24.
//

#include "send_job.h"

#include <utility>

#include "iostream"

SendJob::SendJob(std::string &&msg_prefix, bool disconnectAfter, bool responseExpected, bool overrideLast, bool repeatable):
        disconnectAfter(disconnectAfter),
        responseExpected(responseExpected),
        overrideLast(overrideLast),
        repeatable(repeatable),
        msg_prefix(msg_prefix)
{}

void SendJob::setDisconnectAfter(bool val) noexcept {
    disconnectAfter = val;
}

bool SendJob::shouldDisconnectAfter() const noexcept {
    return disconnectAfter;
}

bool SendJob::isResponseExpected() const noexcept {
    return responseExpected;
}

bool SendJob::registrable() const noexcept {
    return overrideLast;
}

bool SendJob::isRepeatable() const noexcept {
    return repeatable;
}

SendJobBusy::SendJobBusy(const std::vector<Side> &taken):
        SendJob("BUSY", true, false, true),
        taken(taken)
{}

std::string SendJobBusy::genMsg() const {
    std::string res = msg_prefix;
    for (Side s: taken) res += (char) s;
    return res + "\r\n";
}

SendJobIntro::SendJobIntro(Side s):
        SendJob("IAM", false, false, true),
        s(s)
{}

std::string SendJobIntro::genMsg() const {
    std::string res = msg_prefix;
    res += (char) s;
    return res + "\r\n";
}

SendJobScore::SendJobScore(const std::unordered_map<Side, int>& scores):
        SendJob("SCORE", false, false, true),
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
        SendJob("TAKEN", false, false, true),
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
        SendJob("TOTAL", false, false, true),
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

SendJobTrick::SendJobTrick(Table table, int trickNo, bool serverSide):
        SendJob("TRICK", false, serverSide, true, serverSide),
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

SendJobWrong::SendJobWrong(int trick_no, bool outOfOrder):
        SendJob("WRONG", false, !outOfOrder, false),
        trick_no(trick_no)
{}

std::string SendJobWrong::genMsg() const {
    return msg_prefix + std::to_string(trick_no) + "\r\n";
}

SendDealJob::SendDealJob(RoundType roundType, Side starting, Hand cards):
        SendJob("DEAL", false, false, true),
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

//
// Created by szymon on 11.05.24.
//

#include "send_job.h"

SendJob::SendJob(std::string &&msg_prefix, bool disconnectAfter):
        disconnectAfter(disconnectAfter),
        msg_prefix(msg_prefix)
{}

void SendJob::setDisconnectAfter(bool val) noexcept {
    disconnectAfter = val;
}

bool SendJob::shouldDisconnectAfter() const noexcept {
    return disconnectAfter;
}

SendJobBusy::SendJobBusy(const std::vector<Side> &taken):
        SendJob("BUST", true),
        taken(taken)
{}

std::string SendJobBusy::genMsg() const {
    std::string res = msg_prefix;
    for (Side s: taken) res += s;
    return res + "\r\n";
}

SendJobIntro::SendJobIntro(Side s):
        SendJob("IAM", false),
        s(s)
{}

std::string SendJobIntro::genMsg() const {
    std::string res = msg_prefix;
    res += s;
    return res + "\r\n";
}

SendJobScore::SendJobScore(const std::unordered_map<Side, int>& scores):
        SendJob("SCORE", false),
        scores(scores)
{}

std::string SendJobScore::genMsg() const {
    std::string res = msg_prefix;
    for (auto itr = scores.begin(); itr != scores.end(); itr ++) {
        res += itr->first;
        res += std::to_string(itr->second);
    }
    return res + "\r\n";
}

SendJobTaken::SendJobTaken(const Table &cards, Side s, int trickNo):
        SendJob("TAKEN", false),
        s(s),
        trickNo(trickNo),
        table(cards)
{}

std::string SendJobTaken::genMsg() const {
    std::string res = msg_prefix + std::to_string(trickNo);
    for (const Card& c: table) {
        res += c.toString();
    }
    res += s;
    return res + "\r\n";
}

SendJobTotal::SendJobTotal(std::unordered_map<Side, int> scores):
        SendJob("TOTAL", false),
        scores(scores)
{}

std::string SendJobTotal::genMsg() const {
    std::string res = msg_prefix;
    for (auto itr = scores.begin(); itr != scores.end(); itr ++) {
        res += itr->first;
        res += std::to_string(itr->second);
    }
    return res + "\r\n";
}

SendJobTrick::SendJobTrick(const Table &table_state, int trickNo):
        SendJob("TRICK", false),
        tableState(table_state),
        trickNo(trickNo)
{}

std::string SendJobTrick::genMsg() const {
    std::string res = msg_prefix + std::to_string(trickNo);
    for (const Card& c: tableState) {
        res = res + c.toString();
    }
    return res + "\r\n";
}

SendJobWrong::SendJobWrong(int trick_no):
        SendJob("WRONG", false),
        trick_no(trick_no)
{}

std::string SendJobWrong::genMsg() const {
    return msg_prefix + std::to_string(trick_no) + "\r\n";
}

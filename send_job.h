//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_SENDJOB_H
#define KIERKI_SENDJOB_H

#include "common_types.h"

#include "string"
#include "vector"

class SendJob {
public:
    virtual std::string genMsg() const;
    bool shouldDisconnectAfter() const noexcept;
    void setDisconnectAfter(bool val) noexcept;
protected:
    SendJob(std::string &&msg_prefix, bool disconnectAfter);
    const std::string msg_prefix;
    bool disconnectAfter;
};

class SendJobBusy: SendJob{
public:
    SendJobBusy(std::vector<Side> const& taken);
    std::string genMsg() const override;
private:
    const std::vector<Side> &taken;
};

class SendDealJob: SendJob {
public:
    SendDealJob(RoundType roundType, Side starting, const Hand& cards);
    std::string genMsg() const override;
private:
    const RoundType roundType;
    const Side starting;
    const Hand& cards;
};

class SendJobIntro: SendJob{
public:
    SendJobIntro(Side s);
    std::string genMsg() const override;
private:
    const Side s;
};

class SendJobScore : SendJob {
public:
    SendJobScore(const std::unordered_map<Side, int> &scores);

    std::string genMsg() const override;
private:
    const std::unordered_map<Side, int>& scores;
};

class SendJobTaken: SendJob {
public:
    SendJobTaken(Table const& cards, Side s, int trickNo);
    std::string genMsg() const override;
private:
    const Side s;
    const int trickNo;
    const Table& table;
};

class SendJobTotal: SendJob {
public:
    SendJobTotal(std::unordered_map<Side, int> scores);
    std::string genMsg() const override;
private:
    const std::unordered_map<Side, int> scores;
};

class SendJobTrick: SendJob{
public:
    SendJobTrick(Table const& table_state, int trickNo);
    std::string genMsg() const override;
private:
    const Table& tableState;
    const int trickNo;
};

class SendJobWrong: SendJob {
public:
    SendJobWrong(int trick_no);
    std::string genMsg() const override;
private:
    int trick_no;
};

SendDealJob::SendDealJob(RoundType roundType, Side starting, const Hand &cards):
        SendJob("DEAL", false),
        roundType(roundType),
        starting(starting),
        cards(cards)
{}

std::string SendDealJob::genMsg() const {
    std::string res = msg_prefix + std::to_string(roundType);
    res += starting;
    for (const Card& c: cards) res += c.toString();
    return res + "\r\n";
}

#endif //KIERKI_SENDJOB_H

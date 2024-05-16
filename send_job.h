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

class SendJobBusy: public SendJob{
public:
    SendJobBusy(std::vector<Side> const& taken);
    std::string genMsg() const override;
private:
    const std::vector<Side> &taken;
};

class SendDealJob: public SendJob {
public:
    SendDealJob(RoundType roundType, Side starting, const Hand& cards);
    std::string genMsg() const override;
private:
    const RoundType roundType;
    const Side starting;
    const Hand& cards;
};

class SendJobIntro: public SendJob{
public:
    SendJobIntro(Side s);
    std::string genMsg() const override;
private:
    const Side s;
};

class SendJobScore : public SendJob {
public:
    SendJobScore(const std::unordered_map<Side, int> &scores);

    std::string genMsg() const override;
private:
    const std::unordered_map<Side, int>& scores;
};

class SendJobTaken: public SendJob {
public:
    SendJobTaken(Table const& cards, Side s, int trickNo);
    std::string genMsg() const override;
private:
    const Side s;
    const int trickNo;
    const Table& table;
};

class SendJobTotal: public SendJob {
public:
    SendJobTotal(std::unordered_map<Side, int> scores);
    std::string genMsg() const override;
private:
    const std::unordered_map<Side, int> scores;
};

class SendJobTrick: public SendJob{
public:
    SendJobTrick(Table const& table_state, int trickNo);
    std::string genMsg() const override;
private:
    const Table& tableState;
    const int trickNo;
};

class SendJobWrong: public SendJob {
public:
    SendJobWrong(int trick_no);
    std::string genMsg() const override;
private:
    int trick_no;
};

#endif //KIERKI_SENDJOB_H

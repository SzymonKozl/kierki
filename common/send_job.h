//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_SENDJOB_H
#define KIERKI_SENDJOB_H

#include "common_types.h"

#include "string"
#include "vector"
#include "memory"

class SendJob {
public:
    virtual std::string genMsg() const = 0;
protected:
    explicit SendJob(std::string &&msgPrefix);
    const std::string msgPrefix;
};

using SSendJob = std::shared_ptr<SendJob>;

class SendJobBusy: public SendJob{
public:
    explicit SendJobBusy(std::vector<Side> const& taken);
    std::string genMsg() const override;
private:
    const std::vector<Side> taken;
};

class SendDealJob: public SendJob {
public:
    SendDealJob(RoundType roundType, Side starting, Hand  cards);
    std::string genMsg() const override;
private:
    const RoundType roundType;
    const Side starting;
    const Hand cards;
};

class SendJobIntro: public SendJob{
public:
    explicit SendJobIntro(Side s);
    std::string genMsg() const override;
private:
    const Side s;
};

class SendJobScore : public SendJob {
public:
    explicit SendJobScore(const std::unordered_map<Side, int> &scores);

    std::string genMsg() const override;
private:
    const std::unordered_map<Side, int> scores;
};

class SendJobTaken: public SendJob {
public:
    SendJobTaken(Table  cards, Side s, int trickNo);
    std::string genMsg() const override;
private:
    const Side s;
    const int trickNo;
    const Table table;
};

class SendJobTotal: public SendJob {
public:
    explicit SendJobTotal(const std::unordered_map<Side, int>& scores);
    std::string genMsg() const override;
private:
    const std::unordered_map<Side, int> scores;
};

class SendJobTrick: public SendJob{
public:
    SendJobTrick(Table  table, size_t trickNo);
    std::string genMsg() const override;
private:
    const Table tableState;
    const size_t trickNo;
};

class SendJobWrong: public SendJob {
public:
    explicit SendJobWrong(size_t trickNo);
    std::string genMsg() const override;
private:
    size_t trickNo;
};

#endif //KIERKI_SENDJOB_H

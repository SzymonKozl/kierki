//
// Created by szymon on 13.05.24.
//

#ifndef KIERKI_SERVER_H
#define KIERKI_SERVER_H

#include "io_worker_mgr.h"
#include "common_types.h"
#include "game_rules.h"
#include "logger.h"

#include "unordered_map"
#include "unordered_set"
#include "mutex"
#include "string"
#include "functional"

using active_map = std::unordered_map<Side, int>;

class Server {
public:
    explicit Server(game_scenario &&scenario, uint16_t port, int timeout);
    void run();
private:
    bool furtherMovesNeeded() noexcept;
    void handleSysErr(ErrInfo info);
    bool playerTricked(int trickNoArg, Card card, int workerIx);
    void playerIntro(Side side, int workerIx);
    void prepareRound();
    void forwardConnection(int fd, net_address conn_addr);
    void workerQuits(int status);
    int makeTCPSock(uint16_t port);
    void updatePenalties();
    void clearTmpPenalties();
    void playerDisconnected(Side s, ErrInfo info);
    void finalize();
    bool grandExitCallback(ErrArr errArr, int workerIx, bool hasWork);
    bool execMutexed(std::function<void()> invokable);
    void handleTimeout(int workerIx);
    bool handleWrongMessage(std::string message, int ix);

    game_scenario gameScenario;
    IOWorkerMgr workerMgr;
    active_map activeSides;
    score_map penalties;
    score_map penaltiesRound;
    table_state hands;
    Side nextMove;
    size_t roundNumber;
    size_t trickNo;
    RoundType roundMode;
    std::mutex gameStateMutex;
    Table table;
    int exitCode;
    int playersConnected;
    net_address own_addr;
    table_state lastDeal;
    std::vector<SSendJob> takenInRound;
    bool exiting;
    int timeout;
    Logger msgLogger;
    std::unordered_map<int, Side> workerToSide;
    std::unordered_map<int, Side> zombieWorkerToSide;
    std::unordered_set<int> zombieWorkers;
    std::unordered_set<int> allIntroduced;
    std::unordered_set<int> sendingBusy;
    bool expectedTrickResponse;
};


#endif //KIERKI_SERVER_H

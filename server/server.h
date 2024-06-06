//
// Created by szymon on 13.05.24.
//

#ifndef KIERKI_SERVER_H
#define KIERKI_SERVER_H

#include "io_worker_mgr.h"
#include "../common/common_types.h"
#include "game_rules.h"
#include "../common/logger.h"

#include "unordered_map"
#include "unordered_set"
#include "mutex"
#include "string"
#include "functional"

using active_map = std::unordered_map<Side, int>;

class Server {
public:
    explicit Server(game_scenario &&scenario, uint16_t port, int timeout);
    int run();
private:
    bool furtherMovesNeeded() noexcept;
    bool handleSysErr(const ErrInfo& info, bool locked = false);
    bool playerTricked(size_t trickNoArg, Card card, int workerIx);
    bool playerIntro(Side side, int workerIx);
    void prepareRound();
    void forwardConnection(int fd, net_address conn_addr);
    int makeTCPSock(uint16_t port);
    void updatePenalties();
    void clearTmpPenalties();
    void finalize();
    bool grandExitCallback(ErrArr errArr, int workerIx, bool hasWork);
    bool execMutexed(std::function<void()>&& invokable);
    void handleTimeout(int workerIx);
    bool handleWrongMessage(int ix);

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
};


#endif //KIERKI_SERVER_H

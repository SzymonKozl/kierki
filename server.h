//
// Created by szymon on 13.05.24.
//

#ifndef KIERKI_SERVER_H
#define KIERKI_SERVER_H

#include "io_worker_mgr.h"
#include "common_types.h"
#include "game_rules.h"

#include "unordered_map"
#include "mutex"
#include "string"

using active_map = std::unordered_map<Side, int>;

class Server {
public:
    Server(game_scenario &&scenario);
    void run();
private:
    bool furtherMovesNeeded() noexcept;
    void handleSysErr(const std::string& call, int error, int type);
    void playerTricked(Side side, Card card);
    void playerIntro(Side side, int workerIx);
    void prepareRound();
    void forwardConnection(int fd, net_address conn_addr);
    void workerQuits(int status);
    int makeTCPSock(uint16_t port);
    void updatePenalties();
    void clearTmpPenalties();
    void playerDisconnected(Side s);

    game_scenario gameScenario;
    IOWorkerMgr workerMgr;
    active_map activeSides;
    score_map penalties;
    score_map penaltiesRound;
    table_state hands;
    Side nextMove;
    int roundNumber;
    int trickNo;
    RoundType roundMode;
    std::mutex gameStateMutex;
    Table table;
    bool exitFlag;
    int playersConnected;
    net_address own_addr;
};


#endif //KIERKI_SERVER_H

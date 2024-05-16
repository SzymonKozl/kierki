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

using score_map = std::unordered_map<Side, int>;
using active_map = std::unordered_map<Side, int>;

class Server {
public:
    Server(game_scenario &&scenario);
    void run();
private:
    bool furtherMovesNeeded() noexcept;
    void handleSysErr(std::string call, int error, int type, Side side);
    void playerTricked(Side side, Card card);
    void playerIntro(Side side, int workerIx);
    void prepareRound();
    void forwardConnection(int fd);
    void workerQuits(int status);
    int makeTCPSock(uint16_t port);
    void updatePenalties();
    void clearTmpPenalties();

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
};


#endif //KIERKI_SERVER_H

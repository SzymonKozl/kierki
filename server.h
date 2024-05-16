//
// Created by szymon on 13.05.24.
//

#ifndef KIERKI_SERVER_H
#define KIERKI_SERVER_H

#include "io_worker_mgr.h"
#include "common_types.h"

#include "unordered_map"
#include "mutex"
#include "string"

class Server {
public:
    Server(game_scenario &&scenario);
    void run();
private:
    bool furtherMovesNeeded() noexcept;
    void handleSysErr(std::string call, int error, int type, Side side);
    void playerTricked(Side side, Card card);
    void playerIntro(Side side);
    void prepareRound();
    void forwardConnection(int fd);
    void workerQuits(int status);
    int makeTCPSock();

    game_scenario gameScenario;
    IOWorkerMgr workerMgr;
    std::unordered_map<Side, bool> activeSides;
    std::unordered_map<Side, Hand> hands;
    std::unordered_map<Side, int> penalties;
    Side nextMove;
    int roundNumber;
    int roundMode;
    std::mutex gameStateMutex;
    Table table;
    bool exitFlag;
    bool readyToPlay;
};


#endif //KIERKI_SERVER_H

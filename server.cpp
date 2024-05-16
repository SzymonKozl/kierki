//
// Created by szymon on 13.05.24.
//

#include "server.h"
#include "io_worker_connect.h"
#include "utils.h"

#include "string"
#include "functional"
#include "iostream"
#include "mutex"

Server::Server(game_scenario &&scenario):
    gameScenario(scenario),
    workerMgr([&](std::string s, int e) { handleSysErr(s, e, IO_ERR_INTERNAL);}),
    activeSides(),
    hands(),
    penalties(),
    nextMove(W),
    roundNumber(0),
    gameStateMutex(),
    table(),
    roundMode(0),
    exitFlag(false),
    readyToPlay(false)
{
    for (Side s: {W, E, S, N}) {
        activeSides[s] = false;
        hands[s] = Hand();
        penalties[s] = 0;
    }
}

void Server::run() {
    prepareRound();
    int tcp_listen_sock = makeTCPSock();
    workerMgr.spawnNewWorker<IOWorkerConnect>(
        tcp_listen_sock,
        [&](int status) { workerQuits(status);},
        [&](std::string s, int e, int t) { handleSysErr(s, e, t);},
        [&](int fd) { forwardConnection(fd);}
    );
    // todo job for main
}

bool Server::furtherMovesNeeded() noexcept {
    if (roundMode == 1 || roundMode == 6 || roundMode == 7) {
        bool emptied = true;
        for (auto it = hands.begin(); it != hands.end(); it++) {
            if (!it->second.empty()) {
                emptied = false;
                break;
            }
        }
        return !emptied;
    }
    else {
        std::function<bool(const Card&)> filter;
        switch (roundMode) {
            case 2:
                filter = [](const Card& c) {return c.getColor() == COLOR_H;};
                break;
            case 3:
                filter = [](const Card& c) {return c.getValue() == "Q";};
                break;
            case 4:
                filter = [](const Card& c) {return c.getValue() == "K" || c.getValue() == "J";};
                break;
            case 5:
                filter = [](const Card& c) {return c.getValue() == "K" and c.getColor() == COLOR_H;};
                break;
        }
        bool found = false;
        for (auto it = hands.begin(); it != hands.end(); it ++) {
            for (const Card& c: it->second) {
                if (filter(c)) {
                    found = true;
                    break;
                }
            }
        }
        return found;
    }
}

void Server::handleSysErr(std::string call, int error, int type, Side side) {
    std::lock_guard<std::mutex> lock(gameStateMutex);
    std::cerr << "System error on " << call << " call! Error code: " << error << std::endl;
    exitFlag = true;
    if (type == IO_ERR_INTERNAL) {
        std::cerr << "internal error! quitting server";
        workerMgr.killAll();
    }
}

void Server::workerQuits(int id) {
    workerMgr.eraseWorker(id);
}



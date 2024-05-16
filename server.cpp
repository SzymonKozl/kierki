//
// Created by szymon on 13.05.24.
//

#include "server.h"
#include "io_worker_connect.h"
#include "utils.h"
#include "game_rules.h"
#include "common_types.h"

#include "string"
#include "functional"
#include "iostream"
#include "mutex"
#include "stdexcept"
#include "sys/socket.h"
#include "arpa/inet.h"

Server::Server(game_scenario &&scenario):
    gameScenario(scenario),
    workerMgr([&](std::string s, int e) { handleSysErr(s, e, IO_ERR_INTERNAL, Side::_SIDE_NULL);}),
    activeSides(),
    hands(),
    penalties(),
    nextMove(W),
    roundNumber(0),
    trickNo(1),
    gameStateMutex(),
    table(),
    roundMode(TRICK_PENALTY),
    exitFlag(false),
    playersConnected(0)
{
    for (Side s: {W, E, S, N}) {
        activeSides[s] = -1;
        hands[s] = Hand();
        penalties[s] = 0;
    }
}

void Server::run() {
    prepareRound();
    int tcp_listen_sock = makeTCPSock(9009);
    workerMgr.spawnNewWorker<IOWorkerConnect>(
        tcp_listen_sock,
        [&](int status) { workerQuits(status);},
        [&](std::string s, int e, int t, Side side) { handleSysErr(s, e, t, side);},
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
    std::lock_guard<std::mutex> lock(gameStateMutex);
    workerMgr.eraseWorker(id);
}

void Server::updatePenalties() {
    for (Side s: sides_) penalties[s] += penaltiesRound[s];
}

void Server::clearTmpPenalties() {
    for (Side s: sides_) penaltiesRound[s] = 0;
}

void Server::prepareRound() {
    auto& [mode, init_state, starting] = gameScenario[roundNumber];
    roundMode = mode;
    hands = init_state;
    nextMove = starting;
    trickNo = 1;
}

void Server::playerTricked(Side side, Card card) {
    std::lock_guard<std::mutex> lock(gameStateMutex);
    if (nextMove != side || !GameRules::isMoveLegal(side, card, hands)) {
        workerMgr.sendJob(*(new SendJobWrong(trickNo)), activeSides[side]);
    }

    table.push_back(card);
    nextMove = nxtSide(nextMove);
    if (table.size() == 4ul) {
        auto [taker, penalty] = GameRules::whoTakes(nextMove, table, roundMode, trickNo);
        nextMove = taker;
        penaltiesRound[taker] += penalty;
        SendJobTaken msgTaken(table, taker, trickNo);
        for (Side s: sides_) workerMgr.sendJob(msgTaken, activeSides[s]);
        if (hands[W].empty()) {
            updatePenalties();
            SendJobScore msgScore(penaltiesRound);
            SendJobTotal msgTotal(penalties);
            if (roundNumber == gameScenario.size() - 1) {
                msgTotal.setDisconnectAfter(true);
            }
            for (Side s: sides_) {
                workerMgr.sendJob(msgScore, activeSides[s]);
                workerMgr.sendJob(msgTotal, activeSides[s]);
            }
            clearTmpPenalties();
        }
        roundNumber ++;
        if (roundNumber == gameScenario.size()) {
            workerMgr.killAll();
            // what now?
            // todo
            exit(0); // prob bad idea
        }
        prepareRound();
        for (Side s: sides_) {
            SendDealJob msgDeal(roundMode, nextMove, hands[s]);
            workerMgr.sendJob(msgDeal, activeSides[s]);
        }
    }
    workerMgr.sendJob(*(new SendJobTrick(table, roundNumber)), activeSides[nextMove]);
}

void Server::playerIntro(Side side, int workerIx) {
    std::lock_guard<std::mutex> lock(gameStateMutex);
    if (activeSides[side] == -1) {
        // accepting new client
        activeSides[side] = workerIx;
        playersConnected ++;
        if (playersConnected == 4) {
            for (Side s: sides_) {
                SendDealJob msgDeal(roundMode, nextMove, hands[s]);
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
            SendJobTrick msgTrick(table, trickNo);
            workerMgr.sendJob(msgTrick, nextMove);
        }
    }
}

int Server::makeTCPSock(uint16_t port) {
    // todo: ivp6 handling
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error("socket");
    }
    if (port) {
        sockaddr_in server_address;
        server_address.sin_family = AF_INET; // IPv4
        server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
        server_address.sin_port = htons(port);
        if (bind(fd,(const sockaddr *) &server_address, sizeof server_address)) {
            throw std::runtime_error("bind");
        }
    }

    if (listen(fd, 10)) {
        throw std::runtime_error("listen");
    }

    return fd;
}

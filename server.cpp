//
// Created by szymon on 13.05.24.
//

#include "server.h"
#include "io_worker_connect.h"
#include "io_worker_handler.h"
#include "utils.h"
#include "game_rules.h"
#include "common_types.h"
#include "constants.h"

#include "string"
#include "memory"
#include "functional"
#include "iostream"
#include "mutex"
#include "stdexcept"
#include <cstdint>
#include <utility>
#include "sys/socket.h"
#include "arpa/inet.h"

Server::Server(game_scenario &&scenario):
    gameScenario(scenario),
    workerMgr([&](const std::string& s, int e) { handleSysErr(s, e, IO_ERR_INTERNAL);}),
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
    int connector_ix = workerMgr.spawnNewWorker<IOWorkerConnect>(
        tcp_listen_sock,
        (IOWorkerExitCb)[&](int status) { workerQuits(status);},
        (IOWorkerSysErrCb) [&](const std::string& s, int e, int t) { handleSysErr(s, e, t);},
        (IOWorkerConnectionMadeCb ) [&](int fd, net_address conn_addr) { forwardConnection(fd, std::move(conn_addr));}
    );
    workerMgr.joinThread(connector_ix);
}

bool Server::furtherMovesNeeded() noexcept {
    if (roundMode == 1 || roundMode == 6 || roundMode == 7) {
        bool emptied = true;
        for (auto & hand : hands) {
            if (!hand.second.empty()) {
                emptied = false;
                break;
            }
        }
        return !emptied;
    }
    else {
        std::function<bool(const Card&)> filter;
        switch (roundMode) {
            case HEART_PENALTY:
                filter = [](const Card& c) {return c.getColor() == COLOR_H;};
                break;
            case QUEEN_PENALTY:
                filter = [](const Card& c) {return c.getValue() == "Q";};
                break;
            case KING_JACK_PENALTY:
                filter = [](const Card& c) {return c.getValue() == "K" || c.getValue() == "J";};
                break;
            case HEART_KING_PENALTY:
                filter = [](const Card& c) {return c.getValue() == "K" and c.getColor() == COLOR_H;};
                break;
        }
        bool found = false;
        for (auto & hand : hands) {
            for (const Card& c: hand.second) {
                if (filter(c)) {
                    found = true;
                    break;
                }
            }
        }
        return found;
    }
}

void Server::handleSysErr(const std::string& call, int error, int type) {
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
        workerMgr.sendJob(std::static_pointer_cast<SendJob>(std::make_shared<SendJobWrong>(trickNo)), activeSides[side]);
        return;
    }

    table.push_back(card);
    nextMove = nxtSide(nextMove);
    if (table.size() == 4ul) {
        auto [taker, penalty] = GameRules::whoTakes(nextMove, table, roundMode, trickNo);
        nextMove = taker;
        penaltiesRound[taker] += penalty;
        SSendJob msgTaken = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTaken>(table, taker, trickNo));
        trickNo ++;
        table.clear();
        for (Side s: sides_) workerMgr.sendJob(msgTaken, activeSides[s]);
        if (trickNo == TRICKS_PER_ROUND + 1) {
            updatePenalties();
            SSendJob msgScore = std::static_pointer_cast<SendJob>(std::make_shared<SendJobScore>(penaltiesRound));
            SSendJob msgTotal = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTotal>(penalties));
            if (roundNumber == gameScenario.size() - 1) {
                msgTotal->setDisconnectAfter(true);
            }
            for (Side s: sides_) {
                workerMgr.sendJob(msgScore, activeSides[s]);
                workerMgr.sendJob(msgTotal, activeSides[s]);
            }
            clearTmpPenalties();
            roundNumber ++;
            if (roundNumber == gameScenario.size()) {
                sleep(2);
                workerMgr.killAll();
                // what now?
                // todo
                exit(0); // prob bad idea
            }
            prepareRound();
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
        }
    }
    workerMgr.sendJob(std::make_shared<SendJobTrick>(table, trickNo), activeSides[nextMove]);
}

void Server::playerIntro(Side side, int workerIx) {
    std::lock_guard<std::mutex> lock(gameStateMutex);
    if (activeSides[side] == -1) {
        // accepting new client
        activeSides[side] = workerIx;
        playersConnected ++;
        if (playersConnected == 4) {
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
            SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo));
            workerMgr.sendJob(msgTrick, activeSides[nextMove]);
        }
    }
}

int Server::makeTCPSock(uint16_t port) {
    // todo: ivp6 handling
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("socket");
    }
    if (port) {
        sockaddr_in server_address = {AF_INET, htons(port), htonl(INADDR_ANY)};
        if (bind(fd,(const sockaddr *) &server_address, sizeof server_address)) {
            throw std::runtime_error("bind");
        }
    }

    own_addr = getAddrStruct(fd);

    if (listen(fd, 10)) {
        throw std::runtime_error("listen");
    }

    return fd;
}

void Server::forwardConnection(int fd, net_address conn_addr) {
    std::lock_guard<std::mutex> lock(gameStateMutex);
    workerMgr.spawnNewWorker<IOWorkerHandler>(
            fd,
            [&] (int ix) { workerQuits(ix);},
            [&] (const std::string& s, int e, int type) { handleSysErr(s, e, type);},
            [&] (Side s, int ix) { playerIntro(s, ix);},
            [&] (int, Side s, const Card& c) { playerTricked(s, c);}, // todo: do not ignore trickNo
            [&] (Side s, std::string call, int err, int err_type) {/*todo*/},
            conn_addr,
            own_addr
            );
}

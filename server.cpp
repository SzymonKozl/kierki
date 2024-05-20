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
    workerMgr([&](errInfo info) { handleSysErr(info);}),
    activeSides(),
    penalties(),
    hands(),
    nextMove(W),
    roundNumber(0),
    trickNo(1),
    roundMode(TRICK_PENALTY),
    gameStateMutex(),
    table(),
    exitCode(0),
    playersConnected(0),
    lastDeal(),
    takenInRound()
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
        (IOWorkerExitCb)[this](int status) { this->workerQuits(status);},
        (IOWorkerSysErrCb) [this](errInfo info) { this->handleSysErr(info);},
        (IOWorkerConnectionMadeCb ) [this](int fd, net_address conn_addr) { this->forwardConnection(fd, std::move(conn_addr));}
    );
    workerMgr. waitForClearing();
    exit(exitCode);
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

void Server::handleSysErr(errInfo info) {
    MutexGuard lock(gameStateMutex);
    auto [call, error, type] = info;
    std::cerr << "System error on " << call << " call! Error code: " << error << std::endl;
    if (type == IO_ERR_INTERNAL) {
        exitCode = 1;
        std::cerr << "internal error! quitting server";
        finalize();
    }
}

void Server::workerQuits(int id) {
    MutexGuard lock(gameStateMutex);
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

void Server::playerTricked(Side side, Card card, int trickNoArg) {
    MutexGuard lock(gameStateMutex);
    if (playersConnected < 4 || nextMove != side || !GameRules::isMoveLegal(side, card, hands)) {
        workerMgr.sendJob(std::static_pointer_cast<SendJob>(std::make_shared<SendJobWrong>(trickNoArg)), activeSides[side]);
        return;
    }

    table.push_back(card);
    nextMove = nxtSide(nextMove);
    if (table.size() == 4ul) {
        auto [taker, penalty] = GameRules::whoTakes(nextMove, table, roundMode, trickNoArg);
        nextMove = taker;
        penaltiesRound[taker] += penalty;
        SSendJob msgTaken = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTaken>(table, taker, trickNoArg));
        trickNoArg ++;
        table.clear();
        for (Side s: sides_) workerMgr.sendJob(msgTaken, activeSides[s]);
        if (trickNoArg == TRICKS_PER_ROUND + 1) {
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
                finalize();
                return;
            }
            prepareRound();
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
        }
    }
    workerMgr.sendJob(std::make_shared<SendJobTrick>(table, trickNoArg), activeSides[nextMove]);
}

void Server::playerIntro(Side side, int workerIx) {
    MutexGuard lock(gameStateMutex);
    std::cout << "ugabuga\n";
    if (activeSides[side] == -1) {
        // accepting new client
        activeSides[side] = workerIx;
        playersConnected ++;
        if (playersConnected == 4 && lastDeal.use_count() == 0) {
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
            SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo));
            workerMgr.sendJob(msgTrick, activeSides[nextMove]);
        }
        else if (lastDeal.get() != nullptr) {
            workerMgr.sendJob(lastDeal, workerIx);
            for (auto &msg: takenInRound) {
                workerMgr.sendJob(msg, workerIx);
            }
            if (playersConnected == 4) {
                SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo));
                workerMgr.sendJob(msgTrick, activeSides[nextMove]);
            }
        }
    }
    else {
        std::vector<Side> sidesBusy;
        for (auto entry: activeSides) sidesBusy.push_back(entry.first);
        SSendJob msgBusy = std::static_pointer_cast<SendJob>(std::make_shared<SendJobBusy>(sidesBusy));
        workerMgr.sendJob(msgBusy, workerIx);
    }
}

void Server::playerDisconnected(Side s, errInfo info) {
    if (info.errType != IO_ERR_NOERR) {
        handleSysErr(info);
    }
    MutexGuard lock(gameStateMutex);
    activeSides[s] = -1;
    playersConnected -= 1;
}

int Server::makeTCPSock(uint16_t port) {
    // todo: ivp6 handling
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("socket");
    }
    if (port) {
        sockaddr_in server_address = {AF_INET, htons(port), {htonl(INADDR_ANY)}};
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
    MutexGuard lock(gameStateMutex);
    workerMgr.spawnNewWorker<IOWorkerHandler>(
            fd,
            [this] (int ix) { this->workerQuits(ix);},
            [this] (errInfo info) { this->handleSysErr(info);},
            [this] (Side s, int ix) { this->playerIntro(s, ix);},
            [this] (int t, Side s, const Card& c) { this->playerTricked(s, c, t);},
            [this] (Side s, errInfo info) { this->playerDisconnected(s, info);},
            std::move(conn_addr),
            own_addr
            );
}

void Server::finalize() {
    workerMgr.releaseCleaner();
    workerMgr.finish();
}

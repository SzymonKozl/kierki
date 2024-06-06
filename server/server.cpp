//
// Created by szymon on 13.05.24.
//

#include "server.h"

#include <utility>
#include "io_worker_connect.h"
#include "io_worker_handler.h"
#include "../common/utils.h"
#include "game_rules.h"

#include "string"
#include "memory"
#include "functional"
#include "iostream"
#include "mutex"
#include "stdexcept"
#include "cstdint"
#include "cassert"
#include "sys/socket.h"
#include "arpa/inet.h"

Server::Server(game_scenario &&scenario, uint16_t port, int timeout):
    gameScenario(scenario),
    workerMgr([this](ErrInfo info) { this->handleSysErr(info);}),
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
    own_addr(port, ""),
    lastDeal(),
    takenInRound(),
    exiting(false),
    timeout(timeout),
    msgLogger(std::cout, false),
    workerToSide(),
    zombieWorkerToSide()
{
    for (Side s: {W, E, S, N}) {
        activeSides[s] = -1;
        hands[s] = Hand();
        penalties[s] = 0;
    }
}

int Server::run() {
    prepareRound();
    int tcp_listen_sock = makeTCPSock(own_addr.first);
    workerMgr.spawnNewWorker<IOWorkerConnect>(
        SERVING_PROXY,
        tcp_listen_sock,
        [this](ErrArr arr, int ix, bool hasWork) { return this->grandExitCallback(std::move(arr), ix, hasWork);},
        [this](int ix) {this->handleTimeout(ix);},
        [this](std::function<void()> inv) {return this->execMutexed(std::move(inv));},
        [this](int fd, net_address conn_addr) { this->forwardConnection(fd, std::move(conn_addr));},
        own_addr,
        std::ref(msgLogger)
    );
    workerMgr.waitForClearing();
    return exitCode;
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
            case TRICK_PENALTY: // to prevent compiler warnings
                ASSERT_UNREACHABLE;
                break;
            case SEVENTH_LAST_TRICK_PENALTY:
                ASSERT_UNREACHABLE;
                break;
            case EVERYTHING:
                ASSERT_UNREACHABLE;
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

bool Server::handleSysErr(const ErrInfo& info, bool locked) {
    msgLogger.logSysErr(info);
    if (locked) {
        if (info.errType == IO_ERR_INTERNAL) {
            exitCode = 1;
            finalize();
            return true;
        }
        return false;
    }
    else {
        MutexGuard lock(gameStateMutex);
        if (info.errType == IO_ERR_INTERNAL) {
            exitCode = 1;
            finalize();
            return true;
        }
        return false;
    }
}

void Server::updatePenalties() {
    for (Side s: sides_) penalties[s] += penaltiesRound[s];
}

void Server::clearTmpPenalties() {
    for (Side s: sides_) penaltiesRound[s] = 0;
}

void Server::prepareRound() {
    auto [mode, init_state, starting] = gameScenario[roundNumber];
    roundMode = mode;
    hands = init_state;
    nextMove = starting;
    trickNo = 1;
    takenInRound.clear();
}

bool Server::playerTricked(size_t trickNoArg, Card card, int workerIx) {
    MutexGuard lock(gameStateMutex);
    if (playersConnected < 4) return false;
    Side side;

    switch (workerMgr.getRole(workerIx)) {

        case SERVING_UNKNOWN: {
            workerMgr.setRole(workerIx, CLEANUP_UNKNOWN);
            workerMgr.sendKill(workerIx);
            return true;
        }
        case SERVING_ACTIVE:
            side = workerToSide.at(workerIx);
            break;
        case SERVING_PROXY: {
            // wtf
            workerMgr.setRole(workerIx, CLEANUP_UNKNOWN);
            finalize();
            return true;
        }
        case CLEANUP: {
            side = zombieWorkerToSide.at(workerIx);
            break;
        }
        case CLEANUP_UNKNOWN: {
            return true;
        }
        case SHUTDOWN:
            return true;
    }
    if (trickNoArg != trickNo || nextMove != side) {
        workerMgr.sendJob(std::static_pointer_cast<SendJob>(std::make_shared<SendJobWrong>(trickNo)), activeSides[side]);
        return true;
    }
    else if (!GameRules::isMoveLegal(side, card, hands, table)){
        workerMgr.sendJob(std::static_pointer_cast<SendJob>(std::make_shared<SendJobWrong>(trickNo)), activeSides[side]);
        return true;
    }

    table.push_back(card);
    rmCardIfPresent(hands[side], card);
    nextMove = nxtSide(nextMove);
    if (table.size() == 4ul) {
        auto [taker, penalty] = GameRules::whoTakes(nextMove, table, roundMode, trickNo);
        nextMove = taker;
        penaltiesRound[taker] += penalty;
        SSendJob msgTaken = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTaken>(table, taker, trickNo));
        trickNo ++;
        table.clear();
        for (Side s: sides_) workerMgr.sendJob(msgTaken, activeSides[s]);
        takenInRound.push_back(msgTaken);
        if (trickNo == TRICKS_PER_ROUND + 1) {
            updatePenalties();
            SSendJob msgScore = std::static_pointer_cast<SendJob>(std::make_shared<SendJobScore>(penaltiesRound));
            SSendJob msgTotal = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTotal>(penalties));
            for (Side s: sides_) {
                workerMgr.sendJob(msgScore, activeSides[s]);
                workerMgr.sendJob(msgTotal, activeSides[s]);
            }
            clearTmpPenalties();
            roundNumber ++;
            if (roundNumber == gameScenario.size()) {
                finalize();
                return true;
            }
            prepareRound();
            lastDeal = hands;
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
        }
    }
    workerMgr.sendJob(std::make_shared<SendJobTrick>(table, trickNo), activeSides[nextMove]);

    return true;
}

bool Server::playerIntro(Side side, int workerIx) {
    MutexGuard lock(gameStateMutex);
    if (workerMgr.getRole(workerIx) != SERVING_UNKNOWN) {
        if (playersConnected < 4) return false;
        workerMgr.sendKill(workerIx);
        workerMgr.setRole(workerIx, CLEANUP);
        return true;
    }
    if (activeSides[side] == -1) {
        // accepting new client
        activeSides[side] = workerIx;
        workerToSide[workerIx] = side;
        workerMgr.setRole(workerIx, SERVING_ACTIVE);
        playersConnected ++;
        if (playersConnected == 4 && lastDeal.empty()) {
            lastDeal = hands;
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
            SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo));
            workerMgr.sendJob(msgTrick, activeSides[nextMove]);
        }
        else if (!lastDeal.empty()) {
            SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, lastDeal[side]));
            workerMgr.sendJob(msgDeal, activeSides[side]);
            for (auto &msg: takenInRound) {
                workerMgr.sendJob(msg, workerIx);
            }
            if (playersConnected == 4) {
                workerMgr.signalRole(SERVING_ACTIVE);
                workerMgr.signalRole(CLEANUP_UNKNOWN);
                workerMgr.signalRole(SERVING_UNKNOWN);
                SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo));
                workerMgr.sendJob(msgTrick, activeSides[nextMove]);
            }
        }
    }
    else {
        workerMgr.setRole(workerIx, CLEANUP_UNKNOWN);
        std::vector<Side> sidesBusy;
        for (auto entry: activeSides) if (entry.second != -1) sidesBusy.push_back(entry.first);
        SSendJob msgBusy = std::static_pointer_cast<SendJob>(std::make_shared<SendJobBusy>(sidesBusy));
        workerMgr.sendJob(msgBusy, workerIx);
    }
    return true;
}

int Server::makeTCPSock(uint16_t port) {
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    int val = 0;
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &val, sizeof val)) {
        throw std::runtime_error("setsockopt");
    }
    val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof val)) {
        throw std::runtime_error("setsockopt");
    }
    if (fd < 0) {
        throw std::runtime_error("socket");
    }
    sockaddr_in6 server_address;
    server_address.sin6_family = AF_INET6;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(port);
    if (bind(fd,(const sockaddr *) &server_address, sizeof server_address)) {
        throw std::runtime_error("bind");
    }
    own_addr = getAddrStruct(fd, AF_INET6);

    if (listen(fd, TCP_QUEUE)) {
        throw std::runtime_error("listen");
    }

    return fd;
}

void Server::forwardConnection(int fd, net_address conn_addr) {
    MutexGuard lock(gameStateMutex);
    setTimeout(fd, timeout);
    workerMgr.spawnNewWorker<IOWorkerHandler>(
            SERVING_UNKNOWN,
            fd,
            [this] (ErrArr errs, int ix, bool hasWork) { return this->grandExitCallback(std::move(errs), ix, hasWork);},
            [this] (int ix) {this->handleTimeout(ix);},
            [this] (std::function<void()> inv) {return this->execMutexed(std::move(inv));},
            [this] (Side s, int ix) { return this->playerIntro(s, ix);},
            [this] (int t, const Card& c, int ix) { return this->playerTricked(t, c, ix);},
            [this] (int ix) {return this->handleWrongMessage(ix);},
            std::move(conn_addr),
            own_addr,
            timeout,
            std::ref(msgLogger)
            );
}

void Server::finalize() {
    if (exiting) return;
    exiting = true;
    workerMgr.releaseCleaner();
    workerMgr.finish();
}

bool Server::grandExitCallback(ErrArr errArr, int workerIx, bool hasWork) {
    MutexGuard lock(gameStateMutex);
    for (ErrInfo const& err: errArr) {
        if (handleSysErr(err, true)) {
            return true;
        }
    }

    switch (workerMgr.getRole(workerIx)) {

        case SERVING_UNKNOWN: {
            workerMgr.eraseWorker(workerIx);
            workerMgr.setRole(workerIx, SHUTDOWN);
            return true;
        }
        case SERVING_ACTIVE: {
            Side s = workerToSide.at(workerIx);
            playersConnected --;
            activeSides[s] = -1;
            zombieWorkerToSide[workerIx] = s;
            workerToSide.erase(workerIx);
            if (hasWork || (playersConnected < 3 && !exiting)) {
                workerMgr.setRole(workerIx, CLEANUP);
                return false;
            }
            else {
                workerMgr.setRole(workerIx, SHUTDOWN);
                workerMgr.eraseWorker(workerIx);
                return true;
            }
        }
        case CLEANUP: {
            if (playersConnected == 4 && !hasWork) {
                workerMgr.eraseWorker(workerIx);
                workerMgr.setRole(workerIx, SHUTDOWN);
                return true;
            }
            return false;
        }
        case SHUTDOWN: {
            return true;
        }
        case SERVING_PROXY: {
            workerMgr.setRole(workerIx, SHUTDOWN);
            workerMgr.eraseWorker(workerIx);
            finalize();
            return true;
        }
        case CLEANUP_UNKNOWN: {
            if (playersConnected == 4 && !hasWork) {
                workerMgr.eraseWorker(workerIx);
                workerMgr.setRole(workerIx, SHUTDOWN);
                return true;
            }
            return false;
        }
    }
    ASSERT_UNREACHABLE;
    return true;
}

bool Server::execMutexed(std::function<void()>&& invokable) {
    MutexGuard lock(gameStateMutex);
    if (playersConnected < 4 && !exiting) return false;
    invokable();
    return true;
}

void Server::handleTimeout(int workerIx) {
    MutexGuard lock(gameStateMutex);
    if (exiting) return;
    if (workerMgr.getRole(workerIx) == SERVING_UNKNOWN) {
        workerMgr.sendKill(workerIx);
        return;
    }
    else if (workerMgr.getRole(workerIx) == SERVING_ACTIVE) {
        if (nextMove != workerToSide.at(workerIx)) return;
        if (playersConnected < 4) {
            return;
        }
        SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo));
        workerMgr.sendJob(msgTrick, workerIx);
    }
}

bool Server::handleWrongMessage(int ix) {
    MutexGuard lock(gameStateMutex);
    if (playersConnected < 4 && !exiting) {
        return false;
    }
    workerMgr.sendKill(ix);
    return true;
}

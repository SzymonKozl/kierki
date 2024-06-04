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

Server::Server(game_scenario &&scenario, uint16_t port, int timeout):
    gameScenario(scenario),
    workerMgr([&](ErrInfo info) { handleSysErr(std::move(info));}),
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
    zombieWorkerToSide(),
    zombieWorkers(),
    sendingBusy(),
    expectedTrickResponse(false)
{
    for (Side s: {W, E, S, N}) {
        activeSides[s] = -1;
        hands[s] = Hand();
        penalties[s] = 0;
    }
}

void Server::run() {
    prepareRound();
    int tcp_listen_sock = makeTCPSock(own_addr.first);
    workerMgr.spawnNewWorker<IOWorkerConnect>(
        INCOMING_PROXY,
        tcp_listen_sock,
        [this](ErrArr arr, int ix, bool hasWork) { return this->grandExitCallback(arr, ix, hasWork);},
        [this](int ix) {this->workerMgr.clearPipes(ix);},
        [this](int ix) {this->handleTimeout(ix);},
        [this](std::function<void()> inv) {return this->execMutexed(std::move(inv));},
        [this](int fd, net_address conn_addr) { this->forwardConnection(fd, std::move(conn_addr));},
        own_addr,
        std::ref(msgLogger)
    );
    workerMgr.waitForClearing();
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

void Server::handleSysErr(ErrInfo info) {
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
    auto [mode, init_state, starting] = gameScenario[roundNumber];
    roundMode = mode;
    hands = init_state;
    nextMove = starting;
    trickNo = 1;
    takenInRound.clear();
}

bool Server::playerTricked(int trickNoArg, Card card, int workerIx) {
    MutexGuard lock(gameStateMutex);
    Side side;
    if (workerToSide.find(workerIx) != workerToSide.end() ) {
        side = workerToSide.at(workerIx);
    }
    else if (zombieWorkerToSide.find(workerIx) != zombieWorkerToSide.end()) {
        side = zombieWorkerToSide.at(workerIx);
    }
    else if (trickNoArg == -1) {
        side = SIDE_NULL_;
    }
    else {
        workerMgr.sendKill(workerIx);
        return true;
    }
    if (playersConnected < 4) {
        return false;
    }
    else if (side == SIDE_NULL_) {
        workerMgr.sendKill(workerIx);
        return true;
    }
    if (trickNoArg != trickNo || nextMove != side) {
        workerMgr.sendJob(std::static_pointer_cast<SendJob>(std::make_shared<SendJobWrong>(trickNo, true)), activeSides[side]);
        return true;
    }
    else if (!GameRules::isMoveLegal(side, card, hands, table)){
        workerMgr.sendJob(std::static_pointer_cast<SendJob>(std::make_shared<SendJobWrong>(trickNo, false)), activeSides[side]);
        expectedTrickResponse = true;
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
    workerMgr.sendJob(std::make_shared<SendJobTrick>(table, trickNo, true), activeSides[nextMove]);
    expectedTrickResponse = true;

    return true;
}

void Server::playerIntro(Side side, int workerIx) {
    MutexGuard lock(gameStateMutex);
    if (exiting) return;
    allIntroduced.insert(workerIx);
    if (activeSides[side] == -1) {
        // accepting new client
        activeSides[side] = workerIx;
        workerToSide[workerIx] = side;
        workerMgr.setRole(workerIx, HANDLING_ACTIVE);
        playersConnected ++;
        if (playersConnected == 4 && lastDeal.empty()) {
            lastDeal = hands;
            for (Side s: sides_) {
                SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, hands[s]));
                workerMgr.sendJob(msgDeal, activeSides[s]);
            }
            SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo, true));
            workerMgr.sendJob(msgTrick, activeSides[nextMove]);
            expectedTrickResponse = true;
        }
        else if (!lastDeal.empty()) {
            SSendJob msgDeal = std::static_pointer_cast<SendJob>(std::make_shared<SendDealJob>(roundMode, nextMove, lastDeal[side]));
            workerMgr.sendJob(msgDeal, activeSides[side]);
            for (auto &msg: takenInRound) {
                workerMgr.sendJob(msg, workerIx);
            }
            if (playersConnected == 4) {
                for (Side s: sides_) {
                    workerMgr.signal(activeSides[s]);
                }
                for (int ix: sendingBusy) {
                    workerMgr.signal(ix);
                }
                SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo, true));
                workerMgr.sendJob(msgTrick, activeSides[nextMove]);
            }
        }
    }
    else {
        workerMgr.setRole(workerIx, HANDLING_DISCARDED);
        std::vector<Side> sidesBusy;
        for (auto entry: activeSides) if (entry.second != -1) sidesBusy.push_back(entry.first);
        SSendJob msgBusy = std::static_pointer_cast<SendJob>(std::make_shared<SendJobBusy>(sidesBusy));
        workerMgr.sendJob(msgBusy, workerIx);
        sendingBusy.insert(workerIx);
    }
}

void Server::playerDisconnected(Side s, ErrInfo info) {
    if (info.errType != IO_ERR_NOERR) {
        handleSysErr(info);
    }
    MutexGuard lock(gameStateMutex);
    activeSides[s] = -1;
    playersConnected -= 1;
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
            HANDLING_UNKNOWN,
            fd,
            [this] (ErrArr errs, int ix, bool hasWork) { return this->grandExitCallback(std::move(errs), ix, hasWork);},
            [this] (int ix) {this->workerMgr.clearPipes(ix);},
            [this] (int ix) {this->handleTimeout(ix);},
            [this] (std::function<void()> inv) {return this->execMutexed(std::move(inv));},
            [this] (Side s, int ix) { this->playerIntro(s, ix);},
            [this] (int t, const Card& c, int ix) { return this->playerTricked(t, c, ix);},
            [this] (std::string msg, int ix) {return this->handleWrongMessage(std::move(msg), ix);},
            std::move(conn_addr),
            own_addr,
            timeout,
            std::ref(msgLogger)
            );
}

void Server::finalize() {
    exiting = true;
    workerMgr.releaseCleaner();
    workerMgr.finish();
}

bool Server::grandExitCallback(ErrArr errArr, int workerIx, bool hasWork) {
    MutexGuard lock(gameStateMutex);
    if (exiting) {
        workerMgr.eraseWorker(workerIx);
        return true;
    }
    else {
        for (ErrInfo const& err: errArr) {
            auto [call, error, type] = err;
            std::cerr << "System error on " << call << " call! Error code: " << error << std::endl;
            if (type == IO_ERR_INTERNAL) {
                exiting = true;
                exitCode = 1;
                std::cerr << "internal error! quitting server";
                finalize();
                return true;
            }
        }
        if (workerToSide.find(workerIx) != workerToSide.end()) {
            Side s = workerToSide.at(workerIx);
            if (s == nextMove) {
                expectedTrickResponse = false;
            }
            activeSides[s] = -1;
            playersConnected --;
            workerToSide.erase(workerIx);
            if (!hasWork) {
                workerMgr.eraseWorker(workerIx);
                return true;
            }
            zombieWorkerToSide[workerIx] = s;
            zombieWorkers.insert(workerIx);
            return false;
        }
        else {
            if (workerIx == 0) { // todo: make implementation independent
                exiting = true;
                exitCode = 1;
                finalize();
                return true;
            }
            if (zombieWorkers.find(workerIx) == zombieWorkers.end()) {
                workerMgr.eraseWorker(workerIx);
                return true;
            }
            if (!hasWork) {
                zombieWorkers.erase(workerIx);
                zombieWorkerToSide.erase(workerIx);
                workerMgr.eraseWorker(workerIx);
                return true;
            }
        }
    }
}

bool Server::execMutexed(std::function<void()> invokable) {
    MutexGuard lock(gameStateMutex);
    if (playersConnected < 4 && !exiting) return false;
    invokable();
    return true;
}

void Server::handleTimeout(int workerIx) {
    MutexGuard lock(gameStateMutex);
    if (exiting) return;
    if (workerToSide.find(workerIx) == workerToSide.end()) {
        if (allIntroduced.find(workerIx) == allIntroduced.end()) {
            workerMgr.sendKill(workerIx);
            return;
        }
    }
    if (nextMove != workerToSide.at(workerIx)) return;
    if (playersConnected < 4) {
        expectedTrickResponse = false;
        return;
    }
    SSendJob msgTrick = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(table, trickNo, true));
    workerMgr.sendJob(msgTrick, workerIx);
    expectedTrickResponse = true;
}

bool Server::handleWrongMessage(std::string message, int ix) {
    MutexGuard lock(gameStateMutex);
    if (playersConnected < 4 && !exiting) {
        return false;
    }
    return true;
}

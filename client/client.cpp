//
// Created by szymon on 16.05.24.
//

#include "client.h"

#include <utility>
#include "player.h"
#include "../common/utils.h"
#include "../common/network_msg_parser.h"

#include "memory"
#include "stdexcept"
#include "algorithm"
#include "unistd.h"
#include "cstdlib"
#include "poll.h"
#include "fcntl.h"
#include "arpa/inet.h"

constexpr char MSG_SEP = '\n';

enum GameStage {
    PRE,
    AFTER_FIRST_DEAL,
    AFTER_DEAL,
    AFTER_TRICK,
    AFTER_SCORE,
    AFTER_TOTAL
};

Client::Client(
        Player &player,
        NetAddress connectTo,
        Side side,
        sa_family_t proto
):
        tcpSock(-1),
        player(player),
        ownAddr(),
        serverAddr(std::move(connectTo)),
        waitingForCard(false),
        selectedCard("3", COLOR_H),
        side(side),
        trickNo(1),
        proto(proto),
        exitFlag(0),
        openSock(false)
{}

int Client::run() {
    GameStage stage = PRE;
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    std::string nextCmd;
    std::string nextMsg;

    sockaddrAny addr = getIntAddr(
            serverAddr.second, proto, htons(serverAddr.first)
            );
    socklen_t len = (addr.family == AF_INET) ?
            sizeof(sockaddr_in) :
            sizeof(sockaddr_in6);
    proto = addr.family;

    tcpSock = makeConnection(proto);
    openSock = true;

    ownAddr = getAddrStruct(tcpSock, proto);

    if (connect(
            tcpSock, reinterpret_cast<sockaddr *>(&addr.addr), len)) {
        throw std::runtime_error("connect");
    }

    SSendJob msgIam = std::static_pointer_cast<SendJob>(
            std::make_shared<SendJobIntro>(side));
    sendMessage(msgIam);
    pollfd pollFds[] {
            {tcpSock, POLLIN, 0},
            {0,       POLLIN, 0}
    };

    bool terminate = false;
    while (!terminate) {
        pollFds[0].revents = 0;
        pollFds[1].revents = 0;
        int poll_status = poll(pollFds, 2, -1);
        if (poll_status < 0) {
            throw std::runtime_error("poll");
        }
        else {
            if (pollFds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw std::runtime_error("poll on stdin");
            }
            else if (pollFds[1].revents & POLLIN) {
                char c;
                ssize_t r;
                while ((r = read(0, &c, 1)) == 1) {
                    if (c == MSG_SEP) {
                        player.anyCmd(nextCmd);
                        nextCmd = "";
                    }
                    else nextCmd += c;
                }
                if (r == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        throw std::runtime_error("read on stdin");
                    }
                }
            }
            else if (pollFds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw std::runtime_error("poll on tcp sock");
            }
            else if (pollFds[0].revents & POLLIN) {
                char c;
                ssize_t r;
                std::string msg;
                while ((r = recv(
                        tcpSock, &c, 1, MSG_DONTWAIT)) == 1) {
                    nextMsg += c;
                    if (c == '\n') { // new msg
                        msg = nextMsg.substr(0, nextMsg.size() - 2);
                        nextMsg.clear();

                        Message msgObj(
                                serverAddr, ownAddr, msg);
                        player.anyMsg(msgObj);
                        ParseResp msgArray = parseNetMsg(msg, false);
                        if (msgArray[0].second == "DEAL") {
                            stage = (stage == PRE) ?
                                    AFTER_FIRST_DEAL : AFTER_DEAL;
                            auto type = static_cast<RoundType>(
                                    msgArray[1].second.at(0) - '0');
                            Hand hand;
                            for (int i = 3; i < 16; i ++)
                                hand.push_back(Card::fromString(
                                        msgArray[i].second));
                            Side starting = static_cast<Side>(
                                    msgArray[2].second.at(0));
                            player.dealMsg(type, hand, starting);
                        }
                        else if (msgArray[0].second == "TRICK_S") {
                            stage = AFTER_TRICK;
                            waitingForCard = true;
                            trickNo = atoi(msgArray[1].second.c_str());
                            Table  t;
                            for (size_t i = 2; i < msgArray.size(); i ++) {
                                t.push_back(Card::fromString(
                                        msgArray[i].second));
                            }
                            player.trickMsg(trickNo, t);
                        }
                        else if (msgArray[0].second == "TAKEN") {
                            trickNo = atoi(msgArray[1].second.c_str());
                            Side s = static_cast<Side>(
                                    msgArray[msgArray.size() - 1].second.at(
                                            0));
                            Table t;
                            for (int i = 2 ; i < 6; i++)
                                t.push_back(Card::fromString(
                                        msgArray[i].second));
                            player.takenMsg(
                                    s, t, trickNo, stage == AFTER_FIRST_DEAL);
                        }
                        else if (msgArray[0].second == "SCORE"
                                || msgArray[0].second == "TOTAL") {
                            if (msgArray[0].second == "SCORE" )
                                stage = AFTER_SCORE;
                            else stage = AFTER_TOTAL;
                            bool total = msgArray[0].second == "TOTAL";
                            score_map res;
                            for (int i = 1; i <= 4; i ++) {
                                Side s = static_cast<Side>(
                                        msgArray[2 * i - 1].second.at(0));
                                int score = atoi(
                                        msgArray[2 * i].second.c_str());
                                res[s] = score;
                            }
                            player.scoreMsg(res, total);
                        }
                        else if (msgArray[0].second == "WRONG") {
                            trickNo = atoi(msgArray[1].second.c_str());
                            waitingForCard = true;
                            player.wrongMsg(trickNo);
                        }
                        else if (msgArray[0].second == "BUSY") {
                            std::vector<Side> taken;
                            for (size_t j = 1; j < msgArray.size(); j ++) {
                                taken.push_back(static_cast<Side>(
                                        msgArray[j].second.at(0)));
                            }
                            player.busyMsg(taken);
                            exitFlag = 1;
                            terminate = true;
                            break;
                        }
                    }
                }
                if (r < 0) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        throw std::runtime_error("read on tcp sock");
                    }
                }
                else if (!r) {
                    exitFlag = stage != AFTER_TOTAL;
                    terminate = true;
                }
            }
        }
    }
    close(tcpSock);
    openSock = false;
    return exitFlag;
}

int Client::makeConnection(sa_family_t proto) {
    int fd = socket(proto, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket");
    if (proto == AF_INET6) {
        sockaddr_in6 serverAddr{};
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_addr = in6addr_any;
        serverAddr.sin6_port = htons(0);
        if (bind(fd, (const sockaddr *) &serverAddr, sizeof serverAddr)) {
            throw std::runtime_error("bind");
        }
    }
    else {
        sockaddr_in serverAddr{AF_INET, htons(0), {INADDR_ANY}, {}};
        if (bind(fd, (const sockaddr *) &serverAddr, sizeof serverAddr)) {
            throw std::runtime_error("bind");
        }
    }
    return fd;
}

void Client::chooseCard(const Card& c) {
    selectedCard = Card(c.getValue(), c.getColor());
    if (waitingForCard) {
        Table hand;
        hand.push_back(selectedCard);
        SSendJob msg = std::static_pointer_cast<SendJob>(
                std::make_shared<SendJobTrick>(hand, trickNo));
        sendMessage(msg);
        waitingForCard = false;
    }
}

void Client::sendMessage(const SSendJob& job) const {
    std::string msg = job->genMsg();
    if (sendNoBlockN(tcpSock, reinterpret_cast<const void *>(msg.c_str()),
                     static_cast<ssize_t>(msg.size())) <
                     static_cast<ssize_t>(msg.size())) {
        throw std::runtime_error("send");
    }
    player.anyMsg(Message(ownAddr, serverAddr, msg.substr(0, msg.size() - 2)));
}

bool Client::isWaitingForCard() const noexcept {
    return waitingForCard;
}

void Client::cleanup() const {
    if (openSock) {
        if (close(tcpSock)) {
            throw std::runtime_error("close");
        }
    }
}

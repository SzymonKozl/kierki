//
// Created by szymon on 16.05.24.
//

#include "client.h"

#include <memory>
#include <utility>
#include "player.h"
#include "utils.h"
#include "message.h"
#include "network_msg_parser.h"
#include "send_job.h"

#include "memory"
#include "utility"
#include "stdexcept"
#include "iostream"
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

int Client::run() {
    GameStage stage = PRE;
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    std::string nextCmd;
    std::string nextMsg;

    sockaddr_any addr = getIntAddr(serverAddr.second, proto, htons(serverAddr.first));
    sockaddr* sockAddr = (addr.family == AF_INET) ? (sockaddr *)addr.addr.addr_in : (sockaddr *)addr.addr.addr_in6;
    socklen_t len = (addr.family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

    proto = sockAddr->sa_family;

    tcp_sock = makeConnection(proto);
    //fcntl(tcp_sock, F_SETFL, fcntl(tcp_sock, F_GETFL) | O_NONBLOCK);

    ownAddr = getAddrStruct(tcp_sock, proto);

    if (connect(tcp_sock, sockAddr, len)) {
        throw std::runtime_error("connect");
    }

    if (addr.family == AF_INET) {
        delete (addr.addr.addr_in);
    }
    else {
        delete (addr.addr.addr_in6);
    }
    SSendJob msgIam = std::static_pointer_cast<SendJob>(std::make_shared<SendJobIntro>(side));
    sendMessage(msgIam);
    pollfd poll_fds[] {
            {tcp_sock, POLLIN, 0},
            {0, POLLIN, 0}
    };

    bool terminate = false;
    while (!terminate) {
        poll_fds[0].revents = 0;
        poll_fds[1].revents = 0;
        int poll_status = poll(poll_fds, 2, -1);
        if (poll_status < 0) {
            throw std::runtime_error("poll");
        }
        else {
            if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw std::runtime_error("poll on stdin");
            }
            else if (poll_fds[1].revents & POLLIN) {
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
            else if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw std::runtime_error("poll on tcp sock");
            }
            else if (poll_fds[0].revents & POLLIN) {
                char c;
                ssize_t r;
                std::string msg;
                while ((r = recv(tcp_sock, &c, 1, MSG_DONTWAIT)) == 1) {
                    nextMsg += c;
                    if (c == '\n') { // new msg
                        msg = nextMsg.substr(0, nextMsg.size() - 2);
                        nextMsg.clear();

                        Message msgObj(serverAddr, ownAddr, msg);
                        player.anyMsg(msgObj);
                        resp_array msg_array = parse_msg(msg, false);
                        if (msg_array[0].second == "DEAL") {
                            stage = (stage == PRE) ? AFTER_FIRST_DEAL : AFTER_DEAL;
                            auto type = (RoundType) (msg_array[1].second.at(0) - '0');
                            Hand hand;
                            for (int i = 3; i < 16; i ++) hand.push_back(Card::fromString(msg_array[i].second));
                            Side starting = (Side) msg_array[2].second.at(0);
                            player.dealMsg(type, hand, starting);
                        }
                        else if (msg_array[0].second == "TRICK_S") {
                            stage = AFTER_TRICK;
                            waitingForCard = true;
                            trickNo = atoi(msg_array[1].second.c_str());
                            Table  t;
                            for (size_t i = 2; i < msg_array.size(); i ++) {
                                t.push_back(Card::fromString(msg_array[i].second));
                            }
                            player.trickMsg(trickNo, t);
                        }
                        else if (msg_array[0].second == "TAKEN") {
                            trickNo = atoi(msg_array[1].second.c_str());
                            Side s = (Side) msg_array[msg_array.size() - 1].second.at(0);
                            Table t;
                            for (int i = 2 ; i < 6; i++) t.push_back(Card::fromString(msg_array[i].second));
                            player.takenMsg(s, t, trickNo, stage == AFTER_FIRST_DEAL);
                        }
                        else if (msg_array[0].second == "SCORE" || msg_array[0].second == "TOTAL") {
                            if (msg_array[0].second == "SCORE" ) stage = AFTER_SCORE;
                            else stage = AFTER_TOTAL;
                            bool total = msg_array[0].second == "TOTAL";
                            score_map res;
                            for (int i = 1; i <= 4; i ++) {
                                Side s = (Side) msg_array[2 * i - 1].second.at(0);
                                int score = atoi(msg_array[2 * i].second.c_str());
                                res[s] = score;
                            }
                            player.scoreMsg(res, total);
                        }
                        else if (msg_array[0].second == "WRONG") {
                            trickNo = atoi(msg_array[1].second.c_str());
                            waitingForCard = true;
                            player.wrongMsg(trickNo);
                        }
                        else if (msg_array[0].second == "BUSY") {
                            std::vector<Side> taken;
                            for (size_t j = 1; j < msg_array.size(); j ++) {
                                taken.push_back((Side) msg_array[j].second.at(0));
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
    close(tcp_sock);
    return exitFlag;
}

int Client::makeConnection(sa_family_t proto) {
    int fd = socket(proto, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket");
    if (proto == AF_INET6) {
        sockaddr_in6 server_address;
        server_address.sin6_family = AF_INET6;
        server_address.sin6_addr = in6addr_any;
        server_address.sin6_port = htons(0);
        if (bind(fd,(const sockaddr *) &server_address, sizeof server_address)) {
            throw std::runtime_error("bind");
        }
    }
    else {
        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(0);
        if (bind(fd,(const sockaddr *) &server_address, sizeof server_address)) {
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
        SSendJob msg = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(hand, trickNo, false));
        sendMessage(msg);
        waitingForCard = false;
    }
}

Client::Client(Player &player, net_address connectTo, Side side, sa_family_t proto):
        tcp_sock(-1),
        player(player),
        ownAddr(),
        serverAddr(std::move(connectTo)),
        waitingForCard(false),
        selectedCard("3", COLOR_H),
        side(side),
        trickNo(1),
        proto(proto),
        exitFlag(0)
{}

void Client::sendMessage(const SSendJob& job) const {
    std::string msg = job->genMsg();
    if (writeN(tcp_sock, (void *) msg.c_str(), msg.size()) < msg.size()) {
        throw std::runtime_error("write");
    }
    player.anyMsg(Message(ownAddr, serverAddr, msg.substr(0, msg.size() - 2)));
}

bool Client::isWaitingForCard() const noexcept {
    return waitingForCard;
}

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

#include "stdexcept"
#include "iostream"
#include "algorithm"
#include "unistd.h"
#include "cstdlib"
#include "poll.h"
#include "fcntl.h"
#include "arpa/inet.h"

constexpr char MSG_SEP = '\r';

enum GameStage {
    PRE,
    AFTER_FIRST_DEAL,
    AFTER_DEAL,
    AFTER_TRICK,
    AFTER_SCORE,
    AFTER_TOTAL
};

void Client::run() {
    GameStage stage = PRE;
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    std::string nextCmd;
    try {
        tcp_sock = makeConnection();
    } catch (std::runtime_error &e) {
        std::cerr << "Error on" << e.what();
        exit(1);
    }
    sockaddr_in addr = {AF_INET, htons(serverAddr.first), {getIntAddr(serverAddr.second)}};

    ownAddr = getAddrStruct(tcp_sock);

    if (connect(tcp_sock, (sockaddr *) &addr, sizeof addr)) {
        throw std::runtime_error("connect");
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
            terminate = true;
            std::cerr << "poll";
        }
        else {
            if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                terminate = true;
                std::cerr << "poll - stdin";
            }
            else if (poll_fds[1].revents & POLLIN) {
                char c;
                while (read(tcp_sock, &c, 1)) {
                    if (c == MSG_SEP) {
                        player.anyCmd(nextCmd);
                        nextCmd = "";
                    }
                    else nextCmd += c;
                }

            }
            else if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                terminate = true;
                std::cerr << "poll - tcp";
            }
            else if (poll_fds[0].revents & POLLIN) {
                std::string msg;
                try {
                    msg = readUntilRN(tcp_sock);
                } catch (std::runtime_error &e) {
                    if (errno == 0 && stage == AFTER_TOTAL) {
                        exit(0);
                    }
                    else {
                        exit(1);
                    }
                }
                Message msgObj(serverAddr, ownAddr, msg);
                player.anyMsg(msgObj);
                resp_array msg_array = parse_msg(msg, false);
                if (msg_array[0].second == "DEAL") {
                    stage = (stage == PRE) ? AFTER_FIRST_DEAL : AFTER_DEAL;
                    auto type = (RoundType) (msg_array[1].second.at(0) - '0');
                    Hand hand;
                    for (int i = 3; i < 16; i ++) hand.push_back(Card::fromString(msg_array[i].second));
                    player.dealMsg(type, hand);
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
                    Side s = (Side) msg_array[1].second.at(0);
                    Table t;
                    for (int i = 2 ; i < 6; i++) t.push_back(Card::fromString(msg_array[i].second));
                    player.takenMsg(s, t, stage == AFTER_FIRST_DEAL);
                }
                else if (msg_array[0].second == "SCORE" || msg_array[0].second == "TOTAL") {
                    if (msg_array[0].second == "SCORE" ) stage = AFTER_SCORE;
                    else stage = AFTER_TOTAL;
                    bool total = msg_array[0].second == "TOTAL";
                    score_map res;
                    for (int i = 1; i <= 4; i ++) {
                        Side side = (Side) msg_array[2 * i - 1].second.at(0);
                        int score = atoi(msg_array[2 * i].second.c_str());
                        res[side] = score;
                    }
                    player.scoreMsg(res, total);
                }
                else if (msg_array[0].second == "WRONG") {
                    trickNo = atoi(msg_array[1].second.c_str());
                    player.wrongMsg(trickNo);
                }
            }
        }
    }
}

int Client::makeConnection() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket");
    return fd;
}

void Client::chooseCard(const Card& c) {
    selectedCard = Card(c.getValue(), c.getColor());
    if (waitingForCard) {
        Table h;
        h.push_back(selectedCard);
        SSendJob msg = std::static_pointer_cast<SendJob>(std::make_shared<SendJobTrick>(h, trickNo));
        sendMessage(msg);
        waitingForCard = false;
    }
}

Client::Client(Player &player, net_address connectTo, Side side):
        tcp_sock(-1),
        player(player),
        ownAddr(),
        serverAddr(std::move(connectTo)),
        waitingForCard(false),
        selectedCard("3", COLOR_H),
        lastGivenCard("3", COLOR_H),
        side(side),
        trickNo(1)
{}

void Client::sendMessage(const SSendJob& job) const {
    std::string msg = job->genMsg();
    std::cout << "sending " << msg << std::endl;
    if (writeN(tcp_sock, (void *) msg.c_str(), msg.size()) < msg.size()) {
        throw std::runtime_error("write");
    }
}

bool Client::isWaitingForCard() const noexcept {
    return waitingForCard;
}

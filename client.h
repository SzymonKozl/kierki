//
// Created by szymon on 16.05.24.
//

#ifndef KIERKI_CLIENT_H
#define KIERKI_CLIENT_H

#include "player.h"
#include "io_worker_mgr.h"
#include "common_types.h"
#include "memory"

class Client {
public:
    Client(Player &player, net_address connectTo, Side side);
    void run();
    void chooseCard(const Card& c);
    void sendMessage(const SSendJob& job) const;
    bool isWaitingForCard() const noexcept;
private:
    static int makeConnection();

    int tcp_sock;
    Player &player;
    net_address ownAddr;
    net_address serverAddr;
    bool waitingForCard;
    Card selectedCard;
    Card lastGivenCard;
    Side side;
    int trickNo;
};

#endif //KIERKI_CLIENT_H

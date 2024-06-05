//
// Created by szymon on 16.05.24.
//

#ifndef KIERKI_CLIENT_H
#define KIERKI_CLIENT_H

#include "player.h"
#include "io_worker_mgr.h"
#include "common_types.h"
#include "memory"
#include "arpa/inet.h"

class Client {
public:
    Client(Player &player, net_address connectTo, Side side, sa_family_t proto);
    int run();
    void chooseCard(const Card& c);
    void sendMessage(const SSendJob& job) const;
    bool isWaitingForCard() const noexcept;
    void cleanup();
private:
    static int makeConnection(sa_family_t proto);

    int tcp_sock;
    Player &player;
    net_address ownAddr;
    net_address serverAddr;
    bool waitingForCard;
    Card selectedCard;\
    Side side;
    int trickNo;
    sa_family_t proto;
    int exitFlag;
    bool openSock;
};

#endif //KIERKI_CLIENT_H

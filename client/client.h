//
// Created by szymon on 16.05.24.
//

#ifndef KIERKI_CLIENT_H
#define KIERKI_CLIENT_H

#include "player.h"
#include "../common/common_types.h"
#include "memory"
#include "arpa/inet.h"
#include "../common/send_job.h"

class Client {
public:
    Client(Player &player, NetAddress connectTo, Side side, sa_family_t proto);
    int run();
    void chooseCard(const Card& c);
    void sendMessage(const SSendJob& job) const;
    bool isWaitingForCard() const noexcept;
    void cleanup() const;
private:
    static int makeConnection(sa_family_t proto);

    int tcpSock;
    Player &player;
    NetAddress ownAddr;
    NetAddress serverAddr;
    bool waitingForCard;
    Card selectedCard;
    Side side;
    int trickNo;
    sa_family_t proto;
    int exitFlag;
    bool openSock;
};

#endif //KIERKI_CLIENT_H

//
// Created by szymon on 15.05.24.
//


#include "io_worker_mgr.h"
#include "player_auto.h"
#include "common_types.h"
#include "client.h"
#include "player_auto.h"

#include "iostream"

int main(int argc, char* argv[]) {
    PlayerAuto player;
    Side s = W;
    if (argc == 2) {
        s = (Side) argv[1][0];
    }
    Client clientObj(player, (net_address)std::make_pair(9009, "127.0.0.1"), s);
    player.setup(
            [&clientObj](const Card& c) {clientObj.chooseCard(c);},
            [&clientObj]() {return clientObj.isWaitingForCard();}
            );
    clientObj.run();
}

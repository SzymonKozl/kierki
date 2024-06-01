//
// Created by szymon on 15.05.24.
//


#include "utils.h"
#include "player_auto.h"
#include "common_types.h"
#include "client.h"
#include "player_auto.h"
#include "player_console.h"

#include "iostream"
#include "cstdint"
#include "arpa/inet.h"
#include "boost/program_options.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    uint16_t port = 0;
    std::string host = "";
    Side side = SIDE_NULL_;
    sa_family_t proto = AF_UNSPEC;
    bool autoPlayer = false;
    int itr = 1;
    bool argErr = false;
    while (itr < argc) {
        if (strcmp(argv[itr], "-a") == 0) {
            autoPlayer = true;
            itr ++;
        }
        else if (strcmp(argv[itr], "-h") == 0) {
            if (itr + 1 == argc || argv[itr + 1][0] == '-') {
                std::cerr << "-h option requires value\n";
                break;
            }
            host = argv[itr + 1];
            itr += 2;
        }
        else if (strcmp(argv[itr], "-p") == 0) {
            if (itr + 1 == argc || argv[itr + 1][0] == '-') {
                std::cerr << "-p option requires value\n";
                argErr = true;
                break;
            }
            port = (int) atoi(argv[itr + 1]);
            if (!port) {
                std::cerr << argv[itr + 1] << "is not a valid port number\n";
                argErr = true;
                break;
            }
            itr += 2;
        }
        else if (strcmp(argv[itr], "-4") == 0) {
            proto = AF_INET;
            itr ++;
        }
        else if (strcmp(argv[itr], "-6") == 0) {
            proto = AF_INET6;
            itr ++;
        }
        else if (strcmp(argv[itr], "-N") == 0) {
            side = N;
            itr ++;
        }
        else if (strcmp(argv[itr], "-E") == 0) {
            side = E;
            itr ++;
        }
        else if (strcmp(argv[itr], "-S") == 0) {
            side = S;
            itr ++;
        }
        else if (strcmp(argv[itr], "-W") == 0) {
            side = W;
            itr ++;
        }
        else {
            std::cerr << "unknown option or value for option that takes no value: " << argv[itr] << "\n";
        }
    }
    if (port == 0) {
        std::cerr << "-p <port> has to be specified\n";
        argErr = true;
    }
    if (host.empty()) {
        std::cerr << "-h <host> has to be specified\n";
        argErr = true;
    }
    if (side == SIDE_NULL_) {
        std::cerr << "side has to be specified via -N, -S, -E or -W\n";
    }
    if (argErr) {
        std::cerr << "usage: kierki-client -h <host> -p <port> [-S] [-N] [-E] [-W] [-a] [-6] [-4]";
        exit(1);
    }
    ignoreBrokenPipe();
    if (autoPlayer) {
        PlayerAuto player;
        Client clientObj(player, (net_address) std::make_pair(port, host), side, proto);
        player.setup(
                [&clientObj](const Card &c) { clientObj.chooseCard(c); },
                [&clientObj]() { return clientObj.isWaitingForCard(); }
        );
        try {
            exit(clientObj.run());
        } catch (std::runtime_error &e) {
            std::cerr << "client error on system call: " << e.what() << "!. errno: " << errno << std::endl;
            exit(1);
        }
    }
    else {
        PlayerConsole player;
        Client clientObj(player, (net_address) std::make_pair(port, host), side, proto);
        player.setup(
                [&clientObj](const Card &c) { clientObj.chooseCard(c); },
                [&clientObj]() { return clientObj.isWaitingForCard(); }
        );
        try {
            exit(clientObj.run());
        } catch (std::runtime_error &e) {
            std::cerr << "client error on system call: " << e.what() << "!. errno: " << errno << std::endl;
            exit(1);
        }
    }
}

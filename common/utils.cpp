//
// Created by szymon on 14.05.24.
//

#include "utils.h"
#include "constants.h"
#include "../server/game_rules.h"
#include "common_types.h"

#include "fstream"
#include "iostream"
#include "string"
#include "vector"
#include "unistd.h"
#include "sys/time.h"
#include "arpa/inet.h"
#include "netdb.h"
#include "stdexcept"
#include "cstring"
#include "csignal"

ssize_t sendNoBlockN(int fd, const void * buff, ssize_t n) {
    char* buff_c = reinterpret_cast<char *>(const_cast<void*>(buff));
    ssize_t _written = 0;
    while (_written < n) {
        errno = 0;
        ssize_t t = send(fd, buff_c + _written, n - _written, MSG_DONTWAIT);
        if (t != n - _written) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return _written + t;
            }
        }
        if (t <= 0) return -1;
        _written += t;
    }
    return _written;
}

static sockaddr* matchingSockaddr(addrinfo* addrinfo, sa_family_t family) {
    while (addrinfo) {
        if (addrinfo->ai_family == family || family == 0) {
            return addrinfo->ai_addr;
        }
        addrinfo = addrinfo->ai_next;
    }
    return nullptr;
}

sockaddrAny getIntAddr(const std::string& host, int proto, uint16_t port) {
    addrinfo hints{};
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = proto;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* addressResult;
    int errcode = getaddrinfo(host.c_str(), nullptr, &hints, &addressResult);
    if (errcode != 0) {
        throw std::runtime_error("getaddrinfo");
    }
    sockaddrAny resp{};
    sockaddr * matching = matchingSockaddr(addressResult, proto);
    if (matching == nullptr)
        throw std::runtime_error("getaddrinfo - no matching family");
    proto = matching->sa_family;
    if (proto == AF_INET) {
        memcpy(&resp.addr.addr_in, matching, sizeof(sockaddr_in));
        resp.addr.addr_in.sin_port = port;
    }
    else {
        memcpy(&resp.addr.addr_in6, matching, sizeof(sockaddr_in6));
        resp.addr.addr_in6.sin6_port = port;
    }
    resp.family = proto;

    freeaddrinfo(addressResult);

    return resp;
}

NetAddress getAddrStruct(int fd, sa_family_t proto) {
    uint16_t addrPort;
    std::string addrHost;
    if (proto == AF_INET) {
        sockaddr_in addrServer{};
        socklen_t sockLen = sizeof addrServer;

        if (getsockname(fd, (sockaddr *) &addrServer, &sockLen)) {
            throw std::runtime_error("getsockname");
        }

        addrPort = ntohs(addrServer.sin_port);
        addrHost = inet_ntoa(addrServer.sin_addr);
    }
    else {
        sockaddr_in6 addrServer{};
        socklen_t sockLen = sizeof addrServer;

        if (getsockname(fd, (sockaddr *) &addrServer, &sockLen)) {
            throw std::runtime_error("getsockname");
        }

        addrPort = ntohs(addrServer.sin6_port);
        char buff[128];
        addrHost = inet_ntop(AF_INET6, &addrServer.sin6_addr, buff, 128);
    }
    return std::make_pair(addrPort, std::string(addrHost));
}

std::string formatAddr(const NetAddress& addr) {
    std::string res(addr.second);
    return res + ":" + std::to_string(addr.first);
}

game_scenario parseScenario(const std::string& filePath) {
    game_scenario scenario;
    std::fstream in(filePath);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    size_t ix = 0;
    Side s;
    RoundType type;
    table_state state;
    while (ix < lines.size()) {
        line = lines[ix ++];
        state.clear();
        type = static_cast<RoundType>(line.at(0) - '1' + 1);
        s = static_cast<Side>(line.at(1));
        for (Side s_: sides_) {
            state[s_] = Hand();
            int iter = 0;
            line = lines[ix ++];
            for (int i = 0; i < TRICKS_PER_ROUND; i ++) {
                if (line[iter] == '1') {
                    state[s_].push_back(
                            Card::fromString(line.substr(iter, 3)));
                    iter += 1;
                }
                else state[s_].push_back(
                        Card::fromString(line.substr(iter, 2)));
                iter += 2;
            }
        }
        scenario.emplace_back(type, state, s);
    }
    return scenario;
}

Side nxtSide(const Side &s) {
    switch (s) {
        case N:
            return E;
        case E:
            return S;
        case S:
            return W;
        case W:
            return N;
        case SIDE_NULL_:
            throw std::runtime_error("invalid side for nxtSide: SIDE_NULL_");
    }
    return N;
}

void ignoreBrokenPipe() {
    struct sigaction newAction{};
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;
    newAction.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &newAction, nullptr);
}

void setTimeout(int fd, time_t seconds) {
    timeval to = {.tv_sec = seconds, .tv_usec = 0};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to)) {
        throw std::runtime_error("setsockopt");
    }
}

void rmIntersection(Hand& hand, const Table& table) {
    for (const Card& ct: table) {
        rmCardIfPresent(hand, ct);
    }
}

void rmCardIfPresent(Hand& hand, const Card& card) {
    for (auto it = hand.begin(); it != hand.end(); it ++) {
        if (it->getColor() == card.getColor()
            && it->getValue() == card.getValue()) {
            hand.erase(it);
            return;
        }
    }
}

std::ostream& operator<<(std::ostream & os, const Side & s) {
    return os << static_cast<char>(s);
}

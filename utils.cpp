//
// Created by szymon on 14.05.24.
//

#include "utils.h"
#include "constants.h"
#include "game_rules.h"
#include "common_types.h"

#include "fstream"
#include "string"
#include "vector"
#include "unistd.h"
#include "arpa/inet.h"
#include "netdb.h"
#include "stdexcept"
#include "cstring"
#include "csignal"

size_t writeN(int fd, void * buff, size_t n) {
    size_t written = 0;
    char * buff_c = (char *) buff;
    while (written < n) {
        ssize_t t = write(fd, buff_c + written, n - written);
        if (t <= 0) return -1;
        written += t;
    }
    return written;
}

size_t readN(int fd, void * buff, size_t n) {
    char * buff_c = (char *) buff;
    size_t _read = 0;
    while (_read < n) {
        ssize_t t = read(fd, buff_c + _read, n - _read);
        if (t <= 0) return -1;
        _read += t;
    }
    return _read;
}

std::string readUntilRN(int fd) {
    char buff[1024];
    size_t itr = 0;
    bool finish = false;
    while (!finish) {
        ssize_t  res = read(fd, buff + itr, 1);
        if (res != 1) {
            throw std::runtime_error("error on reading message");
        }
        if (buff[itr ++] == '\r') finish = true;
    }
    ssize_t res = read(fd, buff + itr, 1);
    if (res < 1) throw std::runtime_error("error on reading message");
    if (buff[itr] != '\n') throw std::runtime_error("invalid msg");
    return {buff, itr - 1};
}

int setSockTimeout(int fd, int milis);


sockaddr_any getIntAddr(const std::string& host, int proto, uint16_t port) {
    addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = proto;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* address_result;
    int errcode = getaddrinfo(host.c_str(), nullptr, &hints, &address_result);
    if (errcode != 0) {
        throw std::runtime_error("getaddrinfo");
    }
    sa_family_t proto_res = address_result->ai_addr->sa_family;
    sockaddr_any resp;
    resp.family = proto_res;
    if (proto_res == AF_INET) {
        auto * tmp = new sockaddr_in;
        tmp->sin_addr.s_addr = ((sockaddr_in*) address_result->ai_addr)->sin_addr.s_addr;
        tmp->sin_port = port;
        tmp->sin_family = ((sockaddr_in*) address_result->ai_addr)->sin_family;
        resp.addr = {.addr_in = tmp};
    }
    else {
        auto * tmp = new sockaddr_in6;
        tmp->sin6_addr = ((sockaddr_in6*) address_result->ai_addr)->sin6_addr;
        tmp->sin6_port = port;
        tmp->sin6_family = ((sockaddr_in6*) address_result->ai_addr)->sin6_family;
        resp.addr = {.addr_in6 = tmp};
    }

    freeaddrinfo(address_result);

    return resp;
}

net_address getAddrStruct(int fd, sa_family_t proto) {
    uint16_t s_port;
    std::string s_addr;
    if (proto == AF_INET) {
        sockaddr_in addr_server;
        socklen_t socklen = sizeof addr_server;

        if (getsockname(fd, (sockaddr *) &addr_server, &socklen)) {
            throw std::runtime_error("getsockname");
        }

        s_port = ntohs(addr_server.sin_port);
        s_addr = inet_ntoa(addr_server.sin_addr);
    }
    else {
        sockaddr_in6 addr_server;
        socklen_t socklen = sizeof addr_server;

        if (getsockname(fd, (sockaddr *) &addr_server, &socklen)) {
            throw std::runtime_error("getsockname");
        }

        s_port = ntohs(addr_server.sin6_port);
        char buff[128];
        s_addr = inet_ntop(AF_INET6, &addr_server.sin6_addr, buff, 128);
    }
    return std::make_pair(s_port, s_addr);
}

std::string formatAddr(const net_address& addr) {
    std::string res(addr.second);
    return res + ":" + std::to_string(addr.first);
}

game_scenario parseScenario(const std::string& filepath) {
    game_scenario scenario;
    std::fstream in(filepath);
    std::string line;
    std::vector<std::string> lines;
    while (!in.eof()) {
        std::getline(in, line);
        lines.push_back(line);
    }
    size_t ix = 0;
    Side s;
    RoundType type;
    table_state state;
    while (ix < lines.size()) {
        line = lines[ix ++];
        state.clear();
        type = (RoundType) (line.at(0) - '1' + 1);
        s = (Side) line.at(1);
        for (Side s_: sides_) {
            state[s_] = Hand();
            int iter = 0;
            line = lines[ix ++];
            for (int i = 0; i < TRICKS_PER_ROUND; i ++) {
                if (line[iter] == '1') {
                    state[s_].push_back(Card::fromString(line.substr(iter, 3)));
                    iter += 1;
                }
                else state[s_].push_back(Card::fromString(line.substr(iter, 2)));
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

void dummy(int){}

void ignoreBrokenPipe() {
    struct sigaction new_action;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    new_action.sa_handler = dummy;
    sigaction(SIGPIPE, &new_action, nullptr);
}

void rmIntersection(Hand& hand, const Table& table) {
    for (const Card& ct: table) {
        rmCardIfPresent(hand, ct);
    }
}

void rmCardIfPresent(Hand& hand, const Card& card) {
    for (auto it = hand.begin(); it != hand.end(); it ++) {
        if (it->getColor() == card.getColor() && it->getValue() == card.getValue()) {
            hand.erase(it);
            return;
        }
    }
}

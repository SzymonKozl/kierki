//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_UTILS_H
#define KIERKI_UTILS_H

#include "common_types.h"

#include "unistd.h"
#include "string"
#include "iostream"
#include "arpa/inet.h"
#include "sys/socket.h"

ssize_t writeN(int fd, void * buff, size_t n);

ssize_t readN(int fd, void * buff, size_t n);

ssize_t sendNoBlockN(int fd, void * buff, ssize_t n);

std::string formatAddr(const net_address& addr);

std::string readUntilRN(int fd);

Side nxtSide(const Side& s);

sockaddr_any getIntAddr(const std::string& host, int proto, uint16_t port);

net_address getAddrStruct(int fd, sa_family_t proto);

game_scenario parseScenario(const std::string& filepath);

void ignoreBrokenPipe();

void setTimeout(int fd, time_t seconds);

void rmIntersection(Hand& hand, const Table& table);

void rmCardIfPresent(Hand& hand, const Card& card);

template<typename T> requires requires (const T a) {
    std::cout << a;
}
inline void printList(std::vector<T> const& objects) {
    for (size_t i = 0; i < objects.size(); i ++) {
        std::cout << objects.at(i);
        if (i != objects.size() - 1) std::cout << ", ";
    }
}

std::ostream& operator<<(std::ostream & os, const Side & s);

#endif //KIERKI_UTILS_H

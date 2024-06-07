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

// unreachable macro for parts of code that should be never be reached
// but for some reason are present (ex. to prevent compilation warnings)
#define ASSERT_UNREACHABLE assert((false && "this code should be unreachable"))

ssize_t sendNoBlockN(int fd, const void * buff, ssize_t n);

std::string formatAddr(const NetAddress& addr);

Side nxtSide(const Side& s);

sockaddrAny getIntAddr(const std::string& host, int proto, uint16_t port);

NetAddress getAddrStruct(int fd, sa_family_t proto);

game_scenario parseScenario(const std::string& filePath);

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

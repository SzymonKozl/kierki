//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_UTILS_H
#define KIERKI_UTILS_H

#include "common_types.h"

#include "unistd.h"
#include "string"
#include "arpa/inet.h"
#include "sys/socket.h"

size_t writeN(int fd, void * buff, size_t n);
size_t readN(int fd, void * buff, size_t n);

std::string formatAddr(const net_address& addr);

std::string readUntilRN(int fd);

Side nxtSide(const Side& s);

sockaddr_any getIntAddr(const std::string& host, int proto, uint16_t port);

net_address getAddrStruct(int fd, sa_family_t proto);

game_scenario parseScenario(const std::string& filepath);

void ignoreBrokenPipe();

void rmIntersection(Hand& hand, const Table& table);

void rmCardIfPresent(Hand& hand, const Card& card);

#endif //KIERKI_UTILS_H

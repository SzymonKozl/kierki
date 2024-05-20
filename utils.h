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

in_addr_t getIntAddr(std::string host);

net_address getAddrStruct(int fd);

game_scenario parseScenario(const std::string& filepath);

void ignoreBrokenPipe();

void rmIntersection(Hand& hand, const Table& table);

void rmCardIfPresent(Hand& hand, const Card& card);

#endif //KIERKI_UTILS_H

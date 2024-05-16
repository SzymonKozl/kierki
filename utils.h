//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_UTILS_H
#define KIERKI_UTILS_H

#include "common_types.h"

#include "unistd.h"
#include "string"

int writen(int fd, void * buff, size_t n);
int readn(int fd, void * buff, size_t n);

std::string formatAddr(net_address addr);

std::string readUntilRN(int fd);

Side nxtSide(const Side& s);

#endif //KIERKI_UTILS_H

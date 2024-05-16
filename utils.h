//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_UTILS_H
#define KIERKI_UTILS_H

#include "unistd.h"
#include "string"
#include "common_types.h"

int writen(int fd, void * buff, size_t n);
int readn(int fd, void * buff, size_t n);

std::string readUntilRN(int fd);

Side nxtSide(const Side& s);

#endif //KIERKI_UTILS_H

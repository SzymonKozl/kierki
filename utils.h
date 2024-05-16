//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_UTILS_H
#define KIERKI_UTILS_H

#include "unistd.h"
#include "string"

int writen(int fd, void * buff, size_t n);
int readn(int fd, void * buff, size_t n);

std::string readUntilRN(int fd);

#endif //KIERKI_UTILS_H

//
// Created by szymon on 14.05.24.
//

#include "utils.h"

#include "unistd.h"
#include "stdexcept"

int writen(int fd, void * buff, size_t n) {
    size_t written = 0;
    char * buff_c = (char *) buff;
    while (written < n) {
        ssize_t t = write(fd, buff_c + written, n - written);
        if (t <= 0) return -1;
        written += t;
    }
    return written;
}

int readn(int fd, void * buff, size_t n) {
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
    return std::string(buff, itr + 1);
}

int setSockTimeout(int fd, int milis);

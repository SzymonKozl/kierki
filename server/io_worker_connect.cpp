//
// Created by szymon on 12.05.24.
//

#include "io_worker_connect.h"
#include "../common/constants.h"

#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "endian.h"

IOWorkerConnect::IOWorkerConnect(
        int pipeFd,
        int id,
        int sockFd,
        IOWorkerExitCb exitCallback,
        IOWorkerTimeoutCb timeoutCallback,
        IOWorkerExecuteSafeCb execCallback,
        IOWorkerConnectionMadeCb acceptCallback,
        const NetAddress& ownAddr,
        Logger& logger
        ):
        IOWorker(
                pipeFd,
                id,
                sockFd,
                std::move(exitCallback),
                std::move(timeoutCallback),
                std::move(execCallback),
                IO_ERR_INTERNAL,
                ownAddr,
                {0, ""},
                -1,
                logger
                ),
        accCb(std::move(acceptCallback))
{}

void IOWorkerConnect::socketAction() {
    sockaddr clientAddr{};
    socklen_t clientAddrLen = sizeof clientAddr;
    int newFd = accept(mainFd, &clientAddr, &clientAddrLen);
    if (newFd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        errs.emplace_back("accept", errno, IO_ERR_INTERNAL);
        throw std::runtime_error("");
    }
    if (clientAddr.sa_family == AF_INET) {
        auto* clientV4 = reinterpret_cast<sockaddr_in*>(&clientAddr);
        uint16_t port = be16toh(clientV4->sin_port);
        std::string addr = inet_ntoa(clientV4->sin_addr);
        accCb(newFd, std::make_pair(port, addr));
    }
    else {
        auto* clientV6 = (sockaddr_in6*) &clientAddr;
        uint16_t port = be16toh(clientV6->sin6_port);
        char buff[128];
        std::string addr = inet_ntop(
                AF_INET6,
                reinterpret_cast<const void *>(
                        static_cast<const in6_addr*>(&clientV6->sin6_addr)),
                buff,
                (socklen_t)128);
        accCb(newFd, std::make_pair(port, addr));
    }
}

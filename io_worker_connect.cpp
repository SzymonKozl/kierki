//
// Created by szymon on 12.05.24.
//

#include "io_worker_connect.h"

#include <utility>
#include "common_types.h"
#include "constants.h"

#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "endian.h"

IOWorkerConnect::IOWorkerConnect(
        int pipe_fd,
        int id,
        int sock_fd,
        IOWorkerExitCb exit_callback,
        IOWorkerConnectionMadeCb accept_callback
        ):
        IOWorker(pipe_fd, id, sock_fd, std::move(exit_callback), IO_ERR_INTERNAL, SIDE_NULL_),
        accCb(std::move(accept_callback))
{}


void IOWorkerConnect::pollAction() {
    sockaddr client_addr;
    socklen_t client_addr_s = sizeof client_addr;
    int new_fd = accept(main_fd, &client_addr, &client_addr_s);
    if (new_fd < 0) {
        errs.emplace_back("accept", errno, IO_ERR_INTERNAL);
        throw std::runtime_error("");
    }
    if (client_addr.sa_family == AF_INET) {
        auto* client_v4 = (sockaddr_in*) &client_addr;
        uint16_t port = be16toh(client_v4->sin_port);
        std::string addr = inet_ntoa(client_v4->sin_addr);
        accCb(new_fd, std::make_pair(port, addr));
    }
    else {
        auto* client_v6 = (sockaddr_in6*) &client_addr;
        uint16_t port = be16toh(client_v6->sin6_port);
        char buff[128];
        std::string addr = inet_ntop(AF_INET6, (const void *) &client_v6->sin6_addr, buff, (socklen_t)128);
        accCb(new_fd, std::make_pair(port, addr));
    }
}

void IOWorkerConnect::quitAction() {

}

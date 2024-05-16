//
// Created by szymon on 12.05.24.
//

#include "io_worker_connect.h"
#include "common_types.h"

#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"

IOWorkerConnect::IOWorkerConnect(
        int pipe_fd,
        int id,
        int sock_fd,
        IOWorkerExitCb &exit_callback,
        IOWorkerSysErrCb &error_callback,
        IOWorkerConnectionMadeCb &accept_callback
        ):
        IOWorker(pipe_fd, id, sock_fd, exit_callback, error_callback),
        accCb(accept_callback)
{}


void IOWorkerConnect::pollAction() {
    sockaddr_in client_addr;
    socklen_t client_addr_s = sizeof client_addr;
    int new_fd = accept(main_fd, (sockaddr *) &client_addr, &client_addr_s);
    if (new_fd < 0) {
        errCb("accept", errno, IO_ERR_EXTERNAL, _SIDE_NULL);
    }
    accCb(new_fd);
}

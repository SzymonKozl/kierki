//
// Created by szymon on 11.05.24.
//

#include "io_worker.h"

#include <utility>
#include "common_types.h"
#include "utils.h"
#include "constants.h"

#include "poll.h"
#include "unistd.h"
#include "cerrno"

IOWorker::IOWorker(
        int pipe_fd,
        int id,
        int sock_fd,
        IOWorkerExitCb& exit_callback,
        IOWorkerSysErrCb& error_callback
        ) :
    id(id),
    terminate(false),
    main_fd(sock_fd),
    pipe_fd(pipe_fd),
    jobQueue(),
    exitCb(exit_callback),
    errCb(error_callback),
    err(),
    err_type(IO_ERR_NOERR)
{}

void IOWorker::newJob(SSendJob job) {
    jobQueue.pushNextJob(std::move(job));
}

void IOWorker::scheduleDeath() {
    jobQueue.setKillOrder();
}

void IOWorker::run() {
    pollfd poll_fds[2];
    poll_fds[0].fd = main_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = pipe_fd;
    poll_fds[1].events = POLLIN;

    while (!terminate) {
        poll_fds[0].revents = 0;
        poll_fds[1].revents = 0;
        int poll_out = poll(poll_fds, 2, 1000);
        if (poll_out == 0) continue;
        if (poll_out < 0) {
            err = "poll";
            err_type = IO_ERR_EXTERNAL;
            informAboutError();
            terminate = true;
            continue;
        }
        else {
            if (poll_fds[1].revents & POLLIN) {
                char msg;
                ssize_t read_len = read(pipe_fd, &msg, 1);
                if (read_len < 0) {
                    err = "read";
                    err_type = IO_ERR_INTERNAL;
                    informAboutError();
                    terminate = true;
                }
                else {
                    // new job or exit order
                    if (jobQueue.hasKillOrder()) {
                        terminate = true;
                        continue;
                    }
                    SSendJob sJob = jobQueue.popNextJob();
                    std::string payload = sJob->genMsg();
                    ssize_t sent_len = writeN(main_fd, (void *) payload.c_str(), payload.size());
                    if (sent_len < 0) {
                        err = "write";
                        err_type = IO_ERR_EXTERNAL;
                        informAboutError();
                        terminate = true;
                    }
                }
            }
            else if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                err = "pipe";
                err_type = IO_ERR_INTERNAL;
                informAboutError();
                terminate = true;
            }
            else if (poll_fds[0].revents & POLLIN) {
                pollAction();
            }
            else if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                err = "socket";
                err_type = IO_ERR_EXTERNAL;
                informAboutError();
                terminate = true;
            }
        }
    }
    quitAction();
    exitCb(id);
}

void IOWorker::informAboutError() {
    errCb({err, errno, err_type});
}

void IOWorker::halt() {
    jobQueue.setStopped(true);
}

void IOWorker::unhalt() {
    jobQueue.setStopped(false);
}

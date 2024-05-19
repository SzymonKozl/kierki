//
// Created by szymon on 11.05.24.
//

#include "io_worker.h"
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
    main_fd(sock_fd),
    pipe_fd(pipe_fd),
    jobQueue(),
    exitCb(exit_callback),
    errCb(error_callback),
    terminate(false),
    id(id),
    err(),
    err_type(IO_ERR_NOERR)
{}

void IOWorker::newJob(SSendJob job) {
    jobQueue.pushNextJob(job);
}

void IOWorker::scheduleDeath() {
    jobQueue.setKillOrder();
}

void IOWorker::run() {
    pollfd pollfds[2];
    pollfds[0].fd = main_fd;
    pollfds[0].events = POLLIN;
    pollfds[1].fd = pipe_fd;
    pollfds[1].events = POLLIN;

    while (!terminate) {
        pollfds[0].revents = 0;
        pollfds[1].revents = 0;
        int poll_out = poll(pollfds, 2, 1000);
        if (poll_out == 0) continue;
        if (poll_out < 0) {
            err = "poll";
            err_type = IO_ERR_EXTERNAL;
            informAboutError();
            terminate = true;
            continue;
        }
        else {
            if (pollfds[1].revents & POLLIN) {
                char msg;
                int read_len = read(pipe_fd, &msg, 1);
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
                    int sent_len = writeN(main_fd, (void *) payload.c_str(), payload.size());
                    if (sent_len < 0) {
                        err = "write";
                        err_type = IO_ERR_EXTERNAL;
                        informAboutError();
                        terminate = true;
                    }
                }
            }
            else if (pollfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                err = "pipe";
                err_type = IO_ERR_INTERNAL;
                informAboutError();
                terminate = true;
            }
            else if (pollfds[0].revents & POLLIN) {
                pollAction();
            }
            else if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                err = "socket";
                err_type = IO_ERR_EXTERNAL;
                informAboutError();
                terminate = true;
            }
        }
    }
    quitAction();
    exitCb(0);
}

void IOWorker::informAboutError() {
    errCb(err, errno, err_type);
}

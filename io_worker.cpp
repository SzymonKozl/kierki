//
// Created by szymon on 11.05.24.
//

#include "io_worker.h"
#include "common_types.h"
#include "utils.h"

#include "poll.h"
#include "unistd.h"
#include "cerrno"

IOWorker::IOWorker(
        int sock_fd,
        int id,
        int pipe_fd,
        IOWorkerExitCb& exit_callback,
        IOWorkerSysErrCb& error_callback
        ) :
    main_fd(sock_fd),
    pipe_fd(pipe_fd),
    jobQueue(),
    exitCb(exit_callback),
    errCb(error_callback),
    terminate(false),
    id(id)
{}

void IOWorker::newJob(SendJob &job) {
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
    pollfds[1].fd = POLLIN;

    while (!terminate) {
        pollfds[0].revents = 0;
        pollfds[1].revents = 0;
        int poll_out = poll(pollfds, 2, 1000);
        if (poll_out == 0) continue;
        if (poll_out < 0) {
            errCb("poll", errno, IO_ERR_INTERNAL);
            terminate = true;
            continue;
        }
        else {
            if (pollfds[1].events & POLLIN) {
                char msg;
                int read_len = read(pipe_fd, &msg, 1);
                if (read_len < 0) {
                    errCb("read", errno, IO_ERR_INTERNAL);
                    terminate = true;
                }
                else {
                    // new job or exit order
                    if (jobQueue.hasKillOrder()) {
                        terminate = true;
                        continue;
                    }
                    SendJob job = jobQueue.popNextJob();
                    std::string payload = job.genMsg();
                    int sent_len = writen(main_fd, (void *) payload.c_str(), payload.size());
                    if (sent_len < 0) {
                        errCb("write", errno, IO_ERR_EXTERNAL);
                        terminate = true;
                    }
                }
            }
            else if (pollfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                errCb("pipe", errno, IO_ERR_INTERNAL);
                terminate = true;
            }
            else if (pollfds[0].revents & POLLIN) {
                pollAction();
            }
            else if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                errCb("socket", errno, IO_ERR_EXTERNAL);
                terminate = true;
            }
        }
        exitCb(id);
    }
}

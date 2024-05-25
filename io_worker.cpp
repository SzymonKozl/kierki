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
        IOWorkerExitCb exit_callback,
        IOWorkerPipeCloseCb pipe_close_callback,
        int mainSockErr,
        Side side
        ) :
    id(id),
    terminate(false),
    main_fd(sock_fd),
    pipe_fd(pipe_fd),
    jobQueue(),
    exitCb(std::move(exit_callback)),
    pipeCb(std::move(pipe_close_callback)),
    errs(),
    mainSockErr(mainSockErr),
    side(side),
    closedFd(false)
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
            errs.emplace_back("poll", errno, mainSockErr);
            terminate = true;
            break;
        }
        else {
            if (poll_fds[1].revents & POLLIN) {
                char msg;
                ssize_t read_len = read(pipe_fd, &msg, 1);
                if (read_len < 0) {
                    errs.emplace_back("read", errno, IO_ERR_INTERNAL);
                    break;
                }
                else
                    // new job or exit order
                    while (jobQueue.hasNextJob()) {
                        SSendJob sJob = jobQueue.popNextJob();
                        std::string payload = sJob->genMsg();
                        ssize_t sent_len = writeN(main_fd, (void *) payload.c_str(), payload.size());
                        if (sent_len < 0) {
                            errs.emplace_back("write", errno, mainSockErr);
                            terminate = true;
                            break;
                        }
                    }{
                    if (jobQueue.hasKillOrder()) {
                        terminate = true;
                        break;
                    }
                    if (terminate) break;
                }
            }
            else if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                errs.emplace_back("pipe", errno, IO_ERR_INTERNAL);
                terminate = true;
                break;
            }
            else if (poll_fds[0].revents & POLLIN) {
                try {
                    pollAction();
                } catch (std::exception &e) {
                    terminate = true;
                    break;
                }
            }
            else if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                errs.emplace_back("socket", errno, mainSockErr);
                informAboutError();
                terminate = true;
            }
        }
    }
    pipeCb(id);
    exitCb(errs, id, side);
}

void IOWorker::informAboutError() {
}

void IOWorker::halt() {
    jobQueue.setStopped(true);
}

void IOWorker::unhalt() {
    jobQueue.setStopped(false);
}

IOWorker::~IOWorker() {
    if (!closedFd) {
        if (close(main_fd)) std::cerr << "error on close: " << errno << std:: endl << std::flush;
    }
}

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
#include "fcntl.h"

IOWorker::IOWorker(
        int pipe_fd,
        int id,
        int sock_fd,
        IOWorkerExitCb exit_callback,
        IOWorkerPipeCloseCb pipe_close_callback,
        IOWorkerTimeoutCb  timeoutCb,
        IOWorkerExecuteSafeCb exec_callback,
        int mainSockErr,
        net_address  ownAddr,
        net_address  clientAddr,
        int timeout,
        Logger& logger
        ) :
    id(id),
    wantToToQuit(false),
    terminate(false),
    main_fd(sock_fd),
    pipe_fd(pipe_fd),
    jobQueue(),
    exitCb(std::move(exit_callback)),
    timeoutCb(std::move(timeoutCb)),
    pipeCb(std::move(pipe_close_callback)),
    execCb(std::move(exec_callback)),
    errs(),
    mainSockErr(mainSockErr),
    closedFd(false),
    ownAddr(std::move(ownAddr)),
    clientAddr(std::move(clientAddr)),
    responseTimeout(std::make_shared<decltype(responseTimeout)::element_type>(std::chrono::system_clock::now() + std::chrono::seconds(timeout))),
    timeout(timeout),
    nextTimeout(-1),
    logger(logger),
    peerCorrupted(false),
    poll_fds(nullptr)
{
}

void IOWorker::newJob(const SSendJob& job) {
    jobQueue.pushNextJob(job);
}

void IOWorker::scheduleDeath() {
    jobQueue.setKillOrder();
}

void IOWorker::run() {
    poll_fds = new pollfd[2];
    poll_fds[0].fd = main_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = pipe_fd;
    poll_fds[1].events = POLLIN;

    fcntl(pipe_fd, F_SETFL, fcntl(pipe_fd, F_GETFL) | O_NONBLOCK);
    fcntl(main_fd, F_SETFL, fcntl(pipe_fd, F_GETFL) | O_NONBLOCK);

    while (!terminate) {
        poll_fds[0].revents = 0;
        poll_fds[1].revents = 0;

        // preventing active waiting
        if (!pendingIncoming.empty()) poll_fds[0].fd *= -1;

        int pollResp = poll(poll_fds, 2, nextTimeout);

        if (!pendingIncoming.empty()) poll_fds[0].fd *= -1;

        nextTimeout = -1;
        if (pollResp == 0) {
            timeoutCb(id);
            nextTimeout = -1;
            responseTimeout.reset();
        }
        else if (pollResp < 0) {
            errs.emplace_back("poll", errno, mainSockErr);
            closeConn();
            if (exitCb(errs, id, hasWork())) {
                terminate = true;
                break;
            }
        }
        else {
            handlePipe();
            if (terminate) break;
            socketAction();
            if (terminate) break;
            while (jobQueue.hasNextJob()) {
                pendingOutgoing.push(jobQueue.popNextJob());
            }
            handleQueue();
            if (terminate) break;
            if (jobQueue.hasKillOrder()) {
                while (jobQueue.hasNextJob()) {
                    pendingOutgoing.push(jobQueue.popNextJob());
                }
                handleQueue();
                closeConn();
            }
            if (wantToToQuit) {
                if (exitCb(errs, id, hasWork())) {
                    terminate = true;
                    break;
                }
            }
            if (nextTimeout == -1) {
                if (timeout > 0 && responseTimeout.use_count() > 0) {
                    nextTimeout = std::chrono::duration_cast<std::chrono::milliseconds>(
                            *responseTimeout - std::chrono::system_clock::now()).count();
                    if (nextTimeout < 0) {
                        timeoutCb(id);
                        nextTimeout = -1;
                    }
                }
                else nextTimeout = -1;

            }
        }
    }

    exitCb(errs, id, hasWork());
}

IOWorker::~IOWorker() {
    if (!closedFd) {
        if (close(main_fd)) std::cerr << "error on close: " << errno << std:: endl << std::flush;
    }
    delete[] poll_fds;
    poll_fds = nullptr;
}

void IOWorker::handlePipe() {
    ssize_t read_len;
    char msg;
    do {
        read_len = read(pipe_fd, &msg, 1);
    } while (read_len > 0);
    if (read_len < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            errs.emplace_back("read", errno, IO_ERR_INTERNAL);
            terminate = true;
        }
    }
}

void IOWorker::handleQueue() {
    if (closedFd) return;
    while (!pendingOutgoing.empty()) {
        if (!execCb([this] {
            SSendJob job = pendingOutgoing.front();
            pendingOutgoing.pop();
            std::string msg = job->genMsg();
            errno = 0;
            ssize_t send_resp = sendNoBlockN(main_fd, (void *) msg.c_str(), static_cast<ssize_t>(msg.size()));
            if (send_resp != (ssize_t)msg.size()) {
                this->errs.emplace_back("send", errno, mainSockErr);
                closeConn();
                this->terminate = true;
            } else {
                this->responseTimeout = std::make_shared<decltype(this->responseTimeout)::element_type>(
                        std::chrono::system_clock::now() + std::chrono::seconds(this->timeout)
                );
                this->nextTimeout = this->timeout * 1000;
                this->logger.log(
                        Message(this->ownAddr, this->clientAddr, msg.substr(0, msg.size() - 2))
                );
            }
        })) {
            break;
        }
    }
}

bool IOWorker::hasWork() {
    if (!pendingIncoming.empty()) return true;
    if (!pendingOutgoing.empty() && !closedFd) return true;
    return false;
}

void IOWorker::closeConn() {
    if (!closedFd) {
        if (close(main_fd)) {
            errs.emplace_back("close", errno, mainSockErr);
        }
        closedFd = true;
    }
    wantToToQuit = true;
    if (poll_fds) {
        poll_fds[0].fd = -1;
    }
}

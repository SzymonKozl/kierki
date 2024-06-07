//
// Created by szymon on 11.05.24.
//

#include "io_worker.h"

#include <utility>
#include "../common/utils.h"

#include "poll.h"
#include "unistd.h"
#include "cerrno"
#include "fcntl.h"

IOWorker::IOWorker(
        int pipeFd,
        int id,
        int sockFd,
        IOWorkerExitCb exitCallback,
        IOWorkerTimeoutCb  timeoutCallback,
        IOWorkerExecuteSafeCb execCallback,
        IOErrClass mainSockErr,
        NetAddress  ownAddr,
        NetAddress  clientAddr,
        int timeout,
        Logger& logger
        ) :
    id(id),
    wantToToQuit(false),
    terminate(false),
    mainFd(sockFd),
    pipeFd(pipeFd),
    jobQueue(),
    exitCb(std::move(exitCallback)),
    timeoutCb(std::move(timeoutCallback)),
    execCb(std::move(execCallback)),
    errs(),
    mainSockErr(mainSockErr),
    closedFd(false),
    ownAddr(std::move(ownAddr)),
    clientAddr(std::move(clientAddr)),
    responseTimeout(std::make_shared<TimeVal>(std::chrono::system_clock::now()
        + std::chrono::seconds(timeout))),
    timeout(timeout),
    nextTimeout(-1),
    logger(logger),
    peerCorrupted(false),
    pollFds(nullptr)
{}

void IOWorker::newJob(const SSendJob& job) {
    jobQueue.pushNextJob(job);
}

void IOWorker::scheduleDeath() {
    jobQueue.setKillOrder();
}

void IOWorker::run() {
    pollFds = new pollfd[2];
    pollFds[0].fd = mainFd;
    pollFds[0].events = POLLIN;
    pollFds[1].fd = pipeFd;
    pollFds[1].events = POLLIN;

    if (fcntl(pipeFd, F_SETFL, fcntl(pipeFd, F_GETFL) | O_NONBLOCK)) {
        errs.emplace_back("fcntl", errno, mainSockErr);
        terminate = true;
        
    }
    if (fcntl(mainFd, F_SETFL, fcntl(mainFd, F_GETFL) | O_NONBLOCK)) {
        errs.emplace_back("fcntl", errno, mainSockErr);
        terminate = true;
    }

    while (!terminate) {
        pollFds[0].revents = 0;
        pollFds[1].revents = 0;

        // preventing active waiting
        if (!pendingIncoming.empty()) pollFds[0].fd *= -1;

        int pollResp = poll(pollFds, 2, nextTimeout);

        if (!pendingIncoming.empty()) pollFds[0].fd *= -1;

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
                    nextTimeout = static_cast<int>(std::chrono::duration_cast<
                            std::chrono::milliseconds>(*responseTimeout -
                            std::chrono::system_clock::now()).count());
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
    delete[] pollFds;
    pollFds = nullptr;
    if (!closedFd) {
        if (close(mainFd)) std::cerr << "error on close: " << errno <<
            std:: endl << std::flush;
    }
}

void IOWorker::handlePipe() {
    ssize_t readLen;
    char msg;
    do {
        readLen = read(pipeFd, &msg, 1);
    } while (readLen > 0);
    if (readLen < 0) {
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
            ssize_t sendResp = sendNoBlockN(mainFd, (void *) msg.c_str(),
                                            static_cast<ssize_t>(msg.size()));
            if (sendResp != static_cast<ssize_t>(msg.size())) {
                this->errs.emplace_back("send", errno, mainSockErr);
                closeConn();
                this->terminate = true;
            } else {
                this->responseTimeout = std::make_shared<TimeVal>(
                        std::chrono::system_clock::now() +
                        std::chrono::seconds(this->timeout)
                );
                this->nextTimeout = this->timeout * 1000;
                this->logger.log(Message(
                        this->ownAddr,
                        this->clientAddr,
                        msg.substr(0, msg.size() - 2))
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
        if (close(mainFd)) {
            errs.emplace_back("close", errno, mainSockErr);
        }
        closedFd = true;
    }
    wantToToQuit = true;
    if (pollFds) {
        pollFds[0].fd = -1;
    }
}

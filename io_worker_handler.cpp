//
// Created by szymon on 12.05.24.
//

#include "io_worker_handler.h"

#include <utility>
#include "common_types.h"
#include "network_msg_parser.h"
#include "constants.h"
#include "logger.h"

#include "string"
#include "cerrno"
#include "cassert"

IOWorkerHandler::IOWorkerHandler(
        int pipe_fd,
        int id,
        int sock_fd,
        IOWorkerExitCb exit_callback,
        IOWorkerPipeCloseCb pipe_close_callback,
        IOWorkerTimeoutCb timeout_callback,
        IOWorkerExecuteSafeCb exec_callback,
        IOWorkerIntroCb intro_callback,
        IOWrokerTrickCb trick_callback,
        const net_address& clientAddr,
        const net_address& own_addr,
        int timeout,
        Logger& logger
):
        IOWorker(pipe_fd, id, sock_fd, std::move(exit_callback), std::move(pipe_close_callback), std::move(timeout_callback), std::move(exec_callback), IO_ERR_EXTERNAL, ownAddr, clientAddr, timeout, logger),
        trickCb(std::move(trick_callback)),
        introCb(std::move(intro_callback))
{}

void IOWorkerHandler::socketAction() {
    ssize_t readRes;
    char pLoad;
    if (closedFd) return;
    do {
        readRes = recv(main_fd, &pLoad, 1, MSG_DONTWAIT);
        if (readRes == 1){
            nextIncoming += pLoad;
            if (pLoad == '\n' && nextIncoming.size() > 1 && nextIncoming[nextIncoming.size() - 2] == '\r') {
                pendingIncoming.push(nextIncoming.substr(0, nextIncoming.size() - 2));
                nextIncoming.clear();
            }
        }
    } while (readRes == 1);
    if (readRes < 0) {
        if (errno == ECONNRESET) {
            wantToToQuit = true;
            if (!closedFd && close(main_fd)) {
                errs.emplace_back("close", errno, IO_ERR_INTERNAL);
            }
            closedFd = true;
            return;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            closedFd = true;
            errs.emplace_back("recv", errno, IO_ERR_EXTERNAL);
            wantToToQuit = true;
        }
    }
    else {
        wantToToQuit = true;
        if (close(main_fd)) {
            errs.emplace_back("close", errno, IO_ERR_INTERNAL);
        }
        closedFd = true;
        return;
    }
    while (!pendingIncoming.empty()) {
        std::string msg = pendingIncoming.front();
        resp_array parsed = parse_msg(msg, true);
        if (parsed.empty()) {
            if (trickCb(-1, Card("K", COLOR_H), id)) { //todo normal way
                pendingIncoming.pop();
                logger.log(
                        Message(clientAddr, ownAddr, msg)
                );
                responseTimeout.reset(); // todo: przeplot safe
                nextTimeout = -1;
            }
        }
        if (parsed[0].second == "IAM") {
            Side s = (Side) parsed[1].second[0];
            introCb(s, id);
            logger.log(
                    Message(clientAddr, ownAddr, msg)
            );
            pendingIncoming.pop();
            nextTimeout = -1;
            responseTimeout.reset();
        }
        else if (parsed[0].second == "TRICK_C") {
            int trickNo = atoi(parsed[1].second.c_str());
            Card c = Card::fromString(parsed[2].second);
            if (trickCb(trickNo, c, id)) {
                pendingIncoming.pop();
                logger.log(
                        Message(clientAddr, ownAddr, msg.substr(0, msg.size() - 2))
                        );
                nextTimeout = -1; // todo: przeplot safe
                responseTimeout.reset();
            }
            else {
                break;
            }
        }
        else {
            execCb([this]{
                terminate = true;
                wantToToQuit = true;
            });
        }
    }
}

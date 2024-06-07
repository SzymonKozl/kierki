//
// Created by szymon on 12.05.24.
//

#include "io_worker_handler.h"
#include "../common/network_msg_parser.h"
#include "../common/constants.h"
#include "../common/utils.h"

#include "string"
#include "algorithm"
#include "cerrno"

IOWorkerHandler::IOWorkerHandler(
        int pipeFd,
        int id,
        int sockFd,
        IOWorkerExitCb exitCallback,
        IOWorkerTimeoutCb timeoutCallback,
        IOWorkerExecuteSafeCb execCallback,
        IOWorkerIntroCb introCallback,
        IOWrokerTrickCb trickCallback,
        IOWorkerInvalidMsgCb invalidCallback,
        const NetAddress& clientAddr,
        const NetAddress& ownAddr,
        int timeout,
        Logger& logger
):
        IOWorker(pipeFd, id, sockFd, std::move(exitCallback), std::move(timeoutCallback), std::move(execCallback), IO_ERR_EXTERNAL, ownAddr, clientAddr, timeout, logger),
        trickCb(std::move(trickCallback)),
        introCb(std::move(introCallback)),
        invalidCb(std::move(invalidCallback))
{}

void IOWorkerHandler::socketAction() {
    if (wantToToQuit) return;
    ssize_t readRes = 1;
    char pLoad;
    if (!closedFd) {
        while (readRes == 1 && pendingIncoming.empty()) {
            readRes = recv(mainFd, &pLoad, 1, MSG_DONTWAIT);
            if (readRes == 1) {
                nextIncoming += pLoad;
                if (nextIncoming.size() > MAX_PARSE_LEN) {
                    peerCorrupted = true;
                    closeConn();
                    break;
                }
                if (pLoad == '\n' && nextIncoming.size() > 1 && nextIncoming[nextIncoming.size() - 2] == '\r') {
                    pendingIncoming.push(nextIncoming.substr(0, nextIncoming.size() - 2));
                    nextIncoming.clear();
                }
            }
        }
        if (readRes < 0) {
            if (std::find(SILENT_ERRS, SILENT_ERRS + SILENT_ERRS_NO, errno) != SILENT_ERRS + SILENT_ERRS_NO) {
                errs.emplace_back("recv", errno, IO_ERR_SILENT);
                closeConn();
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                errs.emplace_back("recv", errno, IO_ERR_EXTERNAL);
                closeConn();
            }
        } else if (readRes == 0) {
            closeConn();
        }
    }
    while (!pendingIncoming.empty()) {
        std::string msg = pendingIncoming.front();
        ParseResp parsed = parseNetMsg(msg, true);
        if (parsed.empty()) {
            if (invalidCb(id)) {
                logger.log(Message(clientAddr, ownAddr, msg));
                wantToToQuit = true;
                nextTimeout = -1;
                responseTimeout.reset();
                pendingIncoming.pop();
            }
            break;
        }
        if (parsed[0].second == "IAM") {
            Side s = (Side) parsed[1].second[0];
            if (introCb(s, id)) {
                logger.log(
                        Message(clientAddr, ownAddr, msg)
                );
                pendingIncoming.pop();
                nextTimeout = -1;
                responseTimeout.reset();
            }
            else {
                nextTimeout = -1;
                responseTimeout.reset();
                break;
            }
        }
        else if (parsed[0].second == "TRICK_C") {
            int trickNo = atoi(parsed[1].second.c_str());
            Card c = Card::fromString(parsed[2].second);
            if (trickCb(trickNo, c, id)) {
                pendingIncoming.pop();
                logger.log(
                        Message(clientAddr, ownAddr, msg)
                        );
                nextTimeout = -1;
                responseTimeout.reset();
            }
            else {
                nextTimeout = -1;
                responseTimeout.reset();
                break;
            }
        }
        else {
            if (invalidCb(id)) {
                logger.log(Message(clientAddr, ownAddr, msg));
                pendingIncoming.pop();
                wantToToQuit = true;
                nextTimeout = -1;
                responseTimeout.reset();
                break;
            }
        }
    }
    if (peerCorrupted) {
        if (exitCb(errs, id, hasWork())) {
            terminate = true;
        }
    }
}

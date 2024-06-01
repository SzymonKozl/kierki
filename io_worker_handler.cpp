//
// Created by szymon on 12.05.24.
//

#include "io_worker_handler.h"

#include <utility>
#include "utils.h"
#include "common_types.h"
#include "network_msg_parser.h"
#include "constants.h"

#include "string"
#include "stdexcept"
#include "unistd.h"
#include "cerrno"
#include "iostream"

IOWorkerHandler::IOWorkerHandler(
        int pipe_fd,
        int id,
        int sock_fd,
        IOWorkerExitCb exit_callback,
        IOWorkerPipeCloseCb pipe_close_callback,
        IOWorkerIntroCb intro_callback,
        IOWrokerTrickCb trick_callback,
        IOWorkerRepeatOnTiemoutCb repeat_callback,
        const net_address& clientAddr,
        const net_address& own_addr,
        Logger& logger,
        int timeout
):
        IOWorker(pipe_fd, id, sock_fd, std::move(exit_callback), std::move(pipe_close_callback), IO_ERR_EXTERNAL, SIDE_NULL_, logger, ownAddr, clientAddr, timeout),
        trickCb(std::move(trick_callback)),
        introCb(std::move(intro_callback)),
        repeatCb(std::move(repeat_callback)),
        introduced(false)
        {}

void IOWorkerHandler::pollAction() {
    std::string msg;
    try {
        msg = readUntilRN(main_fd);
    } catch (...) {
        // invalid message from Client, disconnecting
        if (close(main_fd)) {
            errs.emplace_back("close", errno, IO_ERR_EXTERNAL);
        }
        throw std::runtime_error("");
    }

    Message messageObj(clientAddr, ownAddr, msg);
    logger.log(messageObj);

    resp_array arr = parse_msg(msg, true);
    if (!arr.empty()) {
        if (arr[0].second == "IAM" && !introduced) {

            Side s = static_cast<Side>(arr[1].second.at(0));
            introduced = true;
            side = s;
            introCb(s, id);
            return;
        }
        else if (arr[0].second == "TRICK_C") {
            int round_no = stoi(arr[1].second);
            trickCb(round_no, side, Card::fromString(arr[2].second)); //todo what if not introduced
            return;
        }
    }
    else {
        if (close(main_fd)) {
            errs.emplace_back("close", errno, IO_ERR_EXTERNAL);
        }
        throw std::runtime_error("");
    }
}

void IOWorkerHandler::quitAction() {
}

void IOWorkerHandler::timeoutAction() {
    if (!introduced) {
        if (close(main_fd)) {
            errs.emplace_back("close", errno, IO_ERR_EXTERNAL);
        }
        closedFd = true;
        terminate = true;
    }
    else {
        if (waitingForResponse) {
            if (!lastMsgSent->isRepeatable()) {
                waitingForResponse = false;
                responseTimeout.reset();
            }
            else {
                repeatCb(id, lastMsgSent);
            }
        }
    }
}

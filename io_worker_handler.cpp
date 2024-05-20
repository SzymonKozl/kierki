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
        IOWorkerSysErrCb error_callback,
        IOWorkerIntroCb intro_callback,
        IOWrokerTrickCb trick_callback,
        IOWorkerDisconnectCb disconnect_callback,
        net_address client_addr,
        net_address own_addr
):
        IOWorker(pipe_fd, id, sock_fd, exit_callback, error_callback),
        logger(std::cout, false),
        disconnectCb(std::move(disconnect_callback)),
        trickCb(std::move(trick_callback)),
        introCb(std::move(intro_callback)),
        client_loc(SIDE_NULL_),
        introduced(false),
        client_addr(std::move(client_addr)),
        own_addr(std::move(own_addr))
        {}

void IOWorkerHandler::pollAction() {
    std::string msg;
    try {
        msg = readUntilRN(main_fd);
    } catch (...) {
        // invalid message from Client, disconnecting
        int errno_cpy = errno;
        if (close(main_fd)) {
            errCb({"close", errno, IO_ERR_EXTERNAL});
        }
        if (introduced) disconnectCb(client_loc, {"pipe", errno_cpy, IO_ERR_EXTERNAL});
        return;
    }

    Message messageObj(client_addr, own_addr, msg);
    logger.log(messageObj);

    resp_array arr = parse_msg(msg, true);
    if (!arr.empty()) {
        if (arr[0].second == "IAM" && !introduced) {

            Side s = static_cast<Side>(arr[1].second.at(0));
            introduced = true;
            client_loc = s;
            introCb(s, id);
            return;
        }
        else if (arr[0].second == "TRICK_C") {
            int round_no = stoi(arr[1].second);
            trickCb(round_no, client_loc, Card::fromString(arr[2].second));
            return;
        }
    }
    else {
        if (close(main_fd)) {
            errCb({"close", errno, IO_ERR_EXTERNAL});
        }
        if (introduced) disconnectCb(client_loc, {"", 0, IO_ERR_NOERR});
        return;
    }
}

void IOWorkerHandler::quitAction() {
    if (introduced) disconnectCb(client_loc, {err, errno, err_type});
}

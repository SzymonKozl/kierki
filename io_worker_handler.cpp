//
// Created by szymon on 12.05.24.
//

#include "io_worker_handler.h"
#include "utils.h"
#include "common_types.h"
#include "network_msg_parser.h"

#include "string"
#include "stdexcept"
#include "unistd.h"
#include "cerrno"
#include "iostream"

IOWorkerHandler::IOWorkerHandler(
        int sock_fd,
        int pipe_fd,
        int id,
        IOWorkerExitCb exit_callback,
        IOWorkerSysErrCb error_callback,
        IOWorkerIntroCb intro_callback,
        IOWrokerTrickCb trick_callback,
        IOWorkerDisconnectCb disconnect_callback,
        net_address client_addr,
        net_address own_addr
):
        IOWorker(pipe_fd, id, sock_fd, exit_callback, error_callback),
        introCb(intro_callback),
        trickCb(trick_callback),
        disconnectCb(disconnect_callback),
        client_loc(_SIDE_NULL),
        introduced(false),
        client_addr(client_addr),
        own_addr(own_addr),
        logger(std::cout, false)
        {}

void IOWorkerHandler::pollAction() {
    std::string msg;
    try {
        msg = readUntilRN(main_fd);
    } catch (std::runtime_error) {
        // invalid message from client, disconnecting
        if (close(main_fd)) {
            errCb("close", errno, IO_ERR_INTERNAL, client_loc);
        }
        else exitCb(0);
        return;
    }

    Message messageObj()
    logger.log(msg);

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
            Card card = Card::fromString(arr[2].second);
            trickCb(round_no, client_loc, card);
            return;
        }
    }
    if (close(main_fd)) {
        errCb("close", errno, IO_ERR_EXTERNAL, client_loc);
    }
    else exitCb(0);
    return;
}

void IOWorkerHandler::disconnectAction() {
    disconnectCb(client_loc);
}

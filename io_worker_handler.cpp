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

IOWorkerHandler::IOWorkerHandler(
        int sock_fd,
        int pipe_fd,
        int id,
        IOWorkerExitCb &exit_callback,
        IOWorkerSysErrCb &error_callback,
        IOWorkerIntroCb &intro_callback,
        IOWrokerTrickCb &trick_callback,
        IOWorkerDisconnectCb &disconnect_callback):
        IOWorker(pipe_fd, id, sock_fd, exit_callback, error_callback),
        introCb(intro_callback),
        trickCb(trick_callback),
        disconnectCb(disconnect_callback),
        client_loc(E),
        introduced(false)
        {}

void IOWorkerHandler::pollAction() {
    std::string msg;
    try {
        msg = readUntilRN(main_fd);
    } catch (std::runtime_error) {
        // invalid message from client, disconnecting
        if (close(main_fd)) {
            errCb("close", errno, IO_ERR_INTERNAL);
        }
        else exitCb(0);
        return;
    }

    resp_array arr = parse_msg(msg, true);
    if (!arr.empty()) {
        if (arr[0].second == "IAM" && !introduced) {

            Side s = static_cast<Side>(arr[1].second.at(0));
            introduced = true;
            client_loc = s;
            introCb(s);
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
        errCb("close", errno, IO_ERR_EXTERNAL);
    }
    else exitCb(0);
    return;
}

void IOWorkerHandler::disconnectAction() {
    disconnectCb(client_loc);
}

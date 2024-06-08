//
// Created by szymon on 18.05.24.
//

#ifndef KIERKI_CONSTANTS_H
#define KIERKI_CONSTANTS_H

#include "cerrno"

constexpr int TRICKS_PER_ROUND = 13;

constexpr int DEFAULT_TIMEOUT = 5;
constexpr int TCP_QUEUE = 10;

constexpr size_t MAX_PARSE_LEN = 1000;

constexpr int SILENT_ERRS[] = {EPIPE, ECONNRESET, ECONNREFUSED, ENOTCONN};
constexpr int SILENT_ERRS_NO = sizeof (SILENT_ERRS) / sizeof (*SILENT_ERRS);

#endif //KIERKI_CONSTANTS_H

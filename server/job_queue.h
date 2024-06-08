//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_JOBQUEUE_H
#define KIERKI_JOBQUEUE_H

#include "../common/send_job.h"

#include "queue"
#include "mutex"
#include "cstddef"

class JobQueue {
public:
    JobQueue();

    SSendJob popNextJob();

    bool hasNextJob() noexcept;

    bool hasKillOrder() noexcept;

    void setKillOrder() noexcept;

    void pushNextJob(const SSendJob& job);

private:
    std::queue<SSendJob> jobsPending;
    std::mutex mutex;
    bool killFlag;
};


#endif //KIERKI_JOBQUEUE_H

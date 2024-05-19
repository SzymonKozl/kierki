//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_JOBQUEUE_H
#define KIERKI_JOBQUEUE_H

#include "send_job.h"

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

    void pushNextJob(SSendJob job);

    void setStopped(bool val) noexcept;

    bool isStopped() noexcept;

    size_t jobCount() noexcept;

private:
    bool killFlag;
    bool halt;
    std::queue<SSendJob> jobsPending;
    std::mutex mutex;
};


#endif //KIERKI_JOBQUEUE_H

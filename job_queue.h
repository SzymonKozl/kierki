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

    SendJob popNextJob();

    const bool hasNextJob() noexcept;

    bool hasKillOrder() noexcept;

    void setKillOrder() noexcept;

    void pushNextJob(SendJob job);

    void setStopped(bool val) noexcept;

    bool isStopped() noexcept;

    size_t jobCount();

private:
    bool killFlag;
    bool stopped;
    std::queue<SendJob> jobsPending;
    std::mutex mutex;
};


#endif //KIERKI_JOBQUEUE_H

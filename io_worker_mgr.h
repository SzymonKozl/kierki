//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_MGR_H
#define KIERKI_IO_WORKER_MGR_H

#include "io_worker.h"
#include "unordered_map"

#include "type_traits"
#include "functional"
#include "string"
#include "thread"

using IOWorkerMgrPipeCb = std::function<void(std::string, int)>;

class IOWorkerMgr {
public:
    template<class T, class... Args> requires std::is_base_of_v<IOWorker, T>
    void spawnNewWorker(Args... args);
    void sendKill(int ix);
    void killAll();
    void sendJob(SendJob job, int ix);
    void eraseWorker(int ix);
    IOWorkerMgr(IOWorkerMgrPipeCb &&pipeCb);
    ~IOWorkerMgr();
private:
    void signal(int ix);

    std::unordered_map<int, IOWorker> workers;
    std::unordered_map<int, std::pair<int, int>> pipes;
    std::unordered_map<int, std::jthread> threads;
    int nextIx;
    IOWorkerMgrPipeCb pipeCb;
};


#endif //KIERKI_IO_WORKER_MGR_H

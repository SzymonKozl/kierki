//
// Created by szymon on 15.05.24.
//

#ifndef KIERKI_CLIENT_AUTO_H
#define KIERKI_CLIENT_AUTO_H

#include "client.h"
#include "strategy.h"

class ClientAuto: Client {
public:
    ClientAuto();

    Card trickMsg(int trickNo, const Table& table) override;
    void takenMsg(Side side, const Table& cards) override;
    void scoreMsg(const std::unordered_map<Side, int> scores, bool total) override;
    void dealMsg(int trickMode, Hand hand) override;
private:
    Strategy strategy;
};


#endif //KIERKI_CLIENT_AUTO_H

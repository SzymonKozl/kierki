//
// Created by szymon on 16.05.24.
//

#ifndef KIERKI_GAMERULES_H
#define KIERKI_GAMERULES_H

#include "../common/common_types.h"
#include "../common/card.h"

#include "unordered_map"

class GameRules {
public:
    static bool isMoveLegal(Side &s, Card &c, table_state &state, const Table& table);

    static std::pair<Side, int> whoTakes(Side const& startingSide, Table const& table, RoundType roundType, int trickNo);
};

#endif //KIERKI_GAMERULES_H

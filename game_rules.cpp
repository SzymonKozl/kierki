//
// Created by szymon on 16.05.24.
//

#include "game_rules.h"

#include "common_types.h"

bool GameRules::isMoveLegal(Side &s, Card &c, table_state &state) {
    // todo
    return true;
}

std::pair<Side, int> GameRules::whoTakes(const Side &starting_side, const Table &table, RoundType roundType, int trickNo) {
    // todo
    return std::make_pair(starting_side, 10);
}
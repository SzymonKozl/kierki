//
// Created by szymon on 16.05.24.
//

#include "game_rules.h"
#include "utils.h"
#include "common_types.h"

constexpr int TRICK_PENALTY_VAL = 1;
constexpr int HEART_PENALTY_VAL = 1;
constexpr int QUEEN_PENALTY_VAL = 5;
constexpr int KING_JACK_PENALTY_VAL = 2;
constexpr int KING_HEART_PENALTY_VAL = 18;
constexpr int SEVENTH_LAST_TRICK_PENALTY_VAL = 10;

bool GameRules::isMoveLegal(Side &s, Card &c, table_state &state, const Table& table) {
    if (!table.empty()) {
        color wanted = table.front().getColor();
        if (c.getColor() != wanted) {
            for (const Card& card: state.at(s)) {
                if (card.getColor() == wanted) return false;
            }
        }
    }
    for (const Card& card: state.at(s)) {
        if (card.getColor() == c.getColor() && card.getValue() == c.getValue()) return true;
    }
    return false;
}

std::pair<Side, int> GameRules::whoTakes(const Side &starting_side, const Table &table, RoundType roundType, int trickNo) {
    int takerIx = 0;
    color wanted = table.front().getColor();
    for (int i = 1; i < 4; i ++) {
        if (table[i].getColor() == wanted && table[i] > table[takerIx]) takerIx = i;
    }
    Side takerSide = starting_side;
    for (int i = 0; i < takerIx; i ++) takerSide = nxtSide(takerSide);
    int pen;
    switch (roundType) {
        case TRICK_PENALTY: {
            return {takerSide, TRICK_PENALTY_VAL};
        }
        case HEART_PENALTY: {
            pen = 0;
            for (const Card& c: table) if (c.getColor() == COLOR_H) pen += HEART_PENALTY_VAL;
            return {takerSide, pen};
        }
        case QUEEN_PENALTY: {
            pen = 0;
            for (const Card& c: table) if (c.getValue() == "Q") pen += QUEEN_PENALTY_VAL;
            return {takerSide, pen};
        }
        case KING_JACK_PENALTY: {
            pen = 0;
            for (const Card& c: table) if (c.getValue() == "K" || c.getValue() == "J") pen += KING_JACK_PENALTY_VAL;
            return {takerSide, pen};
        }
        case HEART_KING_PENALTY: {
            pen = 0;
            for (const Card& c: table) if (c.getValue() == "K" && c.getColor() == COLOR_H) pen += KING_HEART_PENALTY_VAL;
            return {takerSide, pen};
        }
        case SEVENTH_LAST_TRICK_PENALTY: {
            return {takerSide, (trickNo == 7 || trickNo == TRICKS_PER_ROUND) ? SEVENTH_LAST_TRICK_PENALTY_VAL : 0};
        }
        case EVERYTHING: {
            pen = TRICK_PENALTY_VAL;
            pen += (trickNo == 7 || trickNo == TRICKS_PER_ROUND) ? SEVENTH_LAST_TRICK_PENALTY_VAL : 0;
            for (const Card& c: table) {
                if (c.getColor() == COLOR_H) pen += HEART_PENALTY_VAL;
                if (c.getValue() == "Q") pen += QUEEN_PENALTY_VAL;
                if (c.getValue() == "K" || c.getValue() == "J") pen += KING_JACK_PENALTY_VAL;
                if (c.getValue() == "K" && c.getColor() == COLOR_H) pen += KING_HEART_PENALTY_VAL;
            }
            return {takerSide, pen};
        }
    }
}

//
// Created by szymon on 15.05.24.
//

#include "player.h"

#include <utility>

Player::Player()
{}

void Player::setup(putCardCb putCardCallback, cardNeededCb cardNeededCallback) {
    putCb = std::move(putCardCallback);
    checkCardCb = std::move(cardNeededCallback);
}
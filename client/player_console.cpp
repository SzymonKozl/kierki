//
// Created by szymon on 25.05.24.
//

#include "player_console.h"
#include "../common/utils.h"

void PlayerConsole::trickMsg(int trickNo, const Table &table) {
    std::cout << "Trick: " << trickNo << " ";
    printList(table);
    std::cout << "\nAvailable: ";
    printList(localHand);
    std::cout << std::endl << std::flush;
}

void PlayerConsole::takenMsg(
        Side side,
        const Table &cards,
        int trickNo,
        bool apply
        ) {
    std::cout << "A trick " << trickNo << " is taken by " << side << ", cards ";
    printList(cards);
    if (side == inGameSide) takenTricks.emplace_back(side, cards);
    if (apply) {
        rmIntersection(localHand, cards);
    }
    else {
        if (!lastCards.empty()) {
            lastCards.pop_front();
        } else {
            std::cerr << "warn: should remove card\n";
        }
    }
    std::cout << ".\n" << std::flush;
}

void PlayerConsole::scoreMsg(const score_map scores, bool total) {
    if (!total) {
        std::cout << "The scores are:\n";
    }
    else {
        std::cout << "The total scores are:\n";
    }
    for (Side s: sides_) {
        std::cout << s << " | " << scores.at(s) << '\n';
    }
    std::cout << std::flush;
    takenTricks.clear();
}

void PlayerConsole::dealMsg(int trickMode, const Hand& hand, Side starting) {
    std::cout << "New deal " << trickMode << " staring place " <<
        starting << ", your cards: ";
    printList(hand);
    std::cout << ".\n" << std::flush;
    localHand = hand;
}

void PlayerConsole::wrongMsg(int trickNo) {
    std::cout << "Wrong message received in trick " << trickNo << ".\n" <<
        std::flush;
    if (!lastCards.empty()) {
        localHand.push_back(*lastCards.front());
        lastCards.pop_front();
    }
    else {
        std::cerr << "warn: should remove card\n" << std::flush;
    }
}

void PlayerConsole::anyMsg(Message) {
}

void PlayerConsole::anyCmd(std::string msg) {
    if (msg.at(0) == '!') {
        try {
            Card c = Card::fromString(msg.substr(1));
            if (!checkCardCb()) {
                std::cout << "No card request from server."
                             " Card select action is discarded\n" << std::flush;
                return;
            }
            bool found  = false;
            for (const Card& card: localHand) {
                if (card.getColor() == c.getColor()
                    && card.getValue() == c.getValue()) found = true;
            }
            if (!found) {
                std::cout << "Card " << c.toString() <<
                    " is not in your hand.\nAvailable cards: ";
                printList(localHand);
                std::cout << ".\n" << std::flush;
                return;
            }
            rmCardIfPresent(localHand, c);
            lastCards.push_back(std::make_shared<Card>(c.getValue(), c.getColor()));
            putCb(c);
        } catch (...) {
            std::cout << msg.substr(1) << " is not a valid card!\n" << std::flush;
        }
    }
    else if (msg == "cards") {
        printList(localHand);
        std::cout << "\n" << std::flush;
    }
    else if (msg == "tricks") {
        for (auto& trick: takenTricks) {
            printList(trick.second);
            std::cout << "\n";
        }
        std::cout << std::flush;
    }
    else {
        std::cout << "Unknown command: " << msg << std::endl << std::flush;
    }
}

void PlayerConsole::busyMsg(const std::vector<Side>& taken) {
    std::cout << "Place busy, list of busy places received: ";
    printList(taken);
    std::cout << ".\n" << std::flush;
}

PlayerConsole::PlayerConsole(Side side) : Player(side) {

}

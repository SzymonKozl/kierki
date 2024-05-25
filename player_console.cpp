//
// Created by szymon on 25.05.24.
//

#include "player_console.h"
#include "utils.h"
#include "common_types.h"
#include "card.h"

void PlayerConsole::trickMsg(int trickNo, const Table &table) {
    std::cout << "Trick: " << trickNo << " ";
    printList(table);
    std::cout << "\nAvailable: ";
    printList(localHand);
    std::cout << std::endl << std::flush;
}

void PlayerConsole::takenMsg(Side side, const Table &cards, int trickNo, bool apply) {
    std::cout << "A trick " << trickNo << " is taken by " << side << ", cards ";
    printList(cards);
    takenTricks.emplace_back(side, cards);
    if (apply) {
        rmIntersection(localHand, cards);
    }
    std::cout << ".\n";
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
    takenTricks.clear();
}

void PlayerConsole::dealMsg(int trickMode, const Hand& hand, Side starting) {
    std::cout << "New deal " << trickMode << " staring place " << starting << ", your cards: ";
    printList(hand);
    std::cout << ".\n";
    localHand = hand;
}

void PlayerConsole::wrongMsg(int trickNo) {
    std::cout << "Wrong message received in trick " << trickNo << ".\n";
    if (lastCardGiven.use_count()) {
        Card t = *lastCardGiven;
        localHand.emplace_back(t.getValue(), t.getColor());
        lastCardGiven.reset();
    }
}

void PlayerConsole::anyMsg(Message message) {
}

void PlayerConsole::anyCmd(std::string msg) {
    if (msg.at(0) == '!') {
        try {
            Card c = Card::fromString(msg.substr(1));
            if (!checkCardCb()) {
                std::cout << "No card request from server. Card select action is discarded\n" << std::flush;
                return;
            }
            bool found  = false;
            for (const Card& card: localHand) {
                if (card.getColor() == c.getColor() && card.getValue() == c.getValue()) found = true;
            }
            if (!found) {
                std::cout << "Card " << c.toString() << " is not in your hand.\nAvailable cards: ";
                printList(localHand);
                std::cout << ".\n" << std::flush;
                return;
            }
            rmCardIfPresent(localHand, c);
            lastCardGiven = std::make_shared<Card>(c.getValue(), c.getColor());
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

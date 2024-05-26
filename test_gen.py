import random
import sys
from random import seed, shuffle
from typing import List, Tuple


seed(1965)

HAND_SIZE = 13
SIDES = ['W', 'E', 'S', 'N']

COLORS = ['C', 'H', 'D', 'S']
VALUES = [str(x) for x in range(2, 11)] + ['J', 'Q', 'K', 'A']
CARDS = [str(v) + str(c) for c in COLORS for v in VALUES]


def random_hands() -> Tuple[List[str], List[str], List[str], List[str]]:
    cards = CARDS[:]
    shuffle(cards)
    return cards[:HAND_SIZE], cards[HAND_SIZE:2 * HAND_SIZE], cards[-2 * HAND_SIZE:- HAND_SIZE], cards[-HAND_SIZE:]


def random_side() -> str:
    return random.choice(SIDES)


def generate_gameplay(tgt: str, modes: List[int], starting: List[str] = None) -> None:
    if starting is None:
        starting = [random_side() for _ in modes]
    with open(tgt, "w") as f:
        for i, (start, mode) in enumerate(zip(starting, modes)):
            hands = random_hands()
            f.write(f"{mode}{start}\n")
            for j, h in enumerate(hands):
                f.write("".join(h))
                if i < len(starting) - 1 or j < 3:
                    f.write('\n')


if __name__ == '__main__':
    generate_gameplay("scenarios/whole_game.scn", [x for x in range(1, 8)])

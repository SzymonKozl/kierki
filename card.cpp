//
// Created by szymon on 17.05.24.
//

#include "card.h"

#include "unordered_map"
#include "string"

const std::unordered_map<std::string, int> VALUES {
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {"5", 5},
        {"6", 6},
        {"7", 7},
        {"8", 8},
        {"9", 9},
        {"10", 10},
        {"J", 11},
        {"Q", 12},
        {"K", 13},
        {"A", 14}
};

Card::Card(const std::string& value, color col):
    value(value),
    col(col),
    true_val(VALUES.at(value))
{}

std::string Card::toString() const {
    std::string res(value);
    return res + (char) col;
}

color Card::getColor() const noexcept {
    return col;
}

std::string Card::getValue() const {
    std::string res(value);
    return res;
}

Card Card::fromString(const std::string &s) {
    auto col = (color) s.at(s.size() - 1);
    std::string val = s.substr(0, s.size() - 1);
    return {val, col};
}

bool Card::isValueOk(const std::string& v) {
    return VALUES.contains(v);
}

bool Card::operator==(const Card &other) const {
    return true_val == other.true_val;
}

bool Card::operator!=(const Card &other) const {
    return !(*this == other);
}

bool Card::operator<(const Card &other) const {
    return true_val < other.true_val;
}

bool Card::operator<=(const Card &other) const {
    return (*this == other) || (*this < other);
}

bool Card::operator>(const Card &other) const {
    return !(*this <= other);
}

bool Card::operator>=(const Card &other) const {
    return !(*this < other);
}

std::ostream &operator<<(std::ostream &stream, const Card &card) {
    return stream << card.getValue() << (char) card.getColor();
}

Card &Card::operator=(const Card &other) = default;

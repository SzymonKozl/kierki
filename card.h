//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_CARD_H
#define KIERKI_CARD_H

#include "string"
#include "vector"

enum color {
    COLOR_C = 'C',
    COLOR_D = 'D',
    COLOR_H = 'H',
    COLOR_S = 'S'
};

class Card {
public:
    static Card fromString(const std::string &s);
    std::string toString() const;
    color getColor() const noexcept;
    std::string& getValue() const;
    Card() = delete;
    static bool isValueOk(std::string v);

    bool operator<(const Card& other);
    bool operator>(const Card& other);
    bool operator>=(const Card& other);
    bool operator<=(const Card& other);
    bool operator==(const Card& other);
    bool operator!=(const Card& other);

    Card& operator=(Card const& other);

private:
    static const std::vector<char> AVAILABLE_COLORS;
    static const std::vector<std::string> AVAILABLE_VALUES;
    const color col;
    const std::string value;
};


#endif //KIERKI_CARD_H

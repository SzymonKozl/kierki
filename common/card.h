//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_CARD_H
#define KIERKI_CARD_H

#include "string"
#include "vector"
#include "memory"

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
    std::string getValue() const;
    Card(const std::string& value, color col);
    static bool isValueOk(const std::string& v);

    bool operator<(const Card& other) const;
    bool operator>(const Card& other) const;
    bool operator>=(const Card& other) const;
    bool operator<=(const Card& other) const;
    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;

    friend std::ostream& operator<< (std::ostream& stream, const Card& card);

private:
    static const std::vector<char> AVAILABLE_COLORS;
    static const std::vector<std::string> AVAILABLE_VALUES;
    std::string value;
    color col;
    int true_val;
};



using sCard = std::shared_ptr<Card>;


#endif //KIERKI_CARD_H

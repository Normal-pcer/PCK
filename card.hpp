#pragma once
#ifndef CARD_HEADER
#define CARD_HEADER

#include "enums.hpp"

namespace PCK {
class Player;
class Game;

// 卡牌
class Card {
    CardLabel label;
public:
    Card(CardLabel label): label(label) {}

    auto getLabel() const -> CardLabel { return label; };
    auto execute(Player &user, Player *target = nullptr, Game *game = nullptr) -> void;
};
}

#endif
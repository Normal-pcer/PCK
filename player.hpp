#pragma once
#ifndef PLAYER_HEADER
#define PLAYER_HEADER
#include <memory>
#include <variant>
#include <vector>

#include "util.hpp"
#include "enums.hpp"
#include "card.hpp"

namespace PCK {
class Game;

// 策略接口定义
class IDesignant {
public:
    struct Result;

    virtual ~IDesignant() = default;
    auto virtual canProvoke(PlayerRole role) -> bool = 0;
    auto virtual canFlatter(PlayerRole role) -> bool = 0;
    auto virtual responseDuel(Player& source) -> bool = 0;
    
    auto virtual selectCard(Game &game, bool enableKilling) -> Result = 0;
    auto virtual inKilling(Player &source, Game &game) -> Result = 0;
    auto virtual inAoe(Player &source, Game &game, CardLabel type) -> Result = 0;

    auto virtual inBlockingTrick(Player &source, Player &target, Game &game, bool friendly) -> Result = 0;
};

class Player;

auto createDesignant(Player *player) -> std::unique_ptr<IDesignant>;

class Player {
public:
    // 玩家基本信息定义
    i32 id{};                                           // 玩家编号
    i32 health{};                                       // 玩家生命值
    i32 maxHealth = 4;                                  // 最大生命值
    PlayerRole role = PlayerRole::Undefined;            // 玩家角色
    PlayerRole impression = PlayerRole::Undefined;      // 跳忠/跳反状态（包含“类反猪”）
    bool alive = true;                                  // 存活状态
    bool weapon = false;                                // 武器状态

    std::unique_ptr<IDesignant> designant;
    
    Player(i32 id, PlayerRole role)
        : id(id), role(role), designant(createDesignant(this)) {
        health = maxHealth;  // 初始满生命值
        if (role == PlayerRole::M_Main) impression = role;
    }

    // 手牌管理
    struct CardManager {
        Player *super;

        using CardList = std::vector<Card>;
        CardList cards{};

        auto draw(Game &game, i32 n) -> void;
        auto findCard(CardLabel label) -> CardList::iterator;
        template <typename ...Ts>
        auto useCard(CardLabel label, Ts &&...args) -> bool;
        auto erase(CardList::iterator iter) -> void;
    } cardManager{this};
    friend struct CardManager;

    auto damaged(i32 amount, DamageType type, Player &source, Game &game) -> void;
    auto camp() const -> PlayerRole;
    auto play(Game &game) -> void;
};

struct IDesignant::Result {
    struct UseCard {
        using CardList = Player::CardManager::CardList;
        CardList::iterator iter;
    };

    struct None {};

    std::variant<None, UseCard> value{};

    template <typename T>
    auto visit(T &&func) -> decltype(auto) {
        return std::visit(std::forward<T>(func), value);
    }

    template <typename T>
    auto get() -> T & {
        return std::get<T>(value);
    }

    template <typename T>
    auto get_if() -> T * {
        return std::get_if<T>(&value);
    }
};
}
#endif
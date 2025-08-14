#pragma once
#ifndef GAME_HEADER
#define GAME_HEADER

#include <algorithm>
#include <deque>
#include <vector>

#include "util.hpp"
#include "concat_view.hpp"
#include "enums.hpp"
#include "player.hpp"

namespace PCK {
namespace ranges = std::ranges;
namespace views = std::views;

// 通过异常处理游戏结束
struct GameOver: std::exception {
    PlayerRole winner;
    GameOver(PlayerRole winner): winner(winner) {}
    auto what() const noexcept -> char const * override {
        return "Game Over";
    }
};

// 游戏
class Game {
    std::vector<Player> players;                // 玩家列表，在此处唯一管理
    std::deque<Card> deck;                      // 牌堆
public:
    i32 thiefCount = 0;                         // 反猪数量
    Game(
        std::vector<Player> players_,
        std::deque<Card> deck_
    ): players(std::move(players_)), deck(std::move(deck_)) {
        thiefCount = static_cast<i32>(
            ranges::count_if(players, lam(const &pl, pl.role == PlayerRole::F_Thief)));
    }

    // 抽牌
    auto drawCard() -> Card;

    auto getPlayersFrom(Player &player, bool hasThis = false) -> auto;
    auto round() -> void;
    auto print() -> void;
    auto blockTrick(Player &source, Player &target, bool friendly = false) -> bool;
};

// 获取从当前玩家的下一个玩家开始，按照逆时针方向的存活玩家列表。
// 该列表中可以指定是否存在当前玩家。（默认不存在）
// 例如，1 2 3 4 5(死亡) 6，传入 player = 2。
// 返回：3 4 5 6 1。
auto inline Game::getPlayersFrom(Player &player, bool hasThis) -> auto {
    auto id = player.id;
    return concat_view(
        ranges::subrange{players.begin() + id + i32(not hasThis), players.end()},
        ranges::subrange{players.begin(), players.begin() + id}
    ) | views::filter(lam(const &p, p.alive));
}
}

#endif
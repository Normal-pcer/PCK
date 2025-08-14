#ifndef GAME_IMPL_HEADER
#define GAME_IMPL_HEADER

#include "util.hpp"
#include "game.hpp"
#include "player.hpp"

namespace PCK {
auto inline Game::drawCard() -> Card {
    auto card = deck.front();
    if (deck.size() > 1) deck.pop_front();

    return card;
}

auto inline Game::round() -> void {
    for (auto &pl: players) {
        if (not pl.alive) continue;
        pl.play(*this);
    }
}

auto inline Game::print() -> void {
    for (auto &pl: players) {
        if (pl.alive) {
            for (auto &c: pl.cardManager.cards) {
                std::cout << static_cast<char>(c.getLabel()) << ' ';
            }
            std::cout << endl;
        } else {
            std::cout << "DEAD" << endl;
        }
    }
}

// 尝试通过无懈可击，阻止一张锦囊牌。
// 返回是否阻止成功。
// source 向 target 使用了一张锦囊牌，friendly 标识这个操作是向 target 献殷勤还是表敌意。
auto inline Game::blockTrick(Player &source, Player &target, bool friendly) -> bool {
    // 如果没有亮身份，一定无法被无懈可击阻止
    if (target.impression < leastShowedRole) {
        return false;
    }

    // 按顺序，所有人都有机会使用一次无懈可击
    for (auto &pl: getPlayersFrom(source, true)) {
        auto flag = (
            (friendly and pl.designant.canProvoke(target.impression)) or
            (not friendly and pl.designant.canFlatter(target.impression))
        );  // 可以执行无懈可击
        if (flag and pl.cardManager.useCard(CardLabel::J_Unbreakable)) {
            pl.impression = pl.role;
            // 这次无懈可击本身没有被无效化
            return not blockTrick(pl, target, not friendly);
        }
    }

    return false;
}
}
#endif

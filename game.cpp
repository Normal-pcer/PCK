#include "enums.hpp"
#include "util.hpp"
#include "game.hpp"
#include "player.hpp"

namespace PCK {
auto Game::drawCard() -> Card {
    auto card = deck.front();
    if (deck.size() > 1) deck.pop_front();

    return card;
}

auto Game::round() -> void {
    for (auto &pl: players) {
        if (not pl.alive) continue;
        pl.play(*this);

        std::cout << std::format("after Player {}\n", pl.id);
        print();
    }
}

auto Game::print() -> void {
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
auto Game::blockTrick(Player &source, Player &target, bool friendly) -> bool {
    // 如果没有亮身份，一定无法被无懈可击阻止
    if (target.impression < leastShowedRole) {
        return false;
    }

    // 按顺序，所有人都有机会使用一次无懈可击
    for (auto &pl: getPlayersFrom(source, true)) {
        auto res = pl.designant->inBlockingTrick(source, target, *this, friendly);
        if (auto *p = res.get_if<IDesignant::Result::UseCard>();
                p != nullptr and p->iter->getLabel() == CardLabel::J_Unbreakable) {
            pl.cardManager.erase(p->iter);
            pl.impression = pl.role;
            // 这次无懈可击本身没有被无效化
            return not blockTrick(pl, target, not friendly);
        }
    }

    return false;
}
}

#include <iostream>
#include <cassert>

#include "util.hpp"
#include "card.hpp"
#include "player.hpp"
#include "game.hpp"

namespace PCK {
// 所有卡牌的实现
namespace CardImpl {
    auto test() -> void {
        std::cout << "TestCard execute" << endl;
    }
    auto killing(Player &user, Player &target, Game &game) -> void {
        auto res = target.designant->inKilling(user, game);
        if (auto *p = res.get_if<IDesignant::Result::UseCard>()) {
            auto it = p->iter;

            if (it->getLabel() == CardLabel::D_Dodge) {
                user.cardManager.erase(it);
                return;
            }
        }
        // 闪不开，只能掉血
        target.damaged(1, DamageType::Killing, user, game);
    }
    auto peach(Player &user) -> void {
        assert(user.health != user.maxHealth);
        ++user.health;
    }
    auto dodge() -> void {
        // “闪”没有效果
    }
    auto crossbow(Player &user) -> void {
        user.weapon = true;
    }
    // 类似南猪入侵的两类牌
    // 对除了自己以外的所有人，只有丢弃一张 type 才能免伤
    auto aoeAttack(Player &user, Game &game, CardLabel type) -> void {
        auto targets = game.getPlayersFrom(user);
        for (auto &target: targets) {
            // 可以被无懈可击阻止
            if (game.blockTrick(user, target, false)) continue;
            // 弃置一张指定牌，或者生命值 -1
            auto res = target.designant->inAoe(user, game, type);
            if (auto *p = res.get_if<IDesignant::Result::UseCard>(); 
                    p != nullptr and p->iter->getLabel() == type) {
                user.cardManager.erase(p->iter);
            } else {
                target.damaged(1, DamageType::Invading, user, game);
            }
        }
    }
    auto invasion(Player &user, Game &game) -> void {
        aoeAttack(user, game, CardLabel::K_Killing);
    }
    auto arrows(Player &user, Game &game) -> void {
        aoeAttack(user, game, CardLabel::D_Dodge);
    }
    auto unbreakable() -> void {
        // “无懈可击”不应主动调用，被动调用时无效果
    }
    auto duel(Player &user, Player &target, Game &game) -> void {
        // 二者轮流弃置杀，直到一方弃置失败。
        // 失败的一方受到伤害。

        // 开局进行一次“挑衅”，然后正式决斗
        target.damaged(0, DamageType::Dueling, user, game);

        // 锦囊牌可以被无懈可击无效化
        // 即使是无效化，也依旧视作表敌意
        if (game.blockTrick(user, target, false)) {
            return;
        }

        auto recur = [&](auto &&recur, Player &cur, Player &oppo) -> void {
            // 轮到 cur 出牌
            // 如果 ta 想要出牌，并且手里有牌
            if (cur.designant->responseDuel(oppo)) {
                // 选择一张杀
                auto &cards = cur.cardManager.cards;
                auto it = cur.cardManager.findCard(CardLabel::K_Killing);
                if (it != cards.end()) {
                    // 弃置这张牌
                    cards.erase(it);
                    // 继续决斗
                    recur(recur, oppo, cur);  // NOLINT(readability-suspicious-call-argument)
                    return;  // 成功出牌，结束当前递归
                }
            }
            // 不出牌，直接失败
            cur.damaged(1, DamageType::DuelingFailed, oppo, game);
        };

        recur(recur, target, user);
    }
}
auto Card::execute(Player &user, Player *target, Game *game) -> void {
    // 使用传统的 switch-case 转发
    using namespace CardImpl;
    switch (label) {
        case CardLabel::D_Dodge: dodge(); return;
        case CardLabel::F_Dueling: duel(user, *target, *game); return;
        case CardLabel::J_Unbreakable: unbreakable(); return;
        case CardLabel::K_Killing: killing(user, *target, *game); return;
        case CardLabel::N_Invasion: invasion(user, *game); return;
        case CardLabel::P_Peach: peach(user); return;
        case CardLabel::T_Test: test(); return;
        case CardLabel::W_Arrows: arrows(user, *game); return;
        case CardLabel::Z_Crossbow: crossbow(user); return;
        default: PANIC("Unknown card label");
    }
}
}

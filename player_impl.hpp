#ifndef PLAYER_IMPL_HEADER
#define PLAYER_IMPL_HEADER

#include "util.hpp"
#include "game.hpp"
#include "player.hpp"

namespace PCK {
// 抽 n 张卡。
// 可能修改：cards。
auto inline Player::CardManager::draw(Game &game, i32 n) -> void {
    for (i32 i = 0; i < n; ++i) {
        cards.push_back(game.drawCard());
    }
}
// 寻找一张指定标签的卡。
auto inline Player::CardManager::findCard(CardLabel label) -> CardList::iterator {
    return ranges::find(cards, label, lam(x, x.getLabel()));
}
// 寻找指定标签的卡牌，然后：
// - 如果存在，使用并弃置，返回 true
// - 如果不存在，返回 false
// 可能修改 cards。
template <typename ...Ts>
auto inline Player::CardManager::useCard(CardLabel label, Ts &&...args) -> bool {
    auto it = findCard(label);
    if (it != cards.end()) {
        // 预先复制，避免在 *it 上同时读写
        auto copy = *it;
        cards.erase(it);
        copy.execute(*super, std::forward<Ts>(args)...);
        return true;
    }
    return false;
}

// 玩家受到伤害。
// 同时会进行跳反、跳忠等处理，以及后续奖惩逻辑。
// 如果游戏结束，直接抛出异常报告。
// 可能修改：user 和 target 的 cards。
auto inline Player::damaged(i32 amount, DamageType type, Player &source, Game &game) -> void {
    health -= amount;

    // 尝试吃桃免伤
    while (health <= 0) {
        if (not cardManager.useCard(CardLabel::P_Peach)) {
            break;  // 被耗尽
        }
    }

    if (health <= 0) {
        alive = false;
    }

    // 判断游戏结束
    if (not alive) {
        if (role == PlayerRole::M_Main) {
            throw GameOver{PlayerRole::F_Thief};
        }
        if (role == PlayerRole::F_Thief) {
            --game.thiefCount;
            if (game.thiefCount <= 0) throw GameOver{PlayerRole::M_Main};
        }
    }

    // 按照自己的身份进行跳忠/跳反判定
    if (role == PlayerRole::M_Main) {
        // 类反猪判定
        if (source.impression == PlayerRole::Undefined and 
                type >= DamageType::DuelingFailed) {
            source.impression = PlayerRole::Questionable;
        }
    }
    bool strong = (type >= DamageType::Dueling);  // 本次攻击为表敌意
    if (strong) {
        // 获取表敌意之后的印象，如果不是 undefined 就应用
        chkMax(source.impression, -camp());
    }

    // 额外奖惩机制
    if (not alive) {
        if (role == PlayerRole::F_Thief) {
            i32 constexpr bonus = 3;  // 奖励摸牌数量
            source.cardManager.draw(game, bonus);
        } else if (role == PlayerRole::Z_Minister and source.role == PlayerRole::M_Main) {
            // 执行惩罚（丧失手牌和武器）
            source.cardManager.cards.clear();
            source.weapon = false;
        }
    }
}
// 判断玩家阵营，对当前玩家献殷勤属于跳忠还是跳反。
// 即：对当前玩家献殷勤之后，会让自己的 impression 变成什么。
// 如果要判断表敌意，对结果取反即可。
auto inline Player::camp() const -> PlayerRole {
    // 跳忠：对主猪/跳忠的忠猪献殷勤
    if (role == PlayerRole::M_Main or impression == PlayerRole::Z_Minister) {
        return PlayerRole::Z_Minister;
    }
    // 跳反：对跳反的反猪献殷勤
    if (impression == PlayerRole::F_Thief) {
        return PlayerRole::F_Thief;
    }
    return PlayerRole::Undefined;
}
// 开始该玩家的回合
auto inline Player::play(Game &game) -> void {
    // 摸牌阶段
    cardManager.draw(game, 2);

    // 出牌阶段
    // 可以使用任意张牌，每次都需要使用最左侧的可用卡牌
    bool usedKilling = false;  // 如果没有武器，只能使用一次杀

    // 选定并使用一张卡牌，返回过程是否成功
    auto select = [&]() -> bool {
        auto &cards = cardManager.cards;
        for (auto it = cards.begin(); it != cards.end(); ++it) {
            // 判断是否可用
            if (auto res = designant.tryCard(*it, game); res.use()) {
                if (it->getLabel() == CardLabel::K_Killing) {
                    if (usedKilling and not weapon) continue;  // 没有武器，只能“杀”一次
                    usedKilling = true;
                }
                auto copy = *it;
                cards.erase(it);
                copy.execute(*this, res.target, &game);

                return true;
            }
        }
        return false;
    };

    // 直到无法继续出牌
    while (select()) {
        if (not alive) break;
    }
}

auto inline Player::Designant::resolveKill(Card card, Game &game) const -> Decision {
    if (card.getLabel() != CardLabel::K_Killing) return {};
    // 后面的第一个玩家
    auto &target = *game.getPlayersFrom(*super).begin();
    if (canProvoke(target.impression)) {
        return {Decision::Use, &target};
    }
    return {Decision::Skip};
}

auto inline Player::Designant::resolveDuel(Card card, Game &game) const -> Decision {
    if (card.getLabel() != CardLabel::F_Dueling) return {};
    auto *target = selectTarget(game);

    if (target == nullptr) {
        return {Decision::Skip};  // 无法决斗
    }
    return {Decision::Use, target};
}

auto inline Player::Designant::selectTarget(Game &game) const -> Player * {
    auto getFirst = [&](auto &&pred) -> Player * {
        for (auto &pl: game.getPlayersFrom(*super)) {
            if (pred(pl.impression)) return &pl;
        }
        return nullptr;
    };
    if (super->role == PlayerRole::F_Thief) {
        // 特殊处理反猪
        if (auto *res = getFirst(lam(x, x == PlayerRole::M_Main)); res != nullptr)
            return res;
        if (auto *res = getFirst(lam(x, x == PlayerRole::F_Thief)); res != nullptr)
            return res;
        return nullptr;
    }
    return getFirst(lam(x, canProvoke(x)));
}

auto inline Player::Designant::responseDuel(Player &source) const -> bool {
    // 仅有“忠猪不打主猪”一条例外，否则都会尽力决斗
    return super->role != PlayerRole::Z_Minister or source.role != PlayerRole::M_Main;
}
}

#endif

#pragma once
#ifndef PLAYER_HEADER
#define PLAYER_HEADER
#include <vector>

#include "util.hpp"
#include "enums.hpp"
#include "card.hpp"

namespace PCK {
class Game;

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

    Player(i32 id, PlayerRole role)
        : id(id), role(role) {
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
    } cardManager{this};
    friend struct CardManager;

    // 玩家出牌策略。
    // 命名原因：词源为 designate，-ant 后缀表示“...的人”。词义为“指定者”“操纵者”。
    // 由于功能较为固定，由策略模式重构为类中命名空间
    struct Designant {
        Player *super{};
        
        // 决定一张牌是否使用的结果
        struct Decision {
            enum Type: i8 {
                Unresolved,  // 未决定
                Skip,  // 不使用
                Use,  // 使用
            } type = Unresolved;
            Player *target = nullptr;  // 选中的目标

            auto resolved() const -> bool {
                return type != Unresolved;
            }

            auto use() const -> bool {
                return type == Use;
            }
        };

        // 处理“直接可行”或者“直接不可行”的几种技能卡。
        // 例如：猪哥连弩，南猪入侵等。
        // 如果完成处理，返回 true/false 表示是否可行。
        // 否则未处理，返回 nullopt。
        auto direct(Card card, Game &) const -> Decision {
            switch (card.getLabel()) {
            case CardLabel::J_Unbreakable: [[fallthrough]];
            case CardLabel::D_Dodge:
                return {Decision::Skip};  // 一定不会主动使用
            case CardLabel::N_Invasion: [[fallthrough]];
            case CardLabel::W_Arrows: [[fallthrough]];
            case CardLabel::Z_Crossbow:
                return {Decision::Use};
            case CardLabel::P_Peach:
                if (super->health < super->maxHealth) {
                    return {Decision::Use};
                } else {
                    return {Decision::Skip};
                }
            default:
                return {};
            }
        }

        auto resolveKill(Card card, Game &game) const -> Decision;
        auto resolveDuel(Card card, Game &game) const -> Decision;

        // 是否可以向这个角色（impression）表敌意
        auto canProvoke(PlayerRole role) const -> bool {
            switch (super->role) {
            case PlayerRole::M_Main:
                return role == PlayerRole::F_Thief or role == PlayerRole::Questionable;
            case PlayerRole::Z_Minister:
                return role == PlayerRole::F_Thief;
            case PlayerRole::F_Thief:
                return role == PlayerRole::M_Main or role == PlayerRole::Z_Minister;
            default:
                PANIC("Invalid role");
            }
        }

        // 是否可以向这个角色献殷勤
        auto canFlatter(PlayerRole role) const -> bool {
            switch (super->role) {
            case PlayerRole::M_Main:
                return role == PlayerRole::Z_Minister or role == PlayerRole::M_Main;
            case PlayerRole::Z_Minister:
                return role == PlayerRole::Z_Minister or role == PlayerRole::M_Main;
            case PlayerRole::F_Thief:
                return role == PlayerRole::F_Thief;
            default:
                PANIC("Invalid role");
            }
        }

        // 无距离限制地选择一个攻击目标
        auto selectTarget(Game &game) const -> Player *;

        // 尝试使用一张牌。
        // 使用与否，取决于操作者的意愿。即可以拒绝出牌。（例如忠猪不打主猪）
        // 实际的出牌目标也由操作者决定。
        // 返回值：是否成功使用牌。
        auto tryCard(Card card, Game &game) const -> Decision {
            if (auto res = direct(card, game); res.resolved()) return res;
            if (auto res = resolveKill(card, game); res.resolved()) return res;
            if (auto res = resolveDuel(card, game); res.resolved()) return res;
            PANIC("Cannot resolve card");
        }

        // 决定是否要回应来自对方的决斗（duel）
        auto responseDuel(Player &source) const -> bool;
    } designant{this};
    friend struct Designant;

    auto damaged(i32 amount, DamageType type, Player &source, Game &game) -> void;
    auto camp() const -> PlayerRole;
    auto play(Game &game) -> void;
};
}

#endif
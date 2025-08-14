#pragma once
#ifndef ENUMS_HEADER
#define ENUMS_HEADER

#include "util.hpp"
#include "panic.hpp"

namespace PCK {
// 定义枚举
enum class PlayerRole: char {
    Undefined = '0',    // 未定义
    Questionable = '?', // 类反猪
    F_Thief = 'F',      // 反猪
    Z_Minister = 'Z',   // 忠猪
    M_Main = 'm',       // 主猪（不可被覆盖，使用 ASCII 最大的）
};
auto constexpr leastShowedRole = PlayerRole::F_Thief;
auto constexpr parsePlayerRole(char ch) -> PlayerRole {
    switch (ch) {
    case 'F': return PlayerRole::F_Thief;
    case 'M': return PlayerRole::M_Main;
    case 'Z': return PlayerRole::Z_Minister;
    default: return PlayerRole::Undefined;
    }
}
auto constexpr operator- (PlayerRole const &pr) -> PlayerRole {
    switch (pr) {
        case PlayerRole::F_Thief: return PlayerRole::Z_Minister;
        case PlayerRole::Z_Minister: return PlayerRole::F_Thief;
        default: return PlayerRole::Undefined;
    }
}

enum class CardLabel: char {
    P_Peach = 'P',
    K_Killing = 'K',
    D_Dodge = 'D',
    Z_Crossbow = 'Z',
    F_Dueling = 'F',
    N_Invasion = 'N',
    W_Arrows = 'W',
    J_Unbreakable = 'J',
    T_Test = 'T'
};
auto constexpr parseCardLabel(char ch) -> CardLabel { 
    switch (ch) {
    case 'P': return CardLabel::P_Peach;
    case 'K': return CardLabel::K_Killing;
    case 'D': return CardLabel::D_Dodge;
    case 'Z': return CardLabel::Z_Crossbow;
    case 'F': return CardLabel::F_Dueling;
    case 'N': return CardLabel::N_Invasion;
    case 'W': return CardLabel::W_Arrows;
    case 'J': return CardLabel::J_Unbreakable;
    default: PANIC("Unknown card label");
    }
}

// 伤害类别
enum class DamageType: i8 {
    Undefined, 
    DuelingFailed,  // 决斗失败
    Invading,       // 南猪入侵
    Dueling,        // 决斗开始
    Killing,        // 杀
};
}

#endif

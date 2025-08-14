#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <ranges>
#include <utility>
#include <vector>

#include "util.hpp"
#include "panic.hpp"
#include "concat_view.hpp"

namespace ranges = std::ranges;
namespace views = std::views;


namespace Solution {
    namespace Example {
        bool _;
        namespace NamespaceInClass {
            // 我发明了一种奇怪的设计模式，称之为“类中命名空间”。
            // C++ 并不支持在 class 中定义一个 namespace，但是我们可以模拟。
            // 这可以避免出现“巨型类”，众多成员挤在一个作用域中无法管理。
            struct Super {
                struct Namespace {
                    Super *super{};  // 可以通过该指针访问其他数据。
                };
                // 可能需要额外声明友元。
                friend struct Namespace;
            };

            // 这样看上去会引入循环引用的问题，但实际上影响不大。
            // 在逻辑上，我们只是把 Super 的所有成员分成了若干个命名空间。
            // 不同于常规的组合、继承，这些类不应该有任何其他的用途。
            // 即使是希望用 shared_ptr 管理内存，Namespace 中也应该使用裸指针，不会导致内存无法释放。
        }
    }

    // 定义枚举
    enum class PlayerRole: char {
        Undefined = '0',    // 未定义
        Questionable = '?', // 类反猪
        F_Thief = 'F',      // 反猪
        Z_Minister = 'Z',   // 忠猪
        M_Main = 'm',       // 主猪（不可被覆盖，使用 ASCII 最大的）
    };
    auto constexpr leastShowedRole = PlayerRole::F_Thief;
    auto parsePlayerRole(char ch) -> PlayerRole {
        switch (ch) {
        case 'F': return PlayerRole::F_Thief;
        case 'M': return PlayerRole::M_Main;
        case 'Z': return PlayerRole::Z_Minister;
        default: return PlayerRole::Undefined;
        }
    }
    auto operator- (PlayerRole const &pr) -> PlayerRole {
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
    auto parseCardLabel(char ch) -> CardLabel { 
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

    // 通过异常处理游戏结束
    struct GameOver: std::exception {
        PlayerRole winner;
        GameOver(PlayerRole winner): winner(winner) {}
        auto what() const noexcept -> char const * override {
            return "Game Over";
        }
    };

    // 玩家
    class Player;
    // 卡牌（出于性能考虑，**不采用**多态实现）
    class Card;
    // 主要游戏逻辑
    class Game;

    // 卡牌
    class Card {
        CardLabel label;
    public:
        Card(CardLabel label): label(label) {}

        auto getLabel() const -> CardLabel { return label; };
        auto execute(Player &user, Player *target = nullptr, Game *game = nullptr) -> void;
    };

    // 玩家
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

    auto Game::drawCard() -> Card {
        auto card = deck.front();
        if (deck.size() > 1) deck.pop_front();

        return card;
    }

    // 获取从当前玩家的下一个玩家开始，按照逆时针方向的存活玩家列表。
    // 该列表中可以指定是否存在当前玩家。（默认不存在）
    // 例如，1 2 3 4 5(死亡) 6，传入 player = 2。
    // 返回：3 4 5 6 1。
    auto Game::getPlayersFrom(Player &player, bool hasThis) -> auto {
        auto id = player.id;
        return concat_view(
            ranges::subrange{players.begin() + id + i32(not hasThis), players.end()},
            ranges::subrange{players.begin(), players.begin() + id}
        ) | views::filter(lam(const &p, p.alive));
    }

    auto Game::round() -> void {
        for (auto &pl: players) {
            if (not pl.alive) continue;
            pl.play(*this);
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

    // 抽 n 张卡。
    // 可能修改：cards。
    auto Player::CardManager::draw(Game &game, i32 n) -> void {
        for (i32 i = 0; i < n; ++i) {
            cards.push_back(game.drawCard());
        }
    }
    // 寻找一张指定标签的卡。
    auto Player::CardManager::findCard(CardLabel label) -> CardList::iterator {
        return ranges::find(cards, label, lam(x, x.getLabel()));
    }
    // 寻找指定标签的卡牌，然后：
    // - 如果存在，使用并弃置，返回 true
    // - 如果不存在，返回 false
    // 可能修改 cards。
    template <typename ...Ts>
    auto Player::CardManager::useCard(CardLabel label, Ts &&...args) -> bool {
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
    auto Player::damaged(i32 amount, DamageType type, Player &source, Game &game) -> void {
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
    auto Player::camp() const -> PlayerRole {
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
    auto Player::play(Game &game) -> void {
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

    auto Player::Designant::resolveKill(Card card, Game &game) const -> Decision {
        if (card.getLabel() != CardLabel::K_Killing) return {};
        // 后面的第一个玩家
        auto &target = *game.getPlayersFrom(*super).begin();
        if (canProvoke(target.impression)) {
            return {Decision::Use, &target};
        }
        return {Decision::Skip};
    }

    auto Player::Designant::resolveDuel(Card card, Game &game) const -> Decision {
        if (card.getLabel() != CardLabel::F_Dueling) return {};
        auto *target = selectTarget(game);

        if (target == nullptr) {
            return {Decision::Skip};  // 无法决斗
        }
        return {Decision::Use, target};
    }

    auto Player::Designant::selectTarget(Game &game) const -> Player * {
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

    auto Player::Designant::responseDuel(Player &source) const -> bool {
        // 仅有“忠猪不打主猪”一条例外，否则都会尽力决斗
        return super->role != PlayerRole::Z_Minister or source.role != PlayerRole::M_Main;
    }

    
    // 所有卡牌的实现
    namespace CardImpl {
        auto test() -> void {
            std::cout << "TestCard execute" << endl;
        }
        // 可能修改 user 和 target 的 cards。
        auto killing(Player &user, Player &target, Game &game) -> void {
            // 对方先尝试使用闪
            if (not target.cardManager.useCard(CardLabel::D_Dodge)) {
                // 闪不开，只能掉血
                target.damaged(1, DamageType::Killing, user, game);
            }
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
        auto invasionLike(Player &user, Game &game, CardLabel type) -> void {
            auto targets = game.getPlayersFrom(user);
            for (auto &target: targets) {
                // 可以被无懈可击阻止
                if (game.blockTrick(user, target, false)) continue;
                // 弃置一张指定牌，或者生命值 -1
                auto &cards = target.cardManager.cards;
                auto it = target.cardManager.findCard(type);
                if (it != cards.end()) {
                    cards.erase(it);
                } else {
                    target.damaged(1, DamageType::Invading, user, game);
                }
            }
        }
        auto invasion(Player &user, Game &game) -> void {
            invasionLike(user, game, CardLabel::K_Killing);
        }
        auto arrows(Player &user, Game &game) -> void {
            invasionLike(user, game, CardLabel::D_Dodge);
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
                if (cur.designant.responseDuel(oppo)) {
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

    auto solve() -> void {
        i32 playerCount{}, cardCount{};
        std::cin >> playerCount >> cardCount;

        std::vector<Player> players;
        players.reserve(playerCount);
        for (i32 _ = playerCount; _ --> 0; ) {
            char typeChar{}, p;
            std::cin >> typeChar >> p;

            players.emplace_back(
                static_cast<i32>(players.size()), parsePlayerRole(typeChar));
            auto &cur = players.back();

            for (i32 _ = 4; _ --> 0; ) {
                char card{}; std::cin >> card;
                cur.cardManager.cards.emplace_back(parseCardLabel(card));
            }
        }

        std::deque<Card> deck;
        for (auto _ = cardCount; _ --> 0; ) {
            char card{}; std::cin >> card;
            deck.emplace_back(parseCardLabel(card));
        }

        Game game{std::move(players), std::move(deck)};

        try {
            while (true) {
                game.round();
            }
        } catch (GameOver &e) {
            std::cout << (e.winner == PlayerRole::M_Main? "MP": "FP") << '\n';
            game.print();
        }
    }
}

auto main() -> int {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr), std::cout.tie(nullptr);

    Solution::solve();
    return 0;
}

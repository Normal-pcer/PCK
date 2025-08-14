#include <cassert>
#include <deque>
#include <iostream>
#include <utility>
#include <vector>

#include "util.hpp"
#include "card.hpp"
#include "game.hpp"
#include "player.hpp"

namespace PCK {
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

    PCK::solve();
    return 0;
}

#pragma once
#ifndef CONCAT_VIEW_HEADER
#define CONCAT_VIEW_HEADER

#include <cstdint>
#include <ranges>

// 简易实现 C++26 concat_view
namespace my_views {

    template <std::ranges::input_range Range1, std::ranges::input_range Range2>
    class concat_view : public std::ranges::view_interface<concat_view<Range1, Range2>> {
    private:
        Range1 range1_;
        Range2 range2_;

        using iterator1 = std::ranges::iterator_t<Range1>;
        using sentinel1 = std::ranges::sentinel_t<Range1>;
        using iterator2 = std::ranges::iterator_t<Range2>;
        using sentinel2 = std::ranges::sentinel_t<Range2>;

        class iterator;

        struct sentinel {
            sentinel2 end2;
        };

    public:
        concat_view() = default;
        concat_view(Range1 r1, Range2 r2) 
            : range1_(std::move(r1)), range2_(std::move(r2)) {}

        auto begin() {
            return iterator(
                std::ranges::begin(range1_),
                std::ranges::end(range1_),
                std::ranges::begin(range2_),
                std::ranges::end(range2_)
            );
        }

        auto end() {
            return sentinel{ std::ranges::end(range2_) };
        }
    };

    template <std::ranges::input_range Range1, std::ranges::input_range Range2>
    class concat_view<Range1, Range2>::iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::common_type_t<
            typename std::iterator_traits<iterator1>::value_type,
            typename std::iterator_traits<iterator2>::value_type
        >;
        using difference_type = std::common_type_t<
            typename std::iterator_traits<iterator1>::difference_type,
            typename std::iterator_traits<iterator2>::difference_type
        >;
        using reference = std::common_reference_t<
            typename std::iterator_traits<iterator1>::reference,
            typename std::iterator_traits<iterator2>::reference
        >;

    private:
        enum class state: std::uint8_t { first, second, end };
        state current_state_ = state::end;

        iterator1 it1_;
        sentinel1 end1_;
        iterator2 it2_;
        sentinel2 end2_;

    public:
        iterator() = default;

        iterator(iterator1 it1, sentinel1 end1, iterator2 it2, sentinel2 end2)
            : it1_(std::move(it1)), end1_(std::move(end1))
            , it2_(std::move(it2)), end2_(std::move(end2))
        {
            if (it1_ != end1_) {
                current_state_ = state::first;
            } else if (it2_ != end2_) {
                current_state_ = state::second;
            } else {
                current_state_ = state::end;
            }
        }

        reference operator*() const {
            if (current_state_ == state::first) {
                return *it1_;
            }
            if (current_state_ == state::second) {
                return *it2_;
            }
            throw std::logic_error("Dereferencing end iterator");
        }

        iterator& operator++() {
            if (current_state_ == state::first) {
                ++it1_;
                if (it1_ == end1_) {
                    if (it2_ != end2_) {
                        current_state_ = state::second;
                    } else {
                        current_state_ = state::end;
                    }
                }
            } else if (current_state_ == state::second) {
                ++it2_;
                if (it2_ == end2_) {
                    current_state_ = state::end;
                }
            } else {
                throw std::logic_error("Incrementing end iterator");
            }
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const iterator& other) const = default;

        bool operator==(const sentinel& s) const {
            return current_state_ == state::end || 
                   (current_state_ == state::second && it2_ == s.end2);
        }
    };

    template <typename Range1, typename Range2>
    concat_view(Range1&& r1, Range2&& r2) -> concat_view<
        std::views::all_t<Range1>, 
        std::views::all_t<Range2>
    >;

}

inline auto concat_view = [](auto&& range1, auto&& range2) {
#if __cplusplus < 202600L
        return my_views::concat_view(
            std::forward<decltype(range1)>(range1),
            std::forward<decltype(range2)>(range2)
        );
#else
        return std::views::concat(
            std::forward<decltype(range1)>(range1),
            std::forward<decltype(range2)>(range2)
        );
#endif
};

#endif

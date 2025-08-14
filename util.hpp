#pragma once
#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <algorithm>
#include <cstdint>

#ifndef DEBUG_MODE
#define DEBUG_MODE false
#endif
#define debug if constexpr (DEBUG_MODE)

template <typename T> auto chkMax(T &base, const T &cmp) -> T & { return (base = std::max(base, cmp)); }
template <typename T> auto chkMin(T &base, const T &cmp) -> T & { return (base = std::min(base, cmp)); }

#define _lambda_1(expr) [&]() { return expr; }
#define _lambda_2(a, expr) [&](auto a) { return expr; }
#define _lambda_3(a, b, expr) [&](auto a, auto b) { return expr; }
#define _lambda_4(a, b, c, expr) [&](auto a, auto b, auto c) { return expr; }
#define _lambda_overload(a, b, c, d, e, ...) _lambda_##e
#define lambda(...) _lambda_overload(__VA_ARGS__, 4, 3, 2, 1)(__VA_ARGS__)
#define lam lambda

char constexpr endl = '\n';
using i16 = std::int16_t; using i32 = std::int32_t; using i64 = std::int64_t;
using u16 = std::uint16_t; using u32 = std::uint32_t; using u64 = std::uint64_t; using uz = std::size_t;

using i8 = std::int8_t; using u8 = std::uint8_t;

#endif

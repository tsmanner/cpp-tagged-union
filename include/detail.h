#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

// type_traits style struct that contains a type alias for the narrowest
// unsigned integer type that can contain NumValues values.  No specialization
// is available if more than 64 bits are required.
template <std::size_t NumValues, typename Enabled=void>
struct narrowest_unsigned;

// Select uint8_t for 1 to uint8_t max
template <std::size_t NumValues>
struct narrowest_unsigned<NumValues,
  typename std::enable_if<(0x0000000000000000 < NumValues and NumValues <= 0x00000000000000FF)>::type
> {
  using type = uint8_t;
};

// Select uint16_t for uint8_t max + 1 to uint16_t max
template <std::size_t NumValues>
struct narrowest_unsigned<NumValues,
  typename std::enable_if<(0x00000000000000FF < NumValues and NumValues <= 0x000000000000FFFF)>::type
> {
  using type = uint16_t;
};

// Select uint32_t for uint16_t max + 1 to uint32_t max
template <std::size_t NumValues>
struct narrowest_unsigned<NumValues,
  typename std::enable_if<(0x000000000000FFFF < NumValues and NumValues <= 0x00000000FFFFFFFF)>::type
> {
  using type = uint32_t;
};

// Select uint64_t for uint32_t max + 1 to uint64_t max
template <std::size_t NumValues>
struct narrowest_unsigned<NumValues,
  typename std::enable_if<(0x00000000FFFFFFFF < NumValues and NumValues <= 0xFFFFFFFFFFFFFFFF)>::type
> {
  using type = uint64_t;
};



// Recursive variadic function to check for repeat types.

// If only one type is provided, return false
template <typename T>
constexpr bool isIn() {
  return false;
}

// Tail: there's only one type, just check if it's the same.
template <typename T, typename CurrentT>
constexpr bool isIn() {
  return std::is_same<T, CurrentT>::value;
}

// Recursive call: boolean OR the tail of T and the CurrentT with T and
// the list of RemainingTs.
template <typename T, typename CurrentT, typename ...RemainingTs>
constexpr typename std::enable_if<sizeof...(RemainingTs), bool>::type
isIn() {
  return isIn<T, CurrentT>() || isIn<T, RemainingTs...>();
}

#pragma once

#include "detail.h"
#include <tuple>

//
// Recursive union type
//

// Tail of recursion has no contents.
template <typename ...Ts>
union Union {};

// Recursive type can be either this type, or a Union of the remaining types.
template <typename T, typename ...Ts>
union Union<T, Ts...> {
  static_assert(!isIn<T, Ts...>(), "Union types must be unique!");

  T value;
  Union<Ts...> next;
};


//
// Const reference access to elements of a Union
//

template <std::size_t INDEX, typename ...Ts>
constexpr inline
typename std::enable_if<(INDEX == 0), typename std::tuple_element<INDEX, std::tuple<Ts...>>::type>::type const &
get(Union<Ts...> const &u) {
    return u.value;
}

template <std::size_t INDEX, typename ...Ts>
constexpr inline
typename std::enable_if<(INDEX > 0), typename std::tuple_element<INDEX, std::tuple<Ts...>>::type>::type const &
get(Union<Ts...> const &u) {
    return get<INDEX-1>(u.next);
}


//
// Non-const reference access to elements of a Union
//

template <std::size_t INDEX, typename ...Ts>
constexpr inline
typename std::enable_if<(INDEX == 0), typename std::tuple_element<INDEX, std::tuple<Ts...>>::type>::type &
get(Union<Ts...> &u) {
    return u.value;
}

template <std::size_t INDEX, typename ...Ts>
constexpr inline
typename std::enable_if<(INDEX > 0), typename std::tuple_element<INDEX, std::tuple<Ts...>>::type>::type &
get(Union<Ts...> &u) {
    return get<INDEX-1>(u.next);
}

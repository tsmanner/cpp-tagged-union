#pragma once

#include <cstdint>
#include <limits>
#include <tuple>
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


//
// TaggedUnion type that tags each type with it's index in the parameter pack, starting from 0.
//

template <typename ...Ts>
struct TaggedUnion {
  // Type alias for the type used to represent the index.
  using index_type = typename narrowest_unsigned<sizeof...(Ts) + 1>::type;
  // Constant sentinel value to use for uninitialized instances.
  // activeIndex() will return this value if no content has been set yet.
  // indexOf<T>() cannot ever match this value, because indexOf<T> will fail
  // to compile if the type isn't found in Ts.
  static constexpr index_type NONE = sizeof...(Ts);


  // Const accessor for the current index.
  index_type const &activeIndex() const { return index; }


  // Recursively determine the index of a particular type.
  // If the current type matches, return this INDEX value.
  template <typename T, std::size_t INDEX, typename CurrentT, typename ...RemainingTs>
  static constexpr inline
  typename std::enable_if<(std::is_same<T, CurrentT>::value), std::size_t>::type
  indexOf() {
      return INDEX;
  }

  // Recursively determine the index of a particular type.
  // If the current type doesn't matche, recursively call, dropping the CurrentT
  // and incrementing INDEX
  template <typename T, std::size_t INDEX, typename CurrentT, typename ...RemainingTs>
  static constexpr inline
  typename std::enable_if<(!std::is_same<T, CurrentT>::value), std::size_t>::type
  indexOf() {
      return indexOf<T, INDEX + 1, RemainingTs...>();
  }

  // Entry point function for indexOf that accepts a single type to search for.
  template <typename T>
  static constexpr inline std::size_t indexOf() {
      return indexOf<T, 0, Ts...>();
  }


  // Create a new instance of a TaggedUnion, inhabited by type T and initialized
  // with the variadic list of arguments.
  template <typename T, typename ...Args>
  static TaggedUnion<Ts...> create(Args const &...args) {
    TaggedUnion<Ts...> tu;
    get<indexOf<T>()>(tu.value) = T{args...};
    tu.index = indexOf<T>();
    return tu;
  }


  // Get a const reference to the T element of the Union.
  template <typename T>
  inline T const &as() const {
    if (indexOf<T>() == activeIndex()) {
      return get<indexOf<T>()>(value);
    }
    throw "Attempt to access inactive TaggedUnion field";
  }

  // Get a non-const reference to the T element of the Union.
  template <typename T>
  inline T &as() {
    if (indexOf<T>() == activeIndex()) {
      return get<indexOf<T>()>(value);
    }
    throw "Attempt to access inactive TaggedUnion field";
  }

private:
  Union<Ts...> value {};
  index_type index = NONE;

};


#define DEFINE_TAGGED_UNION(NAME, ...)\
  struct NAME : public TaggedUnion<__VA_ARGS__> {\
    enum class Kind { __VA_ARGS__ };\
    Kind kind() const { return static_cast<Kind>(activeIndex()); }\
    template <typename T, typename ...Args>\
    static NAME create(Args const &...args) {\
      return { TaggedUnion<__VA_ARGS__>::create<T>(args...) };\
    }\
  };


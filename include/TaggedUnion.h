#pragma once

#include "Union.h"

//
// TaggedUnion type that associates each type with it's index in the parameter
// pack, starting from 0.
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


#define DEFINE_AUTO_TAGGED_UNION(NAME, ...)\
  struct NAME : public TaggedUnion<__VA_ARGS__> {\
    enum class Kind { __VA_ARGS__ };\
    Kind kind() const { return static_cast<Kind>(activeIndex()); }\
    template <typename T, typename ...Args>\
    static NAME create(Args const &...args) {\
      return { TaggedUnion<__VA_ARGS__>::create<T>(args...) };\
    }\
  };


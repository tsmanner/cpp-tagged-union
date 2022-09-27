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
  inline index_type const &activeIndex() const { return index; }

  // Default constructor
  constexpr TaggedUnion() = default;

  // Per variant constructors
  template <typename T>
  constexpr TaggedUnion(T const &alternative) {
    constexpr auto index_of_T = indexOf<T>();
    index = indexOf<T>();
    get<index_of_T>(value) = alternative;
  }


  // Copy and Move constructors
  constexpr TaggedUnion(TaggedUnion<Ts...> const &tagged_union): value(tagged_union.value), index(tagged_union.index) {}


  // Recursively determine the index of a particular type.
  // If the current type matches, return this INDEX value.
  template <typename T, std::size_t INDEX, typename CurrentT, typename ...RemainingTs>
  static constexpr inline
  typename std::enable_if<(std::is_same<T, CurrentT>::value), std::size_t>::type
  indexOf() {
      return INDEX;
  }

  // Recursively determine the index of a particular type.
  // If the current type doesn't match, recursively call, dropping the CurrentT
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
  static inline TaggedUnion<Ts...> create(Args const &...args) {
    constexpr auto index_of_T = indexOf<T>();
    TaggedUnion<Ts...> tu;
    get<index_of_T>(tu.value) = T{args...};
    tu.index = index_of_T;
    return tu;
  }


  // Get a const reference to the T element of the Union.
  template <typename T>
  inline T const &as() const {
    constexpr auto index_of_T = indexOf<T>();
    if (index == index_of_T) {
      return get<index_of_T>(value);
    }
    throw "Attempt to access inactive TaggedUnion field";
  }

  // Get a non-const reference to the T element of the Union.
  template <typename T>
  inline T &as() {
    constexpr auto index_of_T = indexOf<T>();
    if (index == index_of_T) {
      return get<index_of_T>(value);
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
    inline Kind kind() const { return static_cast<Kind>(activeIndex()); }\
    /* Inherit the TaggedUnion constructors */\
    using TaggedUnion<__VA_ARGS__>::TaggedUnion;\
    /* Add an explicit copy-from-TaggedUnion constructor */\
    constexpr NAME(TaggedUnion<__VA_ARGS__> const &tu): TaggedUnion(tu) {}\
    /* Expose the create factories, yielding an instance of NAME instead of the underlying TaggedUnion */\
    template <typename T, typename ...Args>\
    static inline NAME create(Args const &...args) {\
      return TaggedUnion::create<T>(args...);\
    }\
  };


#define _tagged_union_join2(a, b) a ## b
#define _tagged_union_join(a, b) _tagged_union_join2(a, b)

#define TAGGED_UNION_CASE(_CaseType, _CaseVariant, _obj, _typed) \
    _CaseType::Kind::_CaseVariant: auto &_typed = _obj.as<_CaseVariant>(); _tagged_union_join(_dummy_label, __LINE__)

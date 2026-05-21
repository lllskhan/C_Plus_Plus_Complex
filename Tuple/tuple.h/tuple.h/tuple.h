#include <iostream>
#include <type_traits>

template <typename... Types>
class Tuple;

template <typename Head, typename... Tail>
class Tuple<Head, Tail...>;

template <>
class Tuple<> {};

template <typename... Types>
struct TupleSize;

template <typename... Types>
struct TupleSize<Tuple<Types...>> {
  static constexpr size_t size = sizeof...(Types);
};

template <typename T, typename... Types>
struct type_counter;

template <typename T>
struct type_counter<T> {
  static constexpr bool value = false;
};

template <typename T, typename Head, typename... Tail>
struct type_counter<T, Head, Tail...> {
  static constexpr bool value =
  std::is_same_v<T, Head> + type_counter<T, Tail...>::value;
};

template <size_t N, typename... Types>
decltype(auto) get(Tuple<Types...>& tuple)
noexcept
{
  if constexpr (N == 0) {
    return (tuple.head);
  } else {
    return get<N-1>(tuple.tail);
  }
}

template <size_t N, typename... Types>
decltype(auto) get(const Tuple<Types...>& tuple)
noexcept
{
  if constexpr (N == 0) {
    return (tuple.head);
  } else {
    return get<N-1>(tuple.tail);
  }
}

template <size_t N, typename... Types>
decltype(auto) get(Tuple<Types...>&& tuple)
noexcept
{
  if constexpr (N == 0) {
    return std::forward<decltype(tuple.head)&&>(tuple.head);
  } else {
    return get<N-1>(std::forward<decltype(tuple.tail)>(tuple.tail));
  }
}

template <typename T, typename... Types>
constexpr
T& get(Tuple<Types...>& tuple)
noexcept
requires(type_counter<T, Types...>::value == 1)
{
  if constexpr (std::is_same_v<T, decltype(tuple.head)>) {
    return std::forward<T&>(tuple.head);
  } else {
    return get<T>(tuple.tail);
  }
}

template <typename T, typename... Types>
constexpr
const T& get(const Tuple<Types...>& tuple)
noexcept
requires(type_counter<T, Types...>::value == 1)
{
  if constexpr (std::is_same_v<T, decltype(tuple.head)>) {
    return std::forward<const T&>(tuple.head);
  } else {
    return get<T>(tuple.tail);
  }
}

template <typename T, typename... Types>
constexpr
T&& get(Tuple<Types...>&& tuple)
noexcept
requires(type_counter<T, Types...>::value == 1)
{
  if constexpr (std::is_same_v<T, decltype(tuple.head)>) {
    return std::forward<T&&>(tuple.head);
  } else {
    return get<T>(tuple.tail);
  }
}

template <typename T, typename... Types>
constexpr
const T&& get(const Tuple<Types...>&& tuple)
noexcept
requires(type_counter<T, Types...>::value == 1)
{
  if constexpr (std::is_same_v<T, decltype(tuple.head)>) {
    return std::forward<const T&&>(tuple.head);
  } else {
    return get<T>(tuple.tail);
  }
}



template <typename T>
concept copy_list_initializable =
       requires { T{}; };

template <typename... Args>
concept all_copy_list_initializable =
        (copy_list_initializable<Args> and ...);



template <typename OtherTuple, typename... Types>
struct all_constructible;

template <typename OtherTuple>
struct all_constructible<OtherTuple> {
  static constexpr bool value = true;
};

template <typename OtherTuple, typename Head, typename... Tail>
struct all_constructible<OtherTuple, Head, Tail...> {
  static constexpr bool value =
  std::is_constructible_v<Head, decltype(get<TupleSize<std::decay_t<OtherTuple>>::size - 1 - sizeof...(Tail)>(std::forward<OtherTuple>(std::declval<OtherTuple>())))> and
  all_constructible<OtherTuple, Tail...>::value;
};




template <typename OtherTuple, typename... Types>
struct all_convertible;

template <typename OtherTuple>
struct all_convertible<OtherTuple> {
  static constexpr bool value = true;
};

template <typename OtherTuple, typename Head, typename... Tail>
struct all_convertible<OtherTuple, Head, Tail...> {
  static constexpr bool value =
  std::is_convertible_v< decltype(get<TupleSize<std::decay_t<OtherTuple>>::size - 1 - sizeof...(Tail)>(std::forward<OtherTuple>(std::declval<OtherTuple>()))), Head> and
  all_convertible<OtherTuple, Tail...>::value;
};


template <typename Pair, typename Head, typename... Tail>
concept ConvertibleFromPair = requires {
    requires std::is_convertible_v<decltype(std::get<0>(std::declval<Pair>())), Head>;
    requires (std::is_convertible_v<decltype(std::get<1>(std::declval<Pair>())), Tail> and ...);
};



template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
  Head head;
  Tuple<Tail...> tail;
  
  template <typename ...>
  friend class Tuple;
  
  template <size_t N, typename... Types>
  friend decltype(auto) get(Tuple<Types...>& tuple) noexcept;
  
  template <size_t N, typename... Types>
  friend decltype(auto) get(const Tuple<Types...>& tuple) noexcept;
  
  template <size_t N, typename... Types>
  friend decltype(auto) get(Tuple<Types...>&& tuple) noexcept;
  
  template <typename T, typename... Types>
  friend constexpr
  T& get(Tuple<Types...>& tuple)
  noexcept
  requires(type_counter<T, Types...>::value == 1);

  template <typename T, typename... Types>
  friend constexpr
  const T& get(const Tuple<Types...>& tuple)
  noexcept
  requires(type_counter<T, Types...>::value == 1);

  template <typename T, typename... Types>
  friend constexpr
  T&& get(Tuple<Types...>&& tuple)
  noexcept
  requires(type_counter<T, Types...>::value == 1);
  
  template <typename T, typename... Types>
  friend constexpr
  const T&& get(const Tuple<Types...>&& tuple)
  noexcept
  requires(type_counter<T, Types...>::value == 1);
  
  template <typename FirstTuple, typename... OtherTuples>
  friend constexpr
  decltype(auto) tupleCat(FirstTuple&& first, OtherTuples&&... others);
  
  template <typename... TTypes, typename... UTypes>
  friend constexpr bool operator<(const Tuple<TTypes...>& lhs, const Tuple<UTypes...>& rhs);
  
  
  template <typename Leading, typename... OtherTuples>
  explicit
  Tuple(Leading&& leading, OtherTuples&&... others)
  requires(TupleSize<std::decay_t<Leading>>::size > 1)
  : head(get<0>(std::forward<decltype(leading)>(leading)))
  , tail(std::forward<Leading>(leading).tail, std::forward<decltype(others)>(others)...)
  {}
  
  template <typename Leading, typename After, typename... OtherTuples>
  explicit
  Tuple(Leading&& leading, After&& after, OtherTuples&&... others)
  requires(TupleSize<std::decay_t<Leading>>::size == 1)
  : head(get<0>(std::forward<decltype(leading)>(leading)))
  , tail(std::forward<decltype(after)>(after), std::forward<decltype(others)>(others)...)
  {}
  
  template <typename Leading>
  explicit
  Tuple(Leading&& leading)
  requires(TupleSize<std::decay_t<Leading>>::size == 1)
  : head(get<0>(std::forward<decltype(leading)>(leading)))
  {}
  
public:
  
  constexpr
  explicit(!copy_list_initializable<Head> or
           !all_copy_list_initializable<Tail...>
           )
  Tuple()
  requires (std::is_default_constructible_v<Head> and
            (std::is_default_constructible_v<Tail> and ...)
            )
  : head{}, tail{}
  {}
  
  constexpr
  explicit(!std::is_convertible_v<const Head&, Head> or
           !(std::is_convertible_v<const Tail&, Tail> and ...)
           )
  Tuple(Head&& head, Tail&&... tail)
  requires (std::is_copy_constructible_v<Head> and
            (std::is_copy_constructible_v<Tail> and ...)
            )
  : head(std::forward<Head>(head))
  , tail(std::forward<Tail>(tail)...)
  {}
  
  template <typename First, typename... Remainder>
  constexpr
  explicit(!std::is_convertible_v<First, Head> or
           !(std::is_convertible_v<Remainder, Tail> and ...)
           )
  Tuple(First&& first, Remainder&&... remainder)
  requires (sizeof...(Tail) == sizeof...(Remainder) and
            std::is_constructible_v<Head, First> and
            (std::is_constructible_v<Tail, Remainder> and ...) and
            (!std::is_same_v<std::remove_cvref_t<First>, Tuple> || sizeof...(Tail) != 0)
            )
  : head(std::forward<First>(first))
  , tail(std::forward<Remainder>(remainder)...)
  {}
 
private:
  template <typename OtherTuple>
  explicit
  Tuple(OtherTuple&& other)
  requires(sizeof...(Tail) > 0)
  : head(get<TupleSize<std::decay_t<OtherTuple>>::size - 1 - sizeof...(Tail)>(std::forward<decltype(other)>(other)))
  , tail(std::forward<decltype(other)>(other))
  {}
  
  template <typename OtherTuple>
  explicit
  Tuple(OtherTuple&& other)
  requires(sizeof...(Tail) == 0 and TupleSize<std::decay_t<OtherTuple>>::size >= 0)
  : head(get<TupleSize<std::decay_t<OtherTuple>>::size - 1>(std::forward<decltype(other)>(other)))
  {}
  
public:
  
  template <typename First, typename... Remainder>
  constexpr
  explicit(!all_convertible<const Tuple<First, Remainder...>&, Head, Tail...>::value)
  Tuple(const Tuple<First, Remainder...>& other)
  requires (sizeof...(Tail) == sizeof...(Remainder) and
            all_constructible<decltype(other), Head, Tail...>::value and ((sizeof...(Tail) > 0) or
            (!std::is_convertible_v<decltype(other), Head> and
             !std::is_constructible_v<Head, decltype(other)> and !std::is_same_v<Head, First>))
            )
  : head(get<0>(std::forward<decltype(other)>(other)))
  , tail(std::forward<decltype(other)>(other))
  {}
  
  template <typename First, typename... Remainder>
  constexpr
  explicit(!all_convertible<Tuple<First, Remainder...>&&, Head, Tail...>::value)
  Tuple(Tuple<First, Remainder...>&& other)
  requires (sizeof...(Tail) == sizeof...(Remainder) and
            all_constructible<decltype(other), Head, Tail...>::value and ((sizeof...(Tail) > 0) or
            (!std::is_convertible_v<decltype(other), Head> and
             !std::is_constructible_v<Head, decltype(other)> and !std::is_same_v<Head, First>))
            )
  : head(get<0>(std::forward<decltype(other)>(other)))
  , tail(std::forward<decltype(other)>(other))
  {}
  
  template <typename I1, typename I2>
  constexpr
  explicit(!ConvertibleFromPair<const std::pair<I1, I2>&, Head, Tail...>)
  Tuple(const std::pair<I1, I2>& other)
  requires(sizeof...(Tail) == 1 and
           std::is_constructible_v<Head, decltype(std::get<0>(std::forward<decltype(other)>(other)))> and
           std::is_constructible_v<Tail..., decltype(std::get<1>(std::forward<decltype(other)>(other)))>
           )
  : head(std::get<0>(std::forward<decltype(other)>(other)))
  , tail(std::get<1>(std::forward<decltype(other)>(other)))
  {}
  
  template <typename I1, typename I2>
  constexpr
  explicit(!ConvertibleFromPair<std::pair<I1, I2>&&, Head, Tail...>)
  Tuple(std::pair<I1, I2>&& other)
  requires(sizeof...(Tail) == 1 and
           std::is_constructible_v<Head, decltype(std::get<0>(std::forward<decltype(other)>(other)))> and
           std::is_constructible_v<Tail..., decltype(std::get<1>(std::forward<decltype(other)>(other)))>
           )
  : head(std::get<0>(std::forward<decltype(other)>(other)))
  , tail(std::get<1>(std::forward<decltype(other)>(other)))
  {}
  
  Tuple(const Tuple& other)
  requires(std::is_copy_constructible_v<Head> and
           (std::is_copy_constructible_v<Tail> and ...)
           )
  : head(other.head)
  , tail(other.tail)
  {}
  
  Tuple(Tuple&& other)
  requires(std::is_move_constructible_v<Head> and
           (std::is_move_constructible_v<Tail> and ...)
           )
  : head(std::move(other.head))
  , tail(std::move(other.tail))
  {}
  
  constexpr
  Tuple& operator=(const Tuple& other)
  requires(std::is_copy_assignable_v<Head> and
           (std::is_copy_assignable_v<Tail> and ...)
           )
  {
    if (this != &other) {
      head = other.head;
      tail = other.tail;
    }
    return *this;
  }
  
  constexpr
  Tuple& operator=(Tuple&& other)
  noexcept(std::is_nothrow_move_assignable_v<Head> and
           (std::is_nothrow_move_assignable_v<Tail> and ...)
           )
  requires(std::is_move_assignable_v<Head> and
           (std::is_move_assignable_v<Tail> and ...)
           )
  {
    if (this != &other) {
      head = std::forward<Head>(get<0>(other));
      tail = std::move(other.tail);
    }
    return *this;
  }
  
  template <typename First, typename... Remainder>
  constexpr
  Tuple& operator=(const Tuple<First, Remainder...>& other)
  requires(sizeof...(Tail) == sizeof...(Remainder) and
           std::is_assignable_v<Head&, const First&> and
           (std::is_assignable_v<Tail&, const Remainder&> and ...)
           )
  {
    get<0>(*this) = get<0>(other);
    tail = other.tail;
    return *this;
  }
  
  template <typename First, typename... Remainder>
  constexpr
  Tuple& operator=(Tuple<First, Remainder...>&& other)
  noexcept(std::is_nothrow_move_assignable_v<Head> and (std::is_nothrow_move_assignable_v<Tail> and ...))
  requires(sizeof...(Tail) == sizeof...(Remainder) and
           std::is_assignable_v<Head&, First> and
           (std::is_assignable_v<Tail&, Remainder> and ...)
           )
  {
    get<0>(*this) = std::forward<First>(get<0>(other));
    tail = std::move(other.tail);
    return *this;
  }
  
  template <typename I1, typename I2>
  constexpr
  Tuple operator=(const std::pair<I1, I2>& other)
  requires(sizeof...(Tail) == 1 and
           std::is_assignable_v<Head&, const I1&> and
           std::is_assignable_v<Tail&..., const I2&>
           )
  {
    if (this != &other) {
      head = other.first;
      tail = other.second;
    }
    return *this;
  }
  
  template <typename I1, typename I2>
  constexpr
  Tuple operator=(std::pair<I1, I2>&& other)
  requires(sizeof...(Tail) == 1 and
           std::is_assignable_v<Head&, I1> and
           std::is_assignable_v<Tail&..., I2>)
  {
    if (this != &other) {
      head = std::forward<I1>(other.first);
      tail = std::forward<I2>(other.second);
    }
    return *this;
  }
};

template <typename I1, typename I2>
Tuple(std::pair<I1, I2>) -> Tuple<I1, I2>;


template <typename... Types>
constexpr
decltype(auto) makeTuple(Types&&... types)
{
  Tuple<std::decay_t<Types>...> tuple(std::forward<Types>(types)...);
  return tuple;
}


template <typename... Types>
constexpr
decltype(auto) tie(Types&... types)
noexcept
{
  return Tuple<Types&...>(std::forward<Types&>(types)...);
}


template <typename... Types>
constexpr
decltype(auto) forwardAsTuple(Types&&... types)
noexcept
{
  return Tuple<Types&&...>(std::forward<Types&&>(types)...);
}


template <typename... Args>
struct TypeCompounder;

template <typename... ITypes, typename... UTypes, typename... Remainder>
struct TypeCompounder<Tuple<ITypes...>, Tuple<UTypes...>, Remainder...> {
  using Type = typename TypeCompounder<Tuple<ITypes..., UTypes...>, Remainder...>::Type;
};

template <typename... ITypes, typename... UTypes>
struct TypeCompounder<Tuple<ITypes...>, Tuple<UTypes...>> {
  using Type = Tuple<ITypes..., UTypes...>;
};


template <typename FirstTuple, typename... OtherTuples>
constexpr
decltype(auto) tupleCat(FirstTuple&& first, OtherTuples&&... others)
{
  if constexpr (sizeof...(OtherTuples) == 0) {
    return std::forward<FirstTuple>(first);
  } else {
    using UltimateType = typename TypeCompounder<std::decay_t<FirstTuple>, std::decay_t<OtherTuples>...>::Type;
    return UltimateType(std::forward<decltype(first)>(first), std::forward<decltype(others)>(others)...);
  }
}


template <typename... TTypes, typename... UTypes>
constexpr bool operator==(const Tuple<TTypes...>& , const Tuple<UTypes...>& )
{
  return (std::is_same_v<TTypes, UTypes> and ...);
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator!=(const Tuple<TTypes...>& lhs, const Tuple<UTypes...>& rhs)
{
  return !(lhs == rhs);
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator<(const Tuple<TTypes...>& lhs, const Tuple<UTypes...>& rhs)
{
  if constexpr (sizeof...(TTypes) == 0 and sizeof...(UTypes) == 0) {
    return false;
  } else {
    return (get<0>(lhs) < get<0>(rhs) or (get<0>(lhs) == get<0>(rhs) and lhs.tail < rhs.tail));
  }
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator<=(const Tuple<TTypes...>& lhs, const Tuple<UTypes...>& rhs)
{
  return !(rhs < lhs);
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator>(const Tuple<TTypes...>& lhs, const Tuple<UTypes...>& rhs)
{
  return rhs < lhs;
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator>=(const Tuple<TTypes...>& lhs, const Tuple<UTypes...>& rhs)
{
  return !(lhs < rhs);
}

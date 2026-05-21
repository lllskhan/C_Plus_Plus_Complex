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

#include <iostream>
#include <vector>
#include <utility>
#include <cassert>
#include <functional>
#include <algorithm>
#include <type_traits>

int new_called = 0;
int delete_called = 0;

void* operator new(size_t n) {
    ++new_called;
    return std::malloc(n);
}

void operator delete(void* ptr) noexcept {
    ++delete_called;
    std::free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    ++delete_called;
    std::free(ptr);
}

struct NeitherDefaultNorCopyConstructible {
    double d;

    NeitherDefaultNorCopyConstructible() = delete;
    NeitherDefaultNorCopyConstructible(double d): d(d) {}

    NeitherDefaultNorCopyConstructible(const NeitherDefaultNorCopyConstructible&) = delete;
    NeitherDefaultNorCopyConstructible(NeitherDefaultNorCopyConstructible&&) = default;

    NeitherDefaultNorCopyConstructible& operator=(const NeitherDefaultNorCopyConstructible&) = delete;
    NeitherDefaultNorCopyConstructible& operator=(NeitherDefaultNorCopyConstructible&&) = default;
};

struct NotDefaultConstructible {
    int c;
    NotDefaultConstructible() = delete;
    NotDefaultConstructible(int c): c(c) {}
};

struct NotCopyableOnlyMovable {
    NotCopyableOnlyMovable(const NotCopyableOnlyMovable&) = delete;
    NotCopyableOnlyMovable& operator=(const NotCopyableOnlyMovable&) = delete;
    NotCopyableOnlyMovable(NotCopyableOnlyMovable&&) = default;
    NotCopyableOnlyMovable& operator=(NotCopyableOnlyMovable&&) = default;
};

struct Accountant {
    inline static int constructed = 0;
    inline static int destructed = 0;
    inline static int copy_constructed = 0;
    inline static int move_constructed = 0;
    inline static int copy_assigned = 0;
    inline static int move_assigned = 0;

    Accountant() {
        ++constructed;
    }
    Accountant(const Accountant&) {
        ++constructed;
        ++copy_constructed;
    }
    Accountant(Accountant&&) {
        ++constructed;
        ++move_constructed;
    }
    Accountant& operator=(const Accountant&) {
        ++copy_assigned;
        return *this;
    }
    Accountant& operator=(Accountant&&) {
        ++move_assigned;
        return *this;
    }
    ~Accountant() {
        ++destructed;
    }
};

template <typename T>
class Debug {
    Debug() = delete;
};

template <typename T>
constexpr bool explicit_test() {
    return requires(Tuple<int, T> t) {
        t = {11, 22};
    };
}

static_assert(explicit_test<int>());
static_assert(!explicit_test<std::vector<int>>());

struct ExplicitDefault {
    explicit ExplicitDefault() {}
};

template <typename T>
constexpr bool explicit_default_test() {
    return requires(Tuple<int, T> t) {
        t = {{}, {}};
    };
}

static_assert(explicit_default_test<int>());
static_assert(!explicit_default_test<ExplicitDefault>());

template <typename T>
constexpr bool explicit_conversion_test() {
    return requires(Tuple<int, T> t) {
        t = Tuple<double, int>(3.14, 0);
    };
}

static_assert(explicit_conversion_test<int>());
static_assert(!explicit_conversion_test<std::vector<int>>());

void test_tuple() {

    static_assert(std::is_default_constructible_v<Tuple<int, int>>);
    static_assert(!std::is_default_constructible_v<Tuple<NotDefaultConstructible, std::string>>);
    static_assert(!std::is_default_constructible_v<Tuple<int, NotDefaultConstructible>>);
    
    {
        Tuple<int, int> t1;
        assert(get<0>(t1) == 0);
        assert(get<1>(t1) == 0);
    }

    {
        Tuple<Accountant, Accountant> t2;
        assert(Accountant::constructed == 2);
    }
    assert(Accountant::destructed == 2);
    Accountant::constructed = Accountant::destructed = 0;


    static_assert(std::is_constructible_v<Tuple<int, double, char>, int, double, char>);
    static_assert(!std::is_constructible_v<Tuple<NotCopyableOnlyMovable, int>, const NotCopyableOnlyMovable&, int>);
    static_assert(std::is_constructible_v<Tuple<NotCopyableOnlyMovable, int>, NotCopyableOnlyMovable&&, int>);

    static_assert(std::is_constructible_v<Tuple<std::string, int>, const char*, int>);

    static_assert(std::is_constructible_v<Tuple<int&, char&>, int&, char&>);
    static_assert(std::is_constructible_v<Tuple<int&, const char&>, int&, char&>);
    static_assert(!std::is_constructible_v<Tuple<int&, char&>, int&, const char&>);
    static_assert(std::is_constructible_v<Tuple<int&, const char&>, int&, const char&>);
    static_assert(std::is_constructible_v<Tuple<int&, const char&>, int&, char>);

    static_assert(std::is_constructible_v<Tuple<int&&, char&>, int, char&>);
    static_assert(std::is_constructible_v<Tuple<int&&, char&>, int&&, char&>);
    static_assert(!std::is_constructible_v<Tuple<int&, char&&>, int&, char&>);
    static_assert(std::is_constructible_v<Tuple<int&, const char&>, int&, char&&>);

    {
        Accountant a;
        Tuple<Accountant, int> t(a, 5);
        assert(Accountant::copy_constructed == 1);
        
        Tuple t2 = t;
        assert(Accountant::copy_constructed == 2);

        get<1>(t) = 7;
        assert(get<1>(t) == 7);
        assert(get<1>(t2) == 5);

        t = t2;
        assert(Accountant::copy_assigned == 1);
        assert(Accountant::copy_constructed == 2);
        assert(get<1>(t) == 5);
        assert(get<1>(t2) == 5);

        t2 = std::move(t);
        assert(Accountant::move_assigned == 1);
        assert(Accountant::copy_assigned == 1);
        assert(Accountant::move_constructed == 0);
        assert(Accountant::copy_constructed == 2);
    
        const auto& t3 = t;
        t2 = std::move(t3);
        assert(Accountant::move_assigned == 1);
        assert(Accountant::copy_assigned == 2);
    }
    assert(Accountant::destructed == 3);
    Accountant::copy_constructed = Accountant::destructed = 0;
    Accountant::copy_assigned = Accountant::move_assigned = 0;

    {
        int x = 5;
        double d = 3.14;
        Tuple<int&, double&> t{x, d};

        int y = 6;
        double e = 2.72;
        Tuple<int&, double&> t2{y, e};

        t = t2; // x changed to 6
        assert(get<0>(t) == 6);
        assert(x == 6);

        y = 7;
        assert(get<0>(t) == 6);
        assert(get<0>(t2) == 7);

        get<0>(t) = 8; // x changed to 8
        assert(x == 8);
        assert(y == 7);
        assert(get<0>(t2) == 7);
    }


    {
        Accountant a;
        int x = 5;
        Tuple<Accountant&, const int&> t(a, x);
        assert(Accountant::copy_constructed == 0);
        assert(get<1>(t) == 5);

        x = 7;
        assert(get<1>(t) == 7);

        Tuple<Accountant, const int> t2 = t;
        assert(Accountant::copy_constructed == 1);

        static_assert(!std::is_assignable_v<decltype(t2), decltype(t2)>);
        static_assert(!std::is_assignable_v<decltype(t2), decltype(t)>);
        static_assert(!std::is_assignable_v<decltype(get<1>(t)), int>);
        assert(get<1>(t2) == 7);

        Tuple<Accountant, int> t3 = std::move(t);
        assert(Accountant::move_constructed == 0);
        assert(Accountant::copy_constructed == 2);
        assert(get<1>(t3) == 7);

        Tuple<Accountant&, const int> t4{a, x};
        Tuple<Accountant, int> t5 = std::move(t4);
        assert(Accountant::move_constructed == 0);
        assert(Accountant::copy_constructed == 3);

        Tuple<Accountant, int> t6 = std::move(t2);
        assert(Accountant::move_constructed == 1);
        assert(Accountant::copy_constructed == 3);
       
        Tuple<Accountant, const int&> t7{a, x};
        Tuple<Accountant, int> t8 = std::move(t7);
        assert(Accountant::move_constructed == 2);
        assert(Accountant::copy_constructed == 4);

        static_assert(!std::is_copy_assignable_v<decltype(t7)>);
        static_assert(!std::is_move_assignable_v<decltype(t7)>);

        t3 = std::move(t2);
        assert(Accountant::move_assigned == 1);
        assert(Accountant::copy_assigned == 0);

        get<1>(t3) = 8;

        t3 = std::move(t4);
        assert(Accountant::move_assigned == 1);
        assert(Accountant::copy_assigned == 1);
        assert(get<1>(t4) == 7);
    }
    assert(Accountant::destructed == 7);
    Accountant::copy_constructed = Accountant::move_constructed = Accountant::destructed = 0;
    Accountant::copy_assigned = Accountant::move_assigned = 0;

    static_assert(!std::is_default_constructible_v<Tuple<Accountant, int&>>);
    static_assert(std::is_copy_constructible_v<Tuple<Accountant, int&>>);
    static_assert(!std::is_copy_constructible_v<Tuple<Accountant, int&&>>);
    static_assert(!std::is_copy_constructible_v<Tuple<int, int&&>>);
    
    {
        Accountant a;
        int x = 5;
        Tuple<Accountant, int&&> t(std::move(a), std::move(x));
        assert(Accountant::move_constructed == 1);
        assert(Accountant::copy_constructed == 0);
        assert(get<1>(t) == 5);
       
        x = 7;
        assert(get<1>(t) == 7);
        
        get<1>(t) = 8;
        assert(x == 8);
 
        Tuple t2 = std::move(t);
        assert(Accountant::copy_constructed == 0);
        assert(Accountant::move_constructed == 2);

        assert(get<1>(t2) == 8);

        Tuple<Accountant&&, int&> t3{std::move(a), x};
        Tuple<Accountant, int> t4 = t3;
        assert(Accountant::copy_constructed == 1);
        assert(Accountant::move_constructed == 2);

        Tuple<Accountant, int> t5 = std::move(t3);
        assert(Accountant::copy_constructed == 1);
        assert(Accountant::move_constructed == 3);

        x = 15;
        t4 = t3;
        assert(Accountant::copy_assigned == 1);
        assert(Accountant::move_assigned == 0);
        assert(get<1>(t4) == 15);

        t5 = std::move(t3);
        assert(Accountant::copy_assigned == 1);
        assert(Accountant::move_assigned == 1);
        assert(get<1>(t5) == 15);

        get<int>(t4) = 22;

        t3 = t4;
        assert(Accountant::copy_assigned == 2);
        assert(Accountant::move_assigned == 1);
        assert(x == 22);

        get<int>(t5) = 33;

        t3 = std::move(t5);
        assert(Accountant::copy_assigned == 2);
        assert(Accountant::move_assigned == 2);
        assert(x == 33);
    }
    assert(Accountant::destructed == 5);
    Accountant::copy_constructed = Accountant::move_constructed = Accountant::destructed = 0;
    Accountant::copy_assigned = Accountant::move_assigned = 0;


    {
        int x = 5;
        Tuple<const int, int&, NeitherDefaultNorCopyConstructible> t(1, x, 3.14);
        
        static_assert(!std::is_assignable_v<decltype(get<0>(t)), int>);
        static_assert(!std::is_assignable_v<decltype(get<0>(t)), NeitherDefaultNorCopyConstructible>);

        get<1>(t) = 7;
        assert(x == 7);
    }

    static_assert(!std::is_copy_constructible_v<Tuple<int&, NeitherDefaultNorCopyConstructible>>);
    static_assert(!std::is_copy_constructible_v<Tuple< NeitherDefaultNorCopyConstructible, int&>>);
    static_assert(std::is_move_constructible_v<Tuple<int&, NeitherDefaultNorCopyConstructible>>);

    {
        int x = 5;
        NotDefaultConstructible ndc{1};

        Tuple<NotDefaultConstructible&, int&> tr{ndc, x};

        Tuple<NotDefaultConstructible, int> t2 = tr;

        x = 3;
        assert(get<1>(tr) == 3);
        assert(get<1>(t2) == 5);
    }

    static_assert(!std::is_constructible_v<Tuple<std::string&, int&>, Tuple<std::string, int>>);
    static_assert(!std::is_constructible_v<Tuple<std::string&&, int&&>, Tuple<std::string&, int&>>);
    static_assert(!std::is_constructible_v<Tuple<std::string&, int&&>, Tuple<std::string&, int&>>);
  static_assert(!std::is_constructible_v<Tuple<std::string&&>, Tuple<std::string&>>);

    static_assert(std::is_constructible_v<Tuple<const std::string&, int&>, Tuple<std::string&, int&>>);
  
    static_assert(!std::is_constructible_v<Tuple<std::string&, int&>, Tuple<const std::string&, int&>>);

    assert(new_called == 0);
    assert(delete_called == 0);
    
    // constructor from pair
    {
        std::pair<int, double> p{1, 3.14};
        Tuple t = p;
        get<int>(t) = 2;
        get<double>(t) = 2.72;

        int x = 1;
        double d = 3.14;
        std::pair<int&, double&> p2{x, d};
        Tuple t2 = p2;
        get<int&>(t2) = 2;
        assert(x == 2);
    
        Tuple<int, double> t3 = p2;
        get<int>(t3) = 3;
        assert(x == 2);

        Tuple<int&&, double&&> t4 = std::move(p);
        get<int&&>(t4) = 5;
        assert(p.first == 5);
    }

    // two references on the same object
    {
        int x = 1;
        Tuple<int, int&, const int&, int&&> t{x, x, x, std::move(x)};

        ++get<int>(t);
        ++get<int&>(t);
        ++get<int&&>(t);
        assert(get<const int&>(t) == 3);
        assert(get<int>(t) == 2);
        assert(get<3>(t) == 3);
        assert(x == 3);
    }

    // rvalue get
    {
        int x = 3;
        int y = 4;
        Tuple<int, long, int&, const int&, int&&> tuple(1, 2, x, x, std::move(y));

        static_assert(std::is_same_v<decltype(get<int&&>(tuple)), int&>);
        
        static_assert(std::is_same_v<decltype(get<int>(std::move(tuple))), int&&>);
        static_assert(std::is_same_v<decltype(get<int&>(std::move(tuple))), int&>);
        static_assert(std::is_same_v<decltype(get<const int&>(std::move(tuple))), const int&>);

        const auto& const_tuple = tuple;

        static_assert(std::is_same_v<decltype(get<int>(std::move(const_tuple))), const int&&>);
        static_assert(std::is_same_v<decltype(get<int&>(std::move(const_tuple))), int&>);
        static_assert(std::is_same_v<decltype(get<const int&>(std::move(const_tuple))), const int&>);
        static_assert(std::is_same_v<decltype(get<int&&>(std::move(const_tuple))), int&&>);

    }

    // tuple with one argument
    // get by type, CE if type is absent or ambiguous
    {
        Tuple<int> first(12);
        auto second = first;
        assert(get<0>(first) == get<0>(second));

        second = Tuple<int>(14);
        assert(get<0>(second) == 14);

        second = first;

        Tuple<double> third{3.14};
        second = third;

        assert(get<int>(second) == 3);

        first = 1;
        assert(get<int>(first) == 1);
    }

    // make_tuple, tie, forward_as_tuple
    {
        Tuple tuple = makeTuple(5, std::string("test"), std::vector<int>(2, 5));
        get<2>(tuple)[1] = 2;
        assert(get<2>(tuple)[1] == 2);


    }

    
    // tuple_cat
    {
        Tuple<Accountant, Accountant, int> t1;

        Tuple<Accountant, Accountant, int, Accountant> t2;

        Tuple<Accountant, Accountant, Accountant> t3;

        auto cat = tupleCat(t1, t2, std::move(t3));
      
        std::cout << Accountant::move_constructed << " " << Accountant::copy_constructed << "\n";
        assert(Accountant::move_constructed == 3);
        assert(Accountant::copy_constructed == 5);
    }

}

//#include <tuple>

//struct MyType {};

int main() {
    // test_tuple();
    std::cout << 0;
    return 0;
}

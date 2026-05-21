#include <algorithm>
#include <iostream>
#include <functional>
#include <utility>
#include <type_traits>

template <typename>
class Function;


template <typename Ret, typename... Args>
class Function<Ret(Args...)> {
  static constexpr size_t optimal_ = 16;
  
  void* fptr_;
  alignas(alignof(void*)) std::byte buffer_[optimal_];
  
  using caller_ptr_t = Ret(*)(void*, Args...);
  caller_ptr_t caller_ptr_;
  
  using destroyer_ptr_t = void(*)(void*);
  destroyer_ptr_t destroyer_ptr_;
  
  using copier_ptr_t = void*(*)(void*, const void*);
  copier_ptr_t copier_ptr_;
  
  using mover_ptr_t = void*(*)(void*, void*);
  mover_ptr_t mover_ptr_;
  
  const std::type_info* stored_type_ = nullptr;
  
  template <typename F>
  static Ret caller(void* fptr, Args... args) {
    return std::invoke(*static_cast<F*>(fptr), std::forward<Args>(args)...);
  }
  
  template <typename F>
  static void* mover(void* to, void* from) {
    if (to) {
      new (to) F(std::move(*static_cast<F*>(from)));
      return to;
    } else {
      return new F(std::move(*static_cast<F*>(from)));
    }
  }

  template <typename F>
  static void* copier(void* to, const void* from) {
    if (to) {
      new (to) F(*static_cast<const F*>(from));
      return to;
    } else {
      return new F(*static_cast<const F*>(from));
    }
  }
  
  template <typename F>
  static void destroyer(void* fptr) {
    if constexpr (sizeof(F) > optimal_) {
      delete static_cast<F*>(fptr);
    } else {
      static_cast<F*>(fptr)->~F();
    }
  }
  
  void null() noexcept {
    if (destroyer_ptr_) {
      destroyer_ptr_(fptr_);
      destroyer_ptr_ = nullptr;
      fptr_ = nullptr;
    }
    caller_ptr_ = nullptr;
    copier_ptr_ = nullptr;
    mover_ptr_ = nullptr;
    stored_type_ = nullptr;
  }
  
  
public:
  Function() noexcept
  : fptr_(nullptr)
  , destroyer_ptr_(nullptr)
  {}
  
  Function(std::nullptr_t) noexcept
  : fptr_(nullptr)
  , destroyer_ptr_(nullptr)
  {}
  
  template <typename Func>
  Function(Func&& func)
  requires(std::is_invocable_r_v<Ret, Func, Args...>)
  : caller_ptr_(&caller<std::decay_t<Func>>)
  , destroyer_ptr_(&destroyer<std::decay_t<Func>>)
  , copier_ptr_(&copier<std::decay_t<Func>>)
  , mover_ptr_(&mover<std::decay_t<Func>>)
  , stored_type_(&typeid(std::decay_t<Func>))
  {
    using Type = std::decay_t<Func>;
    
    if constexpr (sizeof(Type) > optimal_) {
      fptr_ = new Type(std::forward<Func>(func));
    } else {
      new (buffer_) Type(std::forward<Func>(func));
      fptr_ = buffer_;
    }
  }
  
  Function(const Function& other)
  : caller_ptr_(other.caller_ptr_)
  , destroyer_ptr_(other.destroyer_ptr_)
  , copier_ptr_(other.copier_ptr_)
  , mover_ptr_(other.mover_ptr_)
  , stored_type_(other.stored_type_)
  {
    if (other.fptr_) {
      if (other.fptr_ == other.buffer_) {
        copier_ptr_(buffer_, other.buffer_);
        fptr_ = buffer_;
      } else {
        fptr_ = copier_ptr_(nullptr, other.fptr_);
      }
    }
  }
  
  Function(Function&& other) noexcept
  : caller_ptr_(std::exchange(other.caller_ptr_, nullptr))
  , destroyer_ptr_(std::exchange(other.destroyer_ptr_, nullptr))
  , copier_ptr_(std::exchange(other.copier_ptr_, nullptr))
  , mover_ptr_(std::exchange(other.mover_ptr_, nullptr))
  , stored_type_(std::exchange(other.stored_type_, nullptr))
  {
    if (other.fptr_) {
      if (other.fptr_ == other.buffer_) {
        mover_ptr_(buffer_, other.buffer_);
        fptr_ = buffer_;
      } else {
        fptr_ = mover_ptr_(nullptr, other.fptr_);
      }
    }
    other.fptr_ = nullptr;
  }
  
  Function& operator=(const Function& other)
  {
    if (this != &other) {
      Function(other).swap(*this);
    }
    return *this;
  }
  
  Function& operator=(Function&& other) noexcept
   {
     if (this != &other) {
       Function temp(std::move(other));
       swap(temp);
     }
     return *this;
   }
  
  Function& operator=(std::nullptr_t) noexcept
  {
    null();
    return *this;
  }
  
  template <typename F>
  Function& operator=(F&& f)
  requires(!std::is_same_v<std::remove_cvref_t<F>, Function> and std::is_invocable_r_v<Ret, F, Args...>)
  {
    Function(std::forward<F>(f)).swap(*this);
    return *this;
  }
  
  template <typename F>
  Function& operator=(std::reference_wrapper<F> f) noexcept
  requires(std::is_invocable_r_v<Ret, F&, Args...>)
  {
    Function(f).swap(*this);
    return *this;
  }
  
  void swap(Function& other) noexcept
  {
    std::swap(fptr_, other.fptr_);
    std::swap(caller_ptr_, other.caller_ptr_);
    std::swap(destroyer_ptr_, other.destroyer_ptr_);
    std::swap(copier_ptr_, other.copier_ptr_);
    std::swap(mover_ptr_, other.mover_ptr_);
    std::swap(stored_type_, other.stored_type_);
    std::swap_ranges(std::begin(buffer_), std::end(buffer_), std::begin(other.buffer_));
    
    if (fptr_ == other.buffer_) {
      fptr_ = buffer_;
    }
    if (other.fptr_ == buffer_) {
      other.fptr_ = other.buffer_;
    }
  }
  
  Ret operator()(Args... args) const {
    if (!fptr_ or !caller_ptr_) {
      throw std::bad_function_call();
    }
    return caller_ptr_(fptr_, std::forward<Args>(args)...);
  }
  
  explicit operator bool() const noexcept
  {
    return fptr_ != nullptr;
  }
  
  const std::type_info& target_type() const noexcept
  {
    return (fptr_ ? *stored_type_ : typeid(void));
  }
  
  template <typename T>
  T* target() noexcept
  {
    return (target_type() == typeid(T) ? fptr_ : nullptr);
  }
  
  template <typename T>
  const T* target() const noexcept
  {
    return (target_type() == typeid(T) ? fptr_ : nullptr);
  }
  
  ~Function() {
    null();
  }
};

template <typename R, typename... Args>
bool operator==(const Function<R(Args...)>&f, std::nullptr_t) noexcept
{
  return !f;
}

template <typename Ret, typename... Args>
Function(Ret(*)(Args...)) -> Function<Ret(Args...)>;

template<typename F>
struct function_traits;

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...)> {
    using signature = R(Args...);
};

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...) const> {
    using signature = R(Args...);
};

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...) &> {
    using signature = R(Args...);
};

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...) noexcept> {
    using signature = R(Args...);
};

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...) const & > {
    using signature = R(Args...);
};

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...) const noexcept> {
    using signature = R(Args...);
};

template<typename R, typename G, typename... Args>
struct function_traits<R(G::*)(Args...) const & noexcept> {
    using signature = R(Args...);
};

template<typename R, typename G>
struct function_traits<R G::*> {
    using signature = R(G*);
};


template <typename F>
requires requires { &F::operator(); }
Function(F) -> Function<typename function_traits<decltype(&F::operator())>::signature>;





template <typename>
class MoveOnlyFunction;


template <typename Ret, typename... Args>
class MoveOnlyFunction<Ret(Args...)> {
  static constexpr size_t optimal_ = 16;
  
  void* fptr_ = nullptr;
  alignas(alignof(void*)) std::byte buffer_[optimal_];
  
  using caller_ptr_t = Ret(*)(void*, Args...);
  caller_ptr_t caller_ptr_;
  
  using destroyer_ptr_t = void(*)(void*);
  destroyer_ptr_t destroyer_ptr_;
  
  template <typename Func>
  static Ret caller(void* ptr, Args... args) {
    return std::invoke(*static_cast<Func*>(ptr), std::forward<Args>(args)...);
  }
  
  template <typename Func>
  static void destroyer(void* ptr) {
    if constexpr (sizeof(Func) > optimal_) {
      delete static_cast<Func*>(ptr);
    } else {
      static_cast<Func*>(ptr)->~Func();
    }
  }
  
  void null() noexcept {
    if (destroyer_ptr_) {
      destroyer_ptr_(fptr_);
      fptr_ = nullptr;
    }
    caller_ptr_ = nullptr;
    destroyer_ptr_ = nullptr;
  }
  
public:
  MoveOnlyFunction() noexcept = default;
  
  MoveOnlyFunction(std::nullptr_t) noexcept {}
  
  template <typename Func>
  MoveOnlyFunction(Func&& func)
  requires(std::is_invocable_r_v<Ret, Func, Args...>)
  : caller_ptr_(&caller<std::decay_t<Func>>)
  , destroyer_ptr_(&destroyer<std::decay_t<Func>>)
  {
    if constexpr (sizeof(std::decay_t<Func>) > optimal_) {
      fptr_ = new std::decay_t<Func>(std::forward<Func>(func));
    } else {
      new (buffer_) std::decay_t<Func>(std::forward<Func>(func));
      fptr_ = buffer_;
    }
  }
  
  MoveOnlyFunction(const MoveOnlyFunction& other) = delete;
  
  MoveOnlyFunction& operator=(const MoveOnlyFunction& other) = delete;
  
  MoveOnlyFunction(MoveOnlyFunction&& other) noexcept
  : fptr_(std::exchange(other.fptr_, nullptr))
  , caller_ptr_(std::exchange(other.caller_ptr_, nullptr))
  , destroyer_ptr_(std::exchange(other.destroyer_ptr_, nullptr))
  {
    std::copy(std::begin(other.buffer_), std::end(other.buffer_), std::begin(buffer_));
    if (fptr_ == other.buffer_) {
      fptr_ = buffer_;
    }
  }
  
  MoveOnlyFunction& operator=(MoveOnlyFunction&& other)
   {
     if (this != &other) {
       null();
       MoveOnlyFunction(std::move(other)).swap(*this);
     }
     return *this;
   }
  
  MoveOnlyFunction& operator=(std::nullptr_t) noexcept
  {
    null();
    return *this;
  }
  
  template <typename F>
  MoveOnlyFunction& operator=(F&& f)
  requires(!std::is_same_v<std::remove_cvref_t<F>, MoveOnlyFunction> and
           std::is_invocable_r_v<Ret, F, Args...>)
  {
    MoveOnlyFunction(std::forward<F>(f)).swap(*this);
    return *this;
  }
  
  void swap(MoveOnlyFunction& other) noexcept
  {
    std::swap(fptr_, other.fptr_);
    std::swap(caller_ptr_, other.caller_ptr_);
    std::swap(destroyer_ptr_, other.destroyer_ptr_);
    std::swap_ranges(std::begin(buffer_), std::end(buffer_), std::begin(other.buffer_));
    
    if (fptr_ == other.buffer_) {
      fptr_ = buffer_;
    }
    if (other.fptr_ == buffer_) {
      other.fptr_ = other.buffer_;
    }
  }
  
  Ret operator()(Args... args) const {
    if (!fptr_ || !caller_ptr_) {
      throw std::bad_function_call();
    }
    return caller_ptr_(fptr_, std::forward<Args>(args)...);
  }
  
  explicit operator bool() const noexcept
  {
    return fptr_ != nullptr;
  }
  
  ~MoveOnlyFunction() {
    null();
  }
};

template <typename R, typename... Args>
bool operator==(const MoveOnlyFunction<R(Args...)>&f, std::nullptr_t) noexcept
{
  return !f;
}


#include <functional>
#include <numeric>
#include <vector>
#include <cassert>
#include <iostream>
#include <memory>


// #include "function.hpp"

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

struct Helper {
  int method_1(int z) { return x + z; }
  int method_2(int z) { return y + z; }

  int x = 3;
  int y = 30;
};

int sum(int x, int y) {
  return x + y;
}

int multiply(int x, int y) {
  return x * y;
}

template <typename R, typename T>
R template_test_function(T t) {
  return static_cast<R>(t);
}

using namespace std::placeholders;

template <template <typename> typename Func, bool IsMoveOnly>
void test_function() {
  {
    Func<int(int, int)> func = sum;
    assert(func(5, 10) == 15);
  }

  {
    Func<int(double)> func = template_test_function<double, int>;
    assert(func(123.456) == 123);
  }

  auto lambda = [](int a, int b, int c, int d){
    return a * b + c * d;
  };

  {
    Func<int(int, int, int, int)> func = lambda;
    assert(func(2, 3, 20, 30) == 606);
  }

  auto func_ptr = +[]() {
    return 10;
  };

  {
    Func<int()> func = func_ptr;
    assert(func() == 10);
  }

  {
    Helper helper;

    Func<int(Helper&, int)> func = &Helper::method_1;
    assert(func(helper, 10) == 13);

    Func<int&(Helper&)> attr = &Helper::x;
    assert(attr(helper) == 3);

    attr(helper) = 10;
    assert(func(helper, 10) == 20);
  }

  {
    Func<int(int, int)> func = sum;
    assert(func(5, 10) == 15);
    func = multiply;
    assert(func(5, 10) == 50);
  }

  {
    Func<int(int, int)> func_1 = sum;
    Func<int(int, int)> func_2 = multiply;
    assert(func_1(5, 10) == 15);

    if constexpr (!IsMoveOnly) {
        func_1 = func_2;
        assert(func_1(5, 10) == 50);
        assert(func_2(5, 10) == 50);
    }

    func_1 = std::move(func_2);
    assert(func_1(5, 10) == 50);
    assert(!func_2);

    assert(std::move(func_1)(5, 10) == 50);
    assert(func_1(5, 10) == 50);
  }

  {
    Helper helper;
    Func<int(Helper&, int)> func = &Helper::method_1;
    Func<int&(Helper&)> attr = &Helper::x;

    assert(func(helper, 10) == 13);
    attr(helper) = 10;
    assert(func(helper, 10) == 20);

    func = &Helper::method_2;
    assert(func(helper, 10) == 40);

    attr = &Helper::y;
    assert(attr(helper) == 30);

    attr(helper) = 100;
    assert(func(helper, 10) == 110);
    
    int x = 25;
    auto lambda = [&x](Helper&) -> int& {
        return x;
    };
    attr = lambda;

    attr(helper) = 55;
    assert(x == 55);
  }

  {
    Func<int(int, int)> func;
    assert(func == nullptr);
    assert(!func);

    func = multiply;
    assert(func != nullptr);
    assert(func);

    auto f = std::move(func);
    assert(func == nullptr);
    assert(!func);
  }
 
  {
    Func<int(int, int)> func = sum;
    assert(std::invoke(func, 5, 10) == 15);
  }

  if constexpr (!IsMoveOnly) {
    Func func = sum;
    assert(func(5, 10) == 15);
  }

  if constexpr (!IsMoveOnly) {
    Func func = [](int x) { return x * x * x; };
    assert(func(10) == 1000);
  }

  {
    Func<int(int)> func = [&](int x) {
        return x == 0 ? 1 : x * func(x - 1);
    };
    assert(func(5) == 120);
  }

  // test that function does not allocate for small objects
  assert(new_called == 0);
  assert(delete_called == 0);

  {
    std::vector<int> vec = {1, 2, 3};
    Func<int()> func = [&]() {
      return std::accumulate(vec.begin(), vec.end(), 0);
    };

    assert(func() == 6);
    vec.push_back(4);
    assert(func() == 10);

    Func<int()> func2 = std::move(func);
    assert(func2() == 10);
  }

  if constexpr (!IsMoveOnly) {
    Func<void()> func;
    bool thrown = false;
    try {
        func();
    } catch (...) {
        thrown = true;
    }
    assert(thrown);
  }

  {
    Func<int(int, int)> func = std::bind(lambda, 2, _2, _1, 30);
    assert(func(20, 3) == 606);
  }

  struct TestStructConst {
      void const_method(int) const {}
      void nonconst_method(int) {}
  };

  static_assert(std::is_constructible_v<Func<void(TestStructConst&, int)>,
              decltype(&TestStructConst::const_method)>);
  static_assert(!std::is_constructible_v<Func<void(const TestStructConst&, int)>,
              decltype(&TestStructConst::nonconst_method)>);

  static_assert(std::is_assignable_v<Func<void(TestStructConst&, int)>,
              decltype(&TestStructConst::const_method)>);
  static_assert(!std::is_assignable_v<Func<void(const TestStructConst&, int)>,
              decltype(&TestStructConst::nonconst_method)>);

  static_assert(std::is_assignable_v<Func<void(TestStructConst&, int)>,
              Func<void(const TestStructConst&, int)>>);
  static_assert(!std::is_assignable_v<Func<void(const TestStructConst&, int)>,
              Func<void(TestStructConst&, int)>>);

  struct TestStructRvalue {
      void usual_method(int) {}
      void rvalue_method(int) && {}
      void lvalue_method(int) & {}
      void const_lvalue_method(int) const & {}
  };

  static_assert(std::is_constructible_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::usual_method)>);
  static_assert(std::is_constructible_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::rvalue_method)>);
  static_assert(!std::is_constructible_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::lvalue_method)>);
  static_assert(std::is_constructible_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::const_lvalue_method)>);

  static_assert(!std::is_invocable_v<Func<void(TestStructRvalue&&, int)>,
              TestStructRvalue&, int>);
  static_assert(std::is_invocable_v<Func<void(TestStructRvalue&&, int)>,
              TestStructRvalue&&, int>);
  static_assert(!std::is_invocable_v<Func<void(TestStructRvalue&, int)>,
              TestStructRvalue&&, int>);

  static_assert(std::is_assignable_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::usual_method)>);
  static_assert(std::is_assignable_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::rvalue_method)>);
  static_assert(!std::is_assignable_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::lvalue_method)>);
  static_assert(std::is_assignable_v<Func<void(TestStructRvalue&&, int)>,
              decltype(&TestStructRvalue::const_lvalue_method)>);

  static_assert(!std::is_assignable_v<Func<void(TestStructRvalue&&, int)>,
              Func<void(TestStructRvalue&, int)>>);
  static_assert(!std::is_assignable_v<Func<void(TestStructRvalue&, int)>,
              Func<void(TestStructRvalue&&, int)>>);
  static_assert(std::is_assignable_v<Func<void(TestStructRvalue&&, int)>,
              Func<void(const TestStructRvalue&, int)>>);
  static_assert(!std::is_assignable_v<Func<void(const TestStructRvalue&, int)>,
              Func<void(TestStructRvalue&&, int)>>);

  struct NotCopyable {
    std::unique_ptr<int> p{new int(5)};
    int operator()(int x) {
      return x + *p;
    }
  };

  if constexpr (IsMoveOnly) {
    NotCopyable nc;
    Func<int(int)> f = std::move(nc);
    assert(f(5) == 10);

    Func<int(int)> f2 = std::move(f);
    f = NotCopyable{std::make_unique<int>(7)};
    assert(f(5) == 12);
    assert(f2(5) == 10);
  }

  // doesn't work for std::function
  //static_assert(IsMoveOnly || !std::is_constructible_v<Func<int(int)>, NotCopyable>);
  //static_assert(IsMoveOnly || !std::is_assignable_v<Func<int(int)>, NotCopyable&>);

  new_called = 0;
  delete_called = 0;
}

//template <typename T>
//using Function = std::function<T>;

//template <typename T>
//using MoveOnlyFunction = std::move_only_function<T>;


int main() {
  test_function<Function, false>();
  test_function<MoveOnlyFunction, true>();
  
    std::cout << 0;
}

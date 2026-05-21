#include <limits>
#include <memory>
#include <stdexcept>
#include <span>
#include <type_traits>

struct DynamicIterState {
  size_t capacity;
  size_t index;
};

struct StaticIterState {
  size_t index;
};

constexpr size_t DYNAMIC_CAPACITY = std::numeric_limits<size_t>::max();

template <typename T, size_t Capacity, bool isConst>
class iter_of_mine {
  static constexpr bool dynamic = Capacity == DYNAMIC_CAPACITY;
  
  using State = std::conditional_t<dynamic, DynamicIterState, StaticIterState>;
  
  using Ptr = std::conditional_t<isConst, const T*, T*>;
  Ptr ptr;
  State state;
  size_t head;
  
  friend class iter_of_mine<T, Capacity, true>;
  
  size_t capacity() const {
    if constexpr(dynamic) {
      return state.capacity;
    } else {
      return Capacity;
    }
  }

public:
  using difference_type = std::ptrdiff_t;
  using value_type = std::conditional_t<isConst, const T, T>;
  using pointer = std::conditional_t<isConst, const T*, T*>;
  using reference = std::conditional_t<isConst, const T&, T&>;
  using iterator_category = std::random_access_iterator_tag;
  
  iter_of_mine(std::span<std::conditional_t<isConst, const T, T>> data, size_t capacity, size_t index, size_t head)
  requires(dynamic)
  : ptr(data.data()), state(capacity, index), head(head)
  {}
  
  iter_of_mine(std::span<std::conditional_t<isConst, const T, T>> data, size_t index, size_t head)
  requires(!dynamic)
  : ptr(data.data()), state(index), head(head)
  {}
  
  iter_of_mine(const iter_of_mine<T, Capacity, false>& other)
  : ptr(other.ptr), state(other.state), head(other.head)
  {}
  
  iter_of_mine(const iter_of_mine<T, Capacity, true>& other)
  requires(isConst)
  : ptr(other.ptr), state(other.state), head(other.head)
  {}
  
  iter_of_mine& operator=(const iter_of_mine& other) = default;
  
  iter_of_mine& operator--() {
    --state.index;
    return *this;
  }
  
  iter_of_mine operator--(int) {
    iter_of_mine ret = *this;
    --(*this);
    return ret;
  }
  
  iter_of_mine& operator++() {
    ++state.index;
    return *this;
  }
  
  iter_of_mine operator++(int) {
    iter_of_mine ret = *this;
    ++(*this);
    return ret;
  }
  
  friend iter_of_mine operator+(const iter_of_mine& other, int shift) {
    auto ret = other;
    ret.state.index += shift;
    return ret;
  }
  
  friend iter_of_mine operator+(int shift, const iter_of_mine& other) {
    return other + shift;
  }
  
  friend iter_of_mine operator-(const iter_of_mine& other, int shift) {
    return other + (-shift);
  }
  
  friend iter_of_mine operator-(int shift, const iter_of_mine& other) {
    return other + (-shift);
  }
  
  reference operator*() const {
    return ptr[(head + state.index) % capacity()];
  }
  
  pointer operator->() const {
    return &ptr[(head + state.index) % capacity()];
  }
  
  difference_type operator-(const iter_of_mine& other) const {
    return state.index - other.state.index;
  }
  
  size_t place() const {
    return state.index;
  }
  
  bool operator==(const iter_of_mine& other) const {
    return ptr == other.ptr and state.index == other.state.index;
  }
  
  bool operator!=(const iter_of_mine& other) const {
    return !(*this == other);
  }
  
  bool operator<(const iter_of_mine& other) const {
    return ptr == other.ptr and state.index < other.state.index;
  }
  
  bool operator>(const iter_of_mine& other) const {
    return other < *this;
  }
  
  bool operator<=(const iter_of_mine& other) const {
    return !(*this > other);
  }
  
  bool operator>=(const iter_of_mine& other) const {
    return !(*this < other);
  }
};


template <typename T>
struct DynamicCapacityState {
  std::unique_ptr<std::byte[]> storage;
  size_t capacity;
  
  DynamicCapacityState() = default;
  
  DynamicCapacityState(size_t capacity)
  : storage(std::make_unique<std::byte[]>(sizeof(T) * capacity))
  , capacity(capacity)
  {}
  
  DynamicCapacityState(const DynamicCapacityState& other, size_t head, size_t size)
      : DynamicCapacityState(other.capacity)
  {
    for (size_t i = 0; i < size; ++i) {
      size_t index = (head + i) % capacity;
      try {
        new (data() + index) T(other.data()[index]);
      } catch(...) {
        annihilate(head, i);
        throw;
      }
    }
  }

  ~DynamicCapacityState() = default;
  
  void swap(DynamicCapacityState& other, size_t, size_t) noexcept {
    std::swap(storage, other.storage);
    std::swap(capacity, other.capacity);
  }
  
  T* data() noexcept {
    return reinterpret_cast<T*>(storage.get());
  }

  const T* data() const noexcept {
    return reinterpret_cast<const T*>(storage.get());
  }
  
  template <typename... Args>
  void emplace(size_t index, Args&&... args) {
    if (index >= capacity) {
      throw std::out_of_range("Index is out of range");
    } else {
      new (data() + index) T(std::forward<Args>(args)...);
    }
  }
  
  void annihilate(size_t head, size_t size) noexcept {
    for (size_t i = 0; i < size; ++i) {
      (data() + (head + i) % capacity)->~T();
    }
  }
  
  void destroy(size_t index) noexcept {
    if (index < capacity) {
      (data() + index)->~T();
    }
  }
};

template <typename T, size_t Capacity>
struct StaticCapacityState {
  alignas(T) std::byte storage[sizeof(T) * Capacity];
  
  StaticCapacityState() = default;
  
  StaticCapacityState(const StaticCapacityState& other, size_t head, size_t size)
  {
    for (size_t i = 0; i < size; ++i) {
      size_t index = (head + i) % Capacity;
      try {
        new (data() + index) T(other.data()[index]);
      } catch(...) {
        annihilate(head, i);
        throw;
      }
    }
  }
  
  void swap(StaticCapacityState& other, size_t size_this, size_t size_other) noexcept {
    size_t mn = std::min(size_this, size_other);
    for (size_t i = 0; i < mn; ++i) {
      std::swap(data()[i], other.data()[i]);
    }
    if (size_this < size_other) {
      for (size_t i = size_this; i < size_other; ++i) {
        emplace(i, std::move(other.data()[i]));
        other.destroy(i);
      }
    } else {
      for (size_t i = size_other; i < size_this; ++i) {
        other.emplace(i, std::move(data()[i]));
        destroy(i);
      }
    }
  }
  
  template <typename... Args>
  void emplace(size_t index, Args&&... args) {
    if (index >= Capacity) {
      throw std::out_of_range("Index is out of range");
    } else {
      new (data() + index) T(std::forward<Args>(args)...);
    }
  }
  
  void annihilate(size_t head, size_t size) noexcept {
    for (size_t i = 0; i < size; ++i) {
      (data() + (head + i) % Capacity)->~T();
    }
  }
  
  void destroy(size_t index) noexcept {
    if (index < Capacity) {
      (data() + index)->~T();
    }
  }
  
  T* data() noexcept {
    return reinterpret_cast<T*>(storage);
  }

  const T* data() const noexcept {
    return reinterpret_cast<const T*>(storage);
  }
};


template <typename T, size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
  
  static constexpr bool dynamic = Capacity == DYNAMIC_CAPACITY;
  
  using State = std::conditional_t<dynamic, DynamicCapacityState<T>, StaticCapacityState<T, Capacity>>;
  
  size_t sz{0};
  size_t head{0};
  size_t tail;
  State state;
  
public:
  CircularBuffer()
  requires(!dynamic)
  : tail(Capacity - 1)
  {}
  
  explicit CircularBuffer(size_t capacity)
  requires(dynamic)
  : tail(capacity - 1)
  , state(capacity)
  {}
  
  explicit CircularBuffer(size_t capacity)
  requires(!dynamic)
  : tail(capacity - 1)
  {
    if (Capacity != capacity) {
      throw std::invalid_argument("Invalid Capacity");
    }
  }
  
  CircularBuffer(const CircularBuffer& other)
      : sz(other.sz)
      , head(other.head)
      , tail(other.tail)
      , state(other.state, other.head, other.sz)
  {}
  
  CircularBuffer& operator=(const CircularBuffer& other) {
    if (this != &other) {
      CircularBuffer copy(other);
      swap(copy);
    }
    return *this;
  }
  
  ~CircularBuffer() noexcept {
    state.annihilate(head, sz);
  }
  
  
  
  using iterator = iter_of_mine<T, Capacity, false>;
  using const_iterator = iter_of_mine<T, Capacity, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  
  iterator begin() noexcept {
    if constexpr(dynamic) {
      return iterator(std::span(state.data(), capacity()), capacity(), 0, head);
    } else {
      return iterator(std::span(state.data(), capacity()), 0, head);
    }
  }
     
  const_iterator begin() const noexcept {
    if constexpr(dynamic) {
      return const_iterator(std::span(state.data(), capacity()), capacity(), 0, head);
    } else {
      return const_iterator(std::span(state.data(), capacity()), 0, head);
    }
  }
  
  const_iterator cbegin() const noexcept {
    return const_iterator(begin());
  }
  
  iterator end() noexcept {
    if constexpr(dynamic) {
      return iterator(std::span(state.data(), capacity()), capacity(), sz, head);
    } else {
      return iterator(std::span(state.data(), capacity()), sz, head);
    }
  }
  
  const_iterator end() const noexcept {
    if constexpr(dynamic) {
      return iterator(std::span(state.data(), capacity()), capacity(), sz, head);
    } else {
      return iterator(std::span(state.data(), capacity()), sz, head);
    }
  }
  
  const_iterator cend() const noexcept {
    return const_iterator(end());
  }
  
  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  
  const_reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }
  
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(rbegin());
  }
  
  reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  
  const_reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }
  
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(rend());
  }

  
  void swap(CircularBuffer& other) noexcept {
    state.swap(other.state, sz, other.sz);
    std::swap(sz, other.sz);
    std::swap(head, other.head);
    std::swap(tail, other.tail);
  }
  
  void push_back(const T& element) {
    size_t new_tail = (tail + 1) % capacity();

    if (full()) {
      T safe = state.data()[head];
      state.destroy(head);
      try {
        state.emplace(new_tail, element);
      } catch(...) {
        state.emplace(head, std::move(safe));
        throw;
      }
      head = (head + 1) % capacity();
    } else {
      state.emplace(new_tail, element);
      sz = std::min(sz + 1, capacity());
    }
        
    tail = new_tail;
  }
  
  void push_front(const T& element) {
    size_t new_head = (head - 1 + capacity()) % capacity();
    
    if (full()) {
      T safe = state.data()[tail];
      state.destroy(tail);
      try {
        state.emplace(new_head, element);
      } catch(...) {
        state.emplace(tail, std::move(safe));
        throw;
      }
      tail = (tail - 1 + capacity()) % capacity();
    } else {
      state.emplace(new_head, element);
      sz = std::min(sz + 1, capacity());
    }
        
    head = new_head;
  }
  
  void pop_back() {
    erase(end() - 1);
  }
  
  void pop_front() {
    erase(begin());
  }
    
  iterator insert(iterator pos, const T& value) {
    auto cap = capacity();
    size_t shift = pos.place();
    size_t insert_index = (shift + head) % capacity();
    
    if (full()) {
      if (pos == begin()) {
        return end();
      }
      
      state.destroy(head);
      head = (head + 1) % capacity();
      --sz;
      --shift;
    }
    
    size_t new_tail = (tail + 1) % capacity();
    size_t current = new_tail;
    
    while (current != insert_index) {
      size_t prev = (current - 1 + cap) % cap ;
      state.emplace(current, std::move(state.data()[prev]));
      state.destroy(prev);
      current = prev;
    }
    
    state.emplace(insert_index, value);
    tail = new_tail;
    ++sz;
    
    if constexpr(dynamic) {
      return iterator(std::span(state.data(), cap), cap, shift, head);
    } else {
      return iterator(std::span(state.data(), cap), shift, head);
    }
  }
  
  iterator erase(iterator pos) {
    size_t shift = pos.place();
    size_t erasion_index = (shift + head) % capacity();
    state.destroy(erasion_index);
    
    size_t current = erasion_index;
    while (current != tail) {
      size_t next = (current + 1) % capacity();
      state.emplace(current, std::move(state.data()[next]));
      state.destroy(next);
      current = next;
    }
    
    tail = (tail - 1 + capacity()) % capacity();
    --sz;
    
    if constexpr(dynamic) {
      return iterator(std::span(state.data(), capacity()), capacity(), shift, head);
    } else {
      return iterator(std::span(state.data(), capacity()), shift, head);
    }
  }
  
  size_t size() const noexcept {
    return sz;
  }
  
  size_t capacity() const noexcept {
    if constexpr(dynamic) {
      return state.capacity;
    } else {
      return Capacity;
    }
  }
  
  bool empty() const noexcept {
    return sz == 0;
  }
  
  bool full() const noexcept {
    if constexpr(dynamic) {
      return sz == state.capacity;
    } else {
      return sz == Capacity;
    }
  }
  
  T& operator[](size_t index) {
    return state.data()[(head + index) % capacity()];
  }
  
  const T& operator[](size_t index) const {
    return state.data()[(head + index) % capacity()];
  }
  
  T& at(size_t index) {
    if (index >= sz) {
      throw std::out_of_range("Out of borders");
    }
    return state.data()[(head + index) % capacity()];
  }
  
  const T& at(size_t index) const {
    if (index >= sz) {
      throw std::out_of_range("Out of borders");
    }
    return state.data()[(head + index) % capacity()];
  }
};






//#include "circular_buffer.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <vector>

enum class WhenCapacityKnown {
  KNOWN_AT_COMPILETIME,
  KNOWN_AT_RUNTIME,
};

// Acts like a factory for the CircularBuffer
template<typename T, WhenCapacityKnown When, std::size_t Capacity>
auto create_buffer() {
  constexpr std::size_t TemplatedCapacity =
      When == WhenCapacityKnown::KNOWN_AT_COMPILETIME ? Capacity : DYNAMIC_CAPACITY;

  return CircularBuffer<T, TemplatedCapacity>(Capacity);
}


// Here we have some helper functions for the actual tests ('assert_{something}')

template<typename Buffer>
void assert_size_and_capacity(
    const Buffer& buffer,
    std::size_t expected_capacity,
    std::size_t expected_size
)
{
  assert(buffer.size() == expected_size);
  assert(buffer.capacity() == expected_capacity);
  assert(buffer.empty() == (expected_size == 0));
  assert(buffer.full() == (expected_size == expected_capacity));
}

template<typename Buffer, typename U, std::size_t ExpectedContentsSize>
void assert_contents(const Buffer& buffer, const U (&expected_contents)[ExpectedContentsSize]) {
  assert(buffer.size() == ExpectedContentsSize);

  for (std::size_t i = 0; i < ExpectedContentsSize; ++i) {
    assert(buffer[i] == expected_contents[i]);
  }
}

template<typename Buffer, typename Range>
void assert_contents(const Buffer& buffer, Range&& range) {
  std::size_t i = 0;

  for (const auto& value: range) {
    assert(buffer[i] == value);
    ++i;
  }

  assert(buffer.size() == i);
}

namespace TestsByPetialetia {

template<WhenCapacityKnown When>
void test_empty() {
  auto buffer = create_buffer<int, When, 32>();
  assert_size_and_capacity(buffer, 32, 0);
}

// This struct is a wrapper for an int value (allocated by a call to 'new')
struct HeapInt {
public:
  HeapInt() = delete;

  HeapInt(int value): ptr(new int(value))
  {
  }

  HeapInt(const HeapInt& other): ptr(new int(*other.ptr))
  {
  }

  HeapInt& operator=(const HeapInt& other) {
    auto copy = other;
    swap(copy);

    return *this;
  }

  void swap(HeapInt& other) {
    std::swap(ptr, other.ptr);
  }

  friend bool operator==(const HeapInt& left, int right) {
    return *left.ptr == right;
  }

  friend bool operator==(const HeapInt& left, const HeapInt& right) {
    return *left.ptr == *right.ptr;
  }

  ~HeapInt() {
    delete ptr;
  }

private:
  int* ptr;
};

template<WhenCapacityKnown When>
void test_push_and_pop() {
  constexpr std::size_t capacity = 4;

  auto buffer = create_buffer<HeapInt, When, capacity>();

  buffer.push_back(2);
  assert_size_and_capacity(buffer, capacity, 1);
  assert_contents(buffer, {2});

  buffer.push_front(1);
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {1, 2});

  buffer.push_back(3);
  buffer.push_front(0);
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {0, 1, 2, 3});

  buffer.push_back(4);
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {1, 2, 3, 4});

  buffer.push_front(0);
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {0, 1, 2, 3});

  buffer.pop_back();
  assert_size_and_capacity(buffer, capacity, 3);
  assert_contents(buffer, {0, 1, 2});

  buffer.pop_front();
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {1, 2});

  buffer.pop_back();
  buffer.pop_front();
  assert_size_and_capacity(buffer, capacity, 0);

  buffer.push_front(42);
  assert_size_and_capacity(buffer, capacity, 1);
  assert_contents(buffer, {42});

  for (std::size_t i = 0; i < 10; ++i) {
    buffer.push_back(i);
    buffer.push_front(i);
  }

  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {9, 0, 42, 0});

  buffer.pop_front();
  buffer.pop_back();
  buffer.pop_front();
  assert_size_and_capacity(buffer, capacity, 1);
  assert_contents(buffer, {42});
}

template<WhenCapacityKnown When>
void test_indexing() {
  constexpr std::size_t capacity = 4;
  auto buffer = create_buffer<HeapInt, When, capacity>();

  buffer.push_back(1);
  buffer.push_front(0);

  buffer[1] = 1337;
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {0, 1337});

  buffer[0] = 42;
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {42, 1337});

  buffer.push_back(2);
  buffer.push_back(3);

  buffer[2] = 314;
  buffer[3] = 2718;
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {42, 1337, 314, 2718});

  const auto& const_buffer = buffer;
  const auto& value = const_buffer[0];
  buffer[0] = 96;
  assert(value == 96);
}

template<WhenCapacityKnown When>
void test_at() {
  constexpr std::size_t capacity = 32;
  auto buffer = create_buffer<HeapInt, When, capacity>();

  buffer.push_back(1);
  buffer.push_front(0);

  assert(buffer[0] == 0);
  assert(buffer.at(0) == buffer[0]);
  assert(buffer[1] == 1);
  assert(buffer.at(1) == buffer[1]);

  for (std::size_t i = 2; i < capacity; ++i) {
    try {
      buffer.at(i);
      assert(false);
    } catch(std::out_of_range&) {
    }
  }

  buffer.at(0) = 42;
  buffer.at(1) = 1337;
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {42, 1337});
}

template<WhenCapacityKnown When>
void test_copy_and_assignment() {
  constexpr std::size_t capacity = 4;
  auto buffer = create_buffer<HeapInt, When, capacity>();

  buffer.push_back(2);
  buffer.push_back(3);

  auto other_buffer = buffer;
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {2, 3});
  assert_size_and_capacity(other_buffer, capacity, 2);
  assert_contents(other_buffer, {2, 3});

  other_buffer.push_front(1);
  other_buffer.push_front(0);
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {2, 3});
  assert_size_and_capacity(other_buffer, capacity, 4);
  assert_contents(other_buffer, {0, 1, 2, 3});

  buffer = other_buffer;
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {0, 1, 2, 3});
  assert_size_and_capacity(other_buffer, capacity, 4);
  assert_contents(other_buffer, {0, 1, 2, 3});
}

void test_constructor_exception() {
  try {
    std::ignore = CircularBuffer<int, 32>(42);
    assert(false);
  } catch(std::invalid_argument&) {
  }
}

// Acts like a wrapper for any type
template<typename T>
struct Value {
  T value;

  operator T() const {
    return value;
  }
};

template<WhenCapacityKnown When>
void test_iterators() {
  constexpr size_t capacity = 32;
  auto buffer = create_buffer<HeapInt, When, capacity>();

  for (std::size_t i = 0; i < buffer.capacity() / 2; ++i) {
    buffer.push_front(buffer.capacity() / 2 - i - 1);
    buffer.push_back(buffer.capacity() / 2 + i);
  }

  using Iter = typename decltype(buffer)::iterator;

  // Formal checks
  static_assert(std::is_same_v<Iter, decltype(buffer.begin())>);
  static_assert(std::is_same_v<Iter, decltype(buffer.end())>);

  Iter begin_iter = buffer.begin();
  Iter end_iter = buffer.end();

  assert(end_iter - begin_iter == static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity()));
  assert(begin_iter - end_iter == -static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity()));
  assert(begin_iter + buffer.capacity() == end_iter);
  assert(buffer.capacity() + begin_iter == end_iter);
  assert(end_iter - buffer.capacity() == begin_iter);
  assert(begin_iter < end_iter);

  for (std::size_t i = 0; i < buffer.capacity(); ++i) {
    Iter cur_iter = begin_iter + i;

    assert(*cur_iter == i);
    assert(cur_iter - begin_iter == static_cast<std::iterator_traits<Iter>::difference_type>(i));
    assert(begin_iter - cur_iter == -static_cast<std::iterator_traits<Iter>::difference_type>(i));
    assert(cur_iter - i == begin_iter);
    assert(end_iter - cur_iter == static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity() - i));
    assert(cur_iter - end_iter == -static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity() - i));
    assert(cur_iter + (buffer.capacity() - i) == end_iter);
    assert((buffer.capacity() - i) + cur_iter == end_iter);
    assert(begin_iter <= cur_iter);
    assert(cur_iter < end_iter);
    assert(cur_iter == cur_iter);
  }

  using ReverseIter = typename decltype(buffer)::reverse_iterator;

  // Formal checks
  static_assert(std::is_same_v<ReverseIter, decltype(buffer.rbegin())>);
  static_assert(std::is_same_v<ReverseIter, decltype(buffer.rend())>);

  ReverseIter rbegin_iter = buffer.rbegin();
  ReverseIter rend_iter = buffer.rend();

  assert(rend_iter - rbegin_iter == static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity()));
  assert(rbegin_iter - rend_iter == -static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity()));
  assert(rbegin_iter + buffer.capacity() == rend_iter);
  assert(buffer.capacity() + rbegin_iter == rend_iter);
  assert(rend_iter - buffer.capacity() == rbegin_iter);
  assert(rbegin_iter < rend_iter);

  for (std::size_t i = 0; i < buffer.capacity(); ++i) {
    auto cur_iter = rbegin_iter + i;

    assert(*cur_iter == buffer.capacity() - i - 1);
    assert(cur_iter - rbegin_iter == static_cast<std::iterator_traits<Iter>::difference_type>(i));
    assert(rbegin_iter - cur_iter == -static_cast<std::iterator_traits<Iter>::difference_type>(i));
    assert(cur_iter - i == rbegin_iter);
    assert(rend_iter - cur_iter == static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity() - i));
    assert(cur_iter - rend_iter == -static_cast<std::iterator_traits<Iter>::difference_type>(buffer.capacity() - i));
    assert(cur_iter + (buffer.capacity() - i) == rend_iter);
    assert((buffer.capacity() - i) + cur_iter == rend_iter);
    assert(rbegin_iter <= cur_iter);
    assert(cur_iter < rend_iter);
    assert(cur_iter == cur_iter);
  }

  std::vector<HeapInt> storage;

  for (const auto& value : buffer) {
    storage.push_back(value);
  }

  assert(storage.size() == buffer.capacity());
  assert(storage.size() == 32);

  for (std::size_t i = 0; i < storage.size(); ++i) {
    assert(storage[i] == i);
  }

  using ConstIter = typename decltype(buffer)::const_iterator;
  const auto& const_buffer = buffer;

  // Formal checks
  static_assert(std::is_same_v<ConstIter, decltype(buffer.cbegin())>);
  static_assert(std::is_same_v<ConstIter, decltype(buffer.cend())>);
  static_assert(std::is_same_v<ConstIter, decltype(const_buffer.cbegin())>);
  static_assert(std::is_same_v<ConstIter, decltype(const_buffer.cend())>);
  static_assert(std::is_same_v<ConstIter, decltype(const_buffer.begin())>);
  static_assert(std::is_same_v<ConstIter, decltype(const_buffer.end())>);

  static_assert(!std::is_assignable_v<decltype(*const_buffer.begin()), HeapInt>);
  static_assert(std::is_assignable_v<decltype(*buffer.begin()), HeapInt>);
  static_assert(!std::is_assignable_v<decltype(*buffer.cbegin()), HeapInt>);

  static_assert(!std::is_assignable_v<decltype(*const_buffer.end()), HeapInt>);
  static_assert(std::is_assignable_v<decltype(*buffer.end()), HeapInt>);
  static_assert(!std::is_assignable_v<decltype(*buffer.cend()), HeapInt>);

  std::for_each(buffer.begin(), buffer.end(), [](HeapInt& value) { value = 1337; });
  auto expected = std::vector(32, 1337);

  assert_size_and_capacity(buffer, capacity, 32);
  assert_contents(buffer, expected);

  *(buffer.begin() + 10) = 42;
  expected[10] = 42;

  assert_size_and_capacity(buffer, capacity, 32);
  assert_contents(buffer, expected);

  auto values_buffer = create_buffer<Value<int>, When, capacity>();
  values_buffer.push_back({1});
  values_buffer.push_front({0});

  values_buffer.begin()->value = 42;
  (values_buffer.begin() + 1)->value = 1337;
  assert_size_and_capacity(values_buffer, capacity, 2);
  assert_contents(values_buffer, {42, 1337});

  const auto& value_ref_arrow = values_buffer.cbegin()->value;
  const auto& value_ref_star = *values_buffer.cbegin();
  values_buffer.begin()->value = 43;
  assert(value_ref_arrow == 43);
  assert(value_ref_star == 43);

  using ConstReverseIter = typename decltype(buffer)::const_reverse_iterator;

  // Formal checks
  static_assert(std::is_same_v<ConstReverseIter, decltype(buffer.crbegin())>);
  static_assert(std::is_same_v<ConstReverseIter, decltype(buffer.crend())>);
  static_assert(std::is_same_v<ConstReverseIter, decltype(const_buffer.crbegin())>);
  static_assert(std::is_same_v<ConstReverseIter, decltype(const_buffer.crend())>);
  static_assert(std::is_same_v<ConstReverseIter, decltype(const_buffer.rbegin())>);
  static_assert(std::is_same_v<ConstReverseIter, decltype(const_buffer.rend())>);

  static_assert(!std::is_assignable_v<decltype(*const_buffer.rbegin()), HeapInt>);
  static_assert(std::is_assignable_v<decltype(*buffer.rbegin()), HeapInt>);
  static_assert(!std::is_assignable_v<decltype(*buffer.crbegin()), HeapInt>);

  static_assert(!std::is_assignable_v<decltype(*const_buffer.rend()), HeapInt>);
  static_assert(std::is_assignable_v<decltype(*buffer.rend()), HeapInt>);
  static_assert(!std::is_assignable_v<decltype(*buffer.crend()), HeapInt>);
}

template<WhenCapacityKnown When>
void test_erase() {
  constexpr size_t capacity = 32;
  auto buffer = create_buffer<HeapInt, When, capacity>();

  buffer.push_front(1);
  buffer.push_front(0);
  buffer.push_back(42);
  buffer.push_back(2);
  buffer.push_back(3);

  buffer.erase(buffer.begin() + 2);
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, {0, 1, 2, 3});

  buffer.erase(--buffer.end());
  assert_size_and_capacity(buffer, capacity, 3);
  assert_contents(buffer, {0, 1, 2});

  buffer.erase(buffer.begin());
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, {1, 2});

  buffer.push_front(0);

  for (std::size_t i = 3; i < buffer.capacity(); ++i) {
    buffer.push_back(i);
  }

  buffer.erase(buffer.begin() + 9);

  assert_size_and_capacity(buffer, capacity, 31);
  assert_contents(buffer, std::views::iota(0, 32) | std::views::filter([](int i) { return i != 9; }));
}

template<WhenCapacityKnown When>
void test_insert() {
  constexpr size_t capacity = 32;
  auto buffer = create_buffer<HeapInt, When, capacity>();
  std::vector<int> expected{};

  buffer.insert(buffer.begin(), 2);
  expected.insert(expected.begin(), 2);
  assert_size_and_capacity(buffer, capacity, 1);
  assert_contents(buffer, expected);

  buffer.insert(buffer.begin(), 0);
  expected.insert(expected.begin(), 0);
  assert_size_and_capacity(buffer, capacity, 2);
  assert_contents(buffer, expected);

  buffer.insert(buffer.begin() + 1, 1);
  expected.insert(expected.begin() + 1, 1);
  assert_size_and_capacity(buffer, capacity, 3);
  assert_contents(buffer, expected);

  buffer.insert(buffer.end(), 3);
  expected.insert(expected.end(), 3);
  assert_size_and_capacity(buffer, capacity, 4);
  assert_contents(buffer, expected);

  for (std::size_t i = 4; i < buffer.capacity(); ++i) {
    size_t offset_from_end = i % 10;

    buffer.insert(buffer.end() - offset_from_end, i);
    expected.insert(expected.end() - offset_from_end, i);
    assert_size_and_capacity(buffer, capacity, i + 1);
    assert_contents(buffer, expected);
  }

  for (std::size_t i = buffer.size(); i > 0; --i) {
    auto value = i + 100;

    buffer.insert(buffer.begin() + i, value);
    expected.erase(expected.begin());
    expected.insert(expected.begin() + (i - 1), value);

    assert_size_and_capacity(buffer, capacity, 32);
    assert_contents(buffer, expected);
  }

  buffer.insert(buffer.begin(), 1337); // if full(), insert on begin() should do nothing
  assert_size_and_capacity(buffer, capacity, 32);
  assert_contents(buffer, expected);
}

// Emulates a class which can throw from a copy constructor
template<typename T>
struct Explosive {
  bool should_explode;
  T value;

  Explosive() = delete;

  Explosive(bool should_explode, const T& value): should_explode(should_explode), value(value) {
  }

  Explosive(const Explosive& other): should_explode(other.should_explode), value(other.value) {
    if (other.should_explode)
      throw "boom";
  }

  Explosive& operator=(const Explosive& other) {
    Explosive copy = other;
    swap(copy);

    return *this;
  }

  void swap(Explosive& other) {
    std::swap(should_explode, other.should_explode);
  }

  operator T() const {
    return value;
  }
};

template<WhenCapacityKnown When>
void test_exceptions() {
  constexpr std::size_t capacity = 32;
  auto buffer = create_buffer<Explosive<std::size_t>, When, capacity>();

  for (std::size_t i = 0; i < buffer.capacity(); ++i) {
    try {
      if (i % 2 == 0) {
        buffer.push_front({false, 42});
        buffer.push_back({true, 1337});
      } else {
        buffer.push_back({false, 42});
        buffer.push_front({true, 1337});
      }

      assert(false);
    } catch (...) {
      assert_size_and_capacity(buffer, capacity, i + 1);
      assert_contents(buffer, std::vector(i + 1, 42ul));
    }
  }

  for (std::size_t i = 0; i < 100; ++i) {
    try {
      if (i % 2 == 0) {
        buffer.push_back({true, 1337});
      } else {
        buffer.push_front({true, 1337});
      }

      assert(false);
    } catch (...) {
      assert_size_and_capacity(buffer, capacity, 32);
      assert_contents(buffer, std::vector(32, 42ul));
    }
  }
}

} // namespace TestsByPetialetia

namespace TestsByNemoumbra {

template<typename Known, typename Unknown>
void assert_iter_size() {
  // Nemo: Tbh, it's a terrible check, but a better one would likely require static analysis
  static_assert(sizeof(typename Known::iterator) < sizeof(typename Unknown::iterator));
  static_assert(sizeof(typename Known::reverse_iterator) < sizeof(typename Unknown::reverse_iterator));
  static_assert(sizeof(typename Known::const_iterator) < sizeof(typename Unknown::const_iterator));
  static_assert(sizeof(typename Known::const_reverse_iterator) < sizeof(typename Unknown::const_reverse_iterator));
}

void test_iterators_size() {
  // If capacity is known at compiletime, there is no need to store it as field in iterator
  assert_iter_size<CircularBuffer<int, 32>, CircularBuffer<int>>();
}

template<typename Iter>
void assert_correct_tag() {
  static_assert(
      std::is_same_v
      < std::random_access_iterator_tag
      , typename std::iterator_traits<Iter>::iterator_category
      >
  );
}

template<WhenCapacityKnown When>
void test_iterator_tags() {
  using Buffer = decltype(create_buffer<int, When, 32>());

  assert_correct_tag<typename Buffer::iterator>();
  assert_correct_tag<typename Buffer::reverse_iterator>();
  assert_correct_tag<typename Buffer::const_iterator>();
  assert_correct_tag<typename Buffer::const_reverse_iterator>();
}

} // namespace TestsByNemoumbra

namespace TestsByKostylevgo {

void test_optimal_memory_usage() {
  static_assert(sizeof(CircularBuffer<int64_t, 10000>) <= 81000);
}

template<WhenCapacityKnown When>
void test_explicit_constructor() {
  // CircularBuffer(size_t) constructor should be explicit
  static_assert((!std::is_convertible_v<std::size_t, decltype(create_buffer<int, When, 5>())>));
}

struct alignas (std::max_align_t) Aligned {};

struct AlignChecker {
  CircularBuffer<Aligned, 1> buf1;
  CircularBuffer<Aligned, 1> buf2;
  int64_t change_alignment;
  CircularBuffer<Aligned, 1> buf3;
  CircularBuffer<Aligned, 1> buf4;
};

template <typename T, std::size_t Capacity>
void check_aligned(CircularBuffer<T, Capacity>& buf) {
  buf.push_back(T());
  void* ptr = buf.begin().operator->();
  void* ptr2 = ptr;
  std::size_t space = sizeof(T);
  void* align_result = std::align(alignof(T), space, ptr2, space);
  // Raw memory in CircularBuffer should be aligned for T
  assert(align_result != nullptr);
  assert(space == sizeof(T));
  assert(ptr == ptr2);
}

void test_static_buffer_alignment() {
  AlignChecker checker;
  check_aligned(checker.buf1);
  check_aligned(checker.buf2);
  check_aligned(checker.buf3);
  check_aligned(checker.buf4);
}

struct Counter {
  Counter() = default;

  Counter(const Counter&) {
    ++counter_;
  }

  Counter& operator=(const Counter&) {
    ++counter_;
    return *this;
  }

  Counter(Counter&&) = default;
  Counter& operator=(Counter&&) = default;

  static void reset() {
    counter_ = 0;
  }

  static std::size_t count() {
    return counter_;
  }

private:
  static inline std::size_t counter_ = 0;
};

template<WhenCapacityKnown When>
void test_optimal_copy_calls() {
  auto a = create_buffer<Counter, When, 5>();
  auto b = create_buffer<Counter, When, 5>();
  while (!b.full()) {
    b.push_back(Counter());
  }
  Counter::reset();
  a = b;
  assert(Counter::count() == 5);
}

/* Toy class, but it is actually possible that one of the fields of some object must be equal to
this / pointer to some inner field of that object itself. List is a good example of that. */

class SelfReferencing {
public:
  SelfReferencing(): reference_(this) {
  }

  SelfReferencing([[maybe_unused]] const SelfReferencing& other): SelfReferencing() {
  }

  SelfReferencing& operator=([[maybe_unused]] const SelfReferencing& other) {
    return *this;
  }

  bool is_linked() {
    return reference_ == this;
  }

private:
  SelfReferencing* reference_;
};

template<WhenCapacityKnown When>
void test_copy_assignment() {
  auto a = create_buffer<SelfReferencing, When, 1>();
  a.push_back(SelfReferencing());
  auto b = create_buffer<SelfReferencing, When, 1>();
  // Do not copy data bytewise, use copy assignment operator
  b = a;
  assert(b[0].is_linked());
}

} // namespace TestsByKostylevgo

namespace TestsByDolesko {

template<WhenCapacityKnown When>
void test_assignability_iterators() {
  auto buffer = create_buffer<std::size_t, When, 5>();

  static_assert(std::is_assignable_v<decltype(buffer.cbegin()), decltype(buffer.begin())>);
  static_assert(std::is_assignable_v<decltype(buffer.begin()), decltype(buffer.begin())>);
  static_assert(std::is_assignable_v<decltype(buffer.cbegin()), decltype(buffer.cbegin())>);
  static_assert(!std::is_assignable_v<decltype(buffer.begin()), decltype(buffer.cbegin())>);
}

template<WhenCapacityKnown When>
void test_convertibility_iterators() {
  auto buffer = create_buffer<std::size_t, When, 5>();

  static_assert(!std::is_convertible_v<decltype(buffer.cbegin()), decltype(buffer.begin())>);
  static_assert(std::is_convertible_v<decltype(buffer.begin()), decltype(buffer.begin())>);
  static_assert(std::is_convertible_v<decltype(buffer.cbegin()), decltype(buffer.cbegin())>);
  static_assert(std::is_convertible_v<decltype(buffer.begin()), decltype(buffer.cbegin())>);
}

template<WhenCapacityKnown When>
void test_conversion_iterators() {
  auto buffer = create_buffer<std::size_t, When, 5>();

  auto cx = buffer.cbegin();
  auto x = buffer.begin();

  assert(cx == x);

  decltype(cx) cy = x;

  assert(cx == cy);
}

} // namespace TestsByDolesko

int main() {
  TestsByPetialetia::test_empty<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_empty<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByPetialetia::test_push_and_pop<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_push_and_pop<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByPetialetia::test_indexing<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_indexing<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByPetialetia::test_at<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_at<WhenCapacityKnown::KNOWN_AT_RUNTIME>();
  
  TestsByPetialetia::test_copy_and_assignment<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_copy_and_assignment<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByPetialetia::test_constructor_exception();

  TestsByPetialetia::test_iterators<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_iterators<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByNemoumbra::test_iterator_tags<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByNemoumbra::test_iterator_tags<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByNemoumbra::test_iterators_size();

  TestsByPetialetia::test_erase<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_erase<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByPetialetia::test_insert<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_insert<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByPetialetia::test_exceptions<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByPetialetia::test_exceptions<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByKostylevgo::test_optimal_memory_usage();
  TestsByKostylevgo::test_explicit_constructor<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByKostylevgo::test_explicit_constructor<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByKostylevgo::test_static_buffer_alignment();
  TestsByKostylevgo::test_optimal_copy_calls<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByKostylevgo::test_optimal_copy_calls<WhenCapacityKnown::KNOWN_AT_RUNTIME>();
  TestsByKostylevgo::test_copy_assignment<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByKostylevgo::test_copy_assignment<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByDolesko::test_assignability_iterators<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByDolesko::test_assignability_iterators<WhenCapacityKnown::KNOWN_AT_RUNTIME>();
  TestsByDolesko::test_convertibility_iterators<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByDolesko::test_convertibility_iterators<WhenCapacityKnown::KNOWN_AT_RUNTIME>();

  TestsByDolesko::test_conversion_iterators<WhenCapacityKnown::KNOWN_AT_COMPILETIME>();
  TestsByDolesko::test_conversion_iterators<WhenCapacityKnown::KNOWN_AT_RUNTIME>();
}

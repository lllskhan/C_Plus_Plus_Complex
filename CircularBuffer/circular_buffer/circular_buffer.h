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

#include <cstddef>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <iterator>


template <size_t N>
class StackStorage {
  std::byte storage[N];
  size_t current = 0;
  
public:
  
  template <typename T>
  void* allocate(size_t n) {
    size_t space = N - current;
    void* ptr = storage + current;
    if (std::align(alignof(T), n, ptr, space)) {
      void* result = ptr;
      current = (static_cast<std::byte*>(ptr) - storage) + n;
      return result;
    }
    throw std::bad_alloc();
  }
  
  StackStorage() {}
  StackStorage(const StackStorage& other) = delete;
  StackStorage& operator=(const StackStorage& other) = delete;
  
};

template <typename T, size_t N>
class StackAllocator {
  
  StackStorage<N>* ss = nullptr;
  
public:
  
  StackStorage<N>* get_ptr() const {
    return ss;
  }
  
  using value_type = T;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  
  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) noexcept
  : ss(other.get_ptr())
  {}
  
  template <typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& other) noexcept
  {
    if (this != &other) {
      ss = other.ss;
    }
    return *this;
  }
  
  StackAllocator(StackStorage<N>& storage) noexcept
                : ss(&storage)
  {}
  
  ~StackAllocator() {}
  
  T* allocate(size_t count) {
    return static_cast<T*>(ss->template allocate<T>(count * sizeof(T)));
  }
  
  void deallocate(T*, size_t) {}
  
  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }
  
  template <typename U>
  void destroy(U* ptr) {
    ptr->~U();
  }
  
  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };
  
  StackAllocator select_on_container_copy_construction() {
    StackAllocator<T, N> copy(StackStorage<N>);
    return copy;
  }
};

template <typename T, typename U, size_t N>
bool operator==(const StackAllocator<T, N>& one, const StackAllocator<U, N>&two) {
  return one.get_ptr() == two.get_ptr();
}

template <typename T, typename U, size_t N>
bool operator!=(const StackAllocator<T, N>& one, const StackAllocator<U, N>&two) {
  return one.get_ptr() != two.get_ptr();
}

template <typename T, typename Alloc = std::allocator<T>>
class List {
  
  struct NodeBase {
          NodeBase* prev = nullptr;
          NodeBase* next = nullptr;
          NodeBase() = default;
  };

  template <typename U>
  struct Node : NodeBase {
    U value;
    Node() {}
    
    template <typename... Args>
    Node(Args&&... args) : value(std::forward<Args>(args)...) {}
  };

  NodeBase endNode;
  size_t sz = 0;
  
  using NodeAlloc = std::allocator_traits<Alloc>::template rebind_alloc<Node<T>>;
  using alc = std::allocator_traits<NodeAlloc>;
  [[no_unique_address]] NodeAlloc nodeAlloc;
  
  template <typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
  friend class UnorderedMap;
  
public:
  List() {};
  
  List(const Alloc& alloc)
  : nodeAlloc(alloc)
  {}
  
  List(size_t n, const Alloc& alloc = Alloc())
      : nodeAlloc(alloc)
  {
    try {
      for (size_t i = 0; i < n; ++i) {
        emplace_back();
      }
    } catch (...) {
      clear();
      throw;
    }
  }
  
  template <typename U = T, typename = std::enable_if_t<std::is_copy_constructible_v<U>>>
  List(size_t n, const T& value, const Alloc& alloc = Alloc()) : nodeAlloc(alloc) {
    try {
      for (size_t i = 0; i < n; ++i) {
        push_back(value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }
  
  List(const List& other)
  : List(alc::select_on_container_copy_construction(other.get_allocator()))
  {
    if constexpr (std::is_copy_constructible_v<T>) {
      if (other.sz > 0) {
        try {
          for (const auto& node : other) {
            push_back(node);
          }
        } catch (...) {
          clear();
          throw;
        }
      }
    }
  }
  
  List(const List& other, const Alloc& alloc)
  : nodeAlloc(alloc)
  {
    if constexpr (std::is_copy_constructible_v<T>) {
      if (other.sz > 0) {
        try {
          for (const auto& node : other) {
            push_back(node);
          }
        } catch (...) {
          clear();
          throw;
        }
      }
    }
  }
  
  List(List&& other) noexcept
      : endNode(other.endNode)
      , sz(other.sz)
      , nodeAlloc(std::move(other.nodeAlloc))
  {
    if (sz > 0) {
      endNode.next->prev = endNode.prev->next = &endNode;
    }
    other.endNode.prev = other.endNode.next = nullptr;
    other.sz = 0;
  }
  
  List& operator=(const List& other) {
    if (this != &other) {
      if (alc::propagate_on_container_copy_assignment::value) {
        List temp(other, other.nodeAlloc);
        swap(temp);
      } else {
        List temp(other, nodeAlloc);
        swap(temp);
      }
    }
    return *this;
  }
  
  List& operator=(List&& other) noexcept(std::allocator_traits<Alloc>::is_always_equal::value &&
                                         std::is_nothrow_move_assignable_v<NodeAlloc>)
  {
    if (this != &other) {
      List copy(std::move(*this));
      if (alc::propagate_on_container_move_assignment::value || nodeAlloc == other.nodeAlloc) {
        nodeAlloc = other.nodeAlloc;
        endNode = std::move(other.endNode);
        sz = other.sz;
        
        if (sz > 0) {
          endNode.next->prev = endNode.prev->next = &endNode;
        }
      } else {
        try {
          for (auto& node : other) {
            emplace_back(std::move(node));
          }
        } catch (...) {
          clear();
          swap(copy);
          throw;
        }
      }
      other.endNode.prev = other.endNode.next = nullptr;
      other.sz = 0;
    }
    return *this;
  }
  
  template <bool isConst>
  class iter_of_mine {
    using Ptr = NodeBase*;
    Ptr ptr;
    friend List;
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::conditional_t<isConst, const T, T>;
    using pointer = std::conditional_t<isConst, const T*, T*>;
    using reference = std::conditional_t<isConst, const T&, T&>;
    using iterator_category = std::bidirectional_iterator_tag;
     
    iter_of_mine(Ptr ptr)
                : ptr(ptr)
    {}
    
    iter_of_mine(iter_of_mine<false> other)
                : ptr(other.ptr)
    {}
    
    reference operator*() const {
      return static_cast<std::conditional_t<isConst, const Node<T>*, Node<T>*>>(ptr)->value;
    }
  
    pointer operator->() const {
      return &static_cast<std::conditional_t<isConst, const Node<T>*, Node<T>*>>(ptr)->value;
    }
    
    iter_of_mine& operator--() {
      ptr = ptr->prev;
      return *this;
    }
    
    iter_of_mine operator--(int) {
      iter_of_mine temp = ptr;
      ptr = ptr->prev;
      return temp;
    }
    
    iter_of_mine& operator++() {
      ptr = ptr->next;
      return *this;
    }
    
    iter_of_mine operator++(int) {
      iter_of_mine temp = ptr;
      ptr = ptr->next;
      return temp;
    }
    
    bool operator==(const iter_of_mine& other) const {
      return ptr == other.ptr;
    }
    
    bool operator!=(const iter_of_mine& other) const {
      return ptr != other.ptr;
    }
    
  };
  
  
  using iterator = iter_of_mine<false>;
  using const_iterator = iter_of_mine<true>;
  using reverse_iterator = std::reverse_iterator<iter_of_mine<false>>;
  using const_reverse_iterator = std::reverse_iterator<iter_of_mine<true>>;
  
  iterator begin() noexcept {
    return iterator(static_cast<Node<T>*>(endNode.next));
  }
  
  const_iterator begin() const noexcept {
    return const_iterator(static_cast<Node<T>*>(endNode.next));
  }
  
  const_iterator cbegin() const noexcept {
    return const_iterator(static_cast<Node<T>*>(endNode.next));
  }
  
  iterator end() noexcept {
    return iterator(static_cast<Node<T>*>(&endNode));
  }
  
  const_iterator end() const noexcept {
    return const_iterator(static_cast<Node<T>*>(const_cast<NodeBase*>(&endNode)));
  }
  
  const_iterator cend() const noexcept {
    return const_iterator(static_cast<Node<T>*>(&endNode));
  }
  
  reverse_iterator rbegin() noexcept {
    return reverse_iterator(static_cast<Node<T>*>(&endNode));
  }
  
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(static_cast<Node<T>*>(const_cast<NodeBase*>(&endNode)));
  }
  
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(static_cast<Node<T>*>(&endNode));
  }
  
  reverse_iterator rend() noexcept {
    return iterator(static_cast<Node<T>*>(&endNode));
  }
  
  const_reverse_iterator rend() const noexcept {
    return const_iterator(static_cast<Node<T>*>(&endNode));
  }
  
  const_reverse_iterator crend() const noexcept {
    return const_iterator(static_cast<Node<T>*>(&endNode));
  }
  
  template <typename... Args>
  iterator emplace(iterator pos, Args&&... args) {
    Node<T>* newNode = alc::allocate(nodeAlloc, 1);
    try {
      alc::construct(nodeAlloc, newNode,
                       std::forward<Args>(args)...);
    } catch (...) {
      alc::deallocate(nodeAlloc, newNode, 1);
      throw;
    }
    if (sz == 0) {
      endNode.prev = endNode.next = newNode;
      newNode->next = &endNode;
    } else {
      newNode->next = pos.ptr;
      newNode->prev = pos.ptr->prev;
      if (pos.ptr->prev) {
        pos.ptr->prev->next = newNode;
      }
      pos.ptr->prev = newNode;
                
      if (pos.ptr == endNode.next) {
        endNode.next = newNode;
      }
    }
    ++sz;
    return iterator(newNode);
  }
  
  iterator insert(iterator pos, const T& value) {
    return emplace(pos, value);
  }
      
  iterator insert(iterator pos, T&& value) {
    return emplace(pos, std::move(value));
  }
    
  iterator insert(const_iterator pos, const T& value) {
    return emplace(pos.ptr, value);
  }
        
  iterator insert(const_iterator pos, T&& value) {
    return emplace(pos.ptr, std::move(value));
  }
  
  iterator erase(const_iterator pos) {
    if (pos.ptr == endNode.next) {
      pop_front();
      return endNode.next;
    }
    pos.ptr->prev->next = pos.ptr->next;
    pos.ptr->next->prev = pos.ptr->prev;
    Node<T>* return_type = static_cast<Node<T>*>(pos.ptr->next);
    alc::destroy(nodeAlloc, pos.ptr);
    alc::deallocate(nodeAlloc, static_cast<Node<T>*>(const_cast<NodeBase*>(pos.ptr)), 1);
    --sz;
    return iterator(return_type);
  }
  
  void clear() {
    while (!this->empty()) {
      pop_back();
    }
  }
  
private:
  
  void swap(List& other) noexcept {
    std::swap(endNode.prev, other.endNode.prev);
    std::swap(endNode.next, other.endNode.next);
    if (sz > 0) {
      endNode.next->prev = &endNode;
      endNode.prev->next = &endNode;
    }
    if (other.sz > 0) {
      other.endNode.next->prev = &other.endNode;
      other.endNode.prev->next = &other.endNode;
    }
    std::swap(sz, other.sz);
    std::swap(nodeAlloc, other.nodeAlloc);
  }
  
public:
  template <typename... Args>
  void emplace_back(Args&&... args) {
    emplace(end(), std::forward<Args>(args)...);
  }
      
  template <typename... Args>
  void emplace_front(Args&&... args) {
    emplace(begin(), std::forward<Args>(args)...);
  }
      
  void push_back(const T& value) {
    emplace_back(value);
  }
      
  void push_back(T&& value) {
    emplace_back(std::move(value));
  }
      
  void push_front(const T& value) {
    emplace_front(value);
  }
      
  void push_front(T&& value) {
    emplace_front(std::move(value));
  }
  
  void pop_front() {
    if (sz == 1) {
      alc::destroy(nodeAlloc, static_cast<Node<T>*>(endNode.next));
      alc::deallocate(nodeAlloc, static_cast<Node<T>*>(endNode.next), 1);
      endNode.prev = nullptr;
    } else {
      Node<T>* first = static_cast<Node<T>*>(endNode.next->next);
      alc::destroy(nodeAlloc, static_cast<Node<T>*>(endNode.next));
      alc::deallocate(nodeAlloc, static_cast<Node<T>*>(endNode.next), 1);
      first->prev = nullptr;
      endNode.next = first;
    }
    --sz;
  }
  
  void pop_back() {
    if (sz == 1) {
      alc::destroy(nodeAlloc, static_cast<Node<T>*>(endNode.prev));
      alc::deallocate(nodeAlloc, static_cast<Node<T>*>(endNode.prev), 1);
      endNode.prev = nullptr;
    } else {
      Node<T>* last = static_cast<Node<T>*>(endNode.prev->prev);
      alc::destroy(nodeAlloc, static_cast<Node<T>*>(endNode.prev));
      alc::deallocate(nodeAlloc, static_cast<Node<T>*>(endNode.prev), 1);
      last->next = &endNode;
      endNode.prev = last;
    }
    --sz;
  }
  
  size_t size() const {
    return sz;
  }
  
  bool empty() const {
    return sz == 0;
  }
  
  Alloc get_allocator() const noexcept {
    return nodeAlloc;
  }
  
  ~List() {
    clear();
  }
};

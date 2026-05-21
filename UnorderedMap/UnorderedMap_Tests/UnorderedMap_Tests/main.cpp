#include <cstddef>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <iterator>
#include <utility>

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
      if (alc::propagate_on_container_move_assignment::value or nodeAlloc == other.nodeAlloc) {
        nodeAlloc = other.nodeAlloc;
        endNode = std::move(other.endNode);
        sz = other.sz;
        
        if (sz > 0) {
          endNode.next->prev = endNode.prev->next = &endNode;
        }
      } else {
        try {
          clear();
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
    
    NodeBase* get_node_ptr() {
      return ptr;
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


template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
  
private:
  using NodeType = std::pair<const Key, Value>;
  [[no_unique_address]] Hash hasher;
  [[no_unique_address]] Equal equalizer;
  [[no_unique_address]] Alloc NodeTypeAlloc;
  
  struct M_Node {
    NodeType kv;
    size_t hash;
    
    M_Node(Key&& key, Value&& val, size_t hash)
    : kv(std::move(key), std::move(val))
    , hash(hash)
    {}
    
    M_Node(M_Node&& node)
    : M_Node(std::move(const_cast<Key&>(node.kv.first)), std::move(node.kv.second), node.hash)
    {}
    
    M_Node(const M_Node& node)
          : kv(node.kv)
          , hash(node.hash)
    {}
    
    M_Node(NodeType&& kv, const Hash& hasher)
          : M_Node(std::move(const_cast<Key&>(kv.first)), std::move(kv.second), hasher(kv.first))
    {}
    
    M_Node(const NodeType& kv, const Hash& hasher)
          : kv(kv)
          , hash(hasher(kv.first))
    {}
    
  };
  
  using ListNodeAlloc = std::allocator_traits<Alloc>::template rebind_alloc<M_Node>;
  
  using Node = typename List<M_Node, ListNodeAlloc>::template Node<M_Node>;
  using NodeBase = typename List<M_Node, ListNodeAlloc>::NodeBase;
  
  using BucketNodeAlloc = std::allocator_traits<Alloc>::template rebind_alloc<Node*>;
  
  float MaxLoadFactor = 0.7;
  List<M_Node, ListNodeAlloc> list;
  std::vector<Node*, BucketNodeAlloc> buckets{nullptr};
  
public:
  UnorderedMap() {};
  
  UnorderedMap(const UnorderedMap& other, const Alloc& alloc)
      : hasher(other.hasher)
      , equalizer(other.equalizer)
      , NodeTypeAlloc(alloc)
      , list(alloc)
      , buckets(other.buckets.size(), nullptr, alloc)
  {
    auto other_it = other.begin();
            
    while (other_it != other.end()) {
      update();
      
      const auto& node = *other_it;
      
      size_t hash = hasher(node.first);
      size_t address = hash % buckets.size();
      
      auto inserted = list.emplace(end().ptr, node, hasher);
      
      if (buckets[address] == nullptr) {
        buckets[address] = static_cast<Node*>(inserted.get_node_ptr());
      }
      
      ++other_it;
    }
  }
  
  UnorderedMap(UnorderedMap&& other, const Alloc& alloc)
      : hasher(std::move(other.hasher))
      , equalizer(std::move(other.equalizer))
      , NodeTypeAlloc(alloc)
      , list(alloc)
      , buckets(other.buckets.size(), nullptr, alloc)
  {
    if (NodeTypeAlloc == other.NodeTypeAlloc) {
      list = std::move(other.list);
      buckets = std::move(other.buckets);
    } else {
      auto other_it = other.begin();
      
      while (other_it != other.end()) {
        update();
        
        auto& node = *other_it;
        
        size_t hash = hasher(node.first);
        size_t address = hash % buckets.size();
        
        auto inserted = list.emplace(end().ptr, std::move(node), hasher);
        
        if (buckets[address] == nullptr) {
          buckets[address] = static_cast<Node*>(inserted.get_node_ptr());
        }
        
        ++other_it;
      }
      other.clear();
    }
  }
  
  UnorderedMap(const UnorderedMap& other)
      : UnorderedMap(other, std::allocator_traits<Alloc>::
            select_on_container_copy_construction(other.NodeTypeAlloc)) {}
  
  UnorderedMap(UnorderedMap&& other) noexcept
              : hasher(std::move(other.hasher))
              , equalizer(std::move(other.equalizer))
              , NodeTypeAlloc(std::move(other.NodeTypeAlloc))
              , list(std::move(other.list))
              , buckets(std::move(other.buckets))
  {}
  
  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this != &other) {
      UnorderedMap temp(other,
                      std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value ?
                      other.NodeTypeAlloc : NodeTypeAlloc);
      private_swap(temp);
    }
    return *this;
  }
  
  UnorderedMap& operator=(UnorderedMap&& other)
                  noexcept(
                           std::allocator_traits<Alloc>::is_always_equal::value
                           && std::is_nothrow_move_assignable_v<Hash>
                           && std::is_nothrow_move_assignable_v<Equal>)
  {
    if (this != &other) {
      hasher = std::move(other.hasher);
      equalizer = std::move(other.equalizer);
      if (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value or NodeTypeAlloc == other.NodeTypeAlloc) {
        list = std::move(other.list);
        buckets = std::move(other.buckets);
        NodeTypeAlloc = other.NodeTypeAlloc;
      } else {
        UnorderedMap temp(std::move(other), NodeTypeAlloc);
        private_swap(temp);
      }
    }
    return *this;
  }
  
  ~UnorderedMap() {
    clear();
  }
  
  void clear() {
    list.clear();
    buckets.assign(buckets.size(), nullptr);
  }
  
  template <bool isConst>
  class iter_of_map {
    using Ptr = NodeBase*;
    Ptr ptr;
    friend UnorderedMap;
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::conditional_t<isConst, const NodeType, NodeType>;
    using pointer = std::conditional_t<isConst, const NodeType*, NodeType*>;
    using reference = std::conditional_t<isConst, const NodeType&, NodeType&>;
    using iterator_category = std::forward_iterator_tag;
    
    iter_of_map(Ptr ptr)
                : ptr(ptr)
    {}
    
    iter_of_map(iter_of_map<false> other)
                : ptr(other.ptr)
    {}
    
    reference operator*() const {
      return static_cast<Node*>(ptr)->value.kv;
    }
  
    pointer operator->() const {
      return &(static_cast<Node*>(ptr)->value.kv);
    }
    
    iter_of_map& operator++() {
      ptr = ptr->next;
      return *this;
    }
    
    iter_of_map operator++(int) {
      iter_of_map temp = ptr;
      ptr = ptr->next;
      return temp;
    }
    
    bool operator==(const iter_of_map& other) const {
      return ptr == other.ptr;
    }
    
    bool operator!=(const iter_of_map& other) const {
      return ptr != other.ptr;
    }
    
  };
  
  using iterator = iter_of_map<false>;
  using const_iterator = iter_of_map<true>;
  
  iterator begin() noexcept {
    return iterator(static_cast<Node*>(list.endNode.next));
  }
  
  const_iterator begin() const noexcept {
    return const_iterator(static_cast<Node*>(list.endNode.next));
  }
  
  const_iterator cbegin() const noexcept {
    return const_iterator(static_cast<Node*>(list.endNode.next));
  }
  
  iterator end() noexcept {
    return iterator(static_cast<Node*>(&list.endNode));
  }
  
  const_iterator end() const noexcept {
    return const_iterator(static_cast<Node*>(const_cast<NodeBase*>(&list.endNode)));
  }
  
  const_iterator cend() const noexcept {
    return const_iterator(static_cast<Node*>(const_cast<NodeBase*>(&list.endNode)));
  }
  
private:
  
  void private_swap(UnorderedMap& other)
  noexcept
  {
    std::swap(NodeTypeAlloc, other.NodeTypeAlloc);
    std::swap(hasher, other.hasher);
    std::swap(equalizer, other.equalizer);
    std::swap(list, other.list);
    std::swap(buckets, other.buckets);
  }
  
  std::pair<iterator, bool> find_of_mine(const Key& key) {
    size_t hash = hasher(key);
    
    if (buckets[hash % buckets.size()] != nullptr) {
      
      Node* elem = buckets[hash % buckets.size()];
      while (elem != static_cast<Node*>(&list.endNode) and elem->value.hash % buckets.size() == hash % buckets.size()) {
        
        if (equalizer(elem->value.kv.first, key)) {
          return {elem, true};
        }
        
        elem = static_cast<Node*>(elem->next);
      }
      return {elem, false};
    }
    return {end(), false};
  }
 
  void update() {
    if (load_factor() > MaxLoadFactor) {
      rehash(std::ceil(list.size() / MaxLoadFactor));
    }
  }
  
  template <typename Arg>
  std::pair<iterator, bool> private_insert(Arg&& node) {
    update();
    
    auto [iter, found] = find_of_mine(node.first);
    if (found) {
      return {iter, false};
    }
    
    size_t hash = hasher(node.first);
    size_t address = hash % buckets.size();
    
    auto inserted = list.emplace(iter.ptr, std::forward<Arg>(node), hasher);
    
    if (buckets[address] == nullptr) {
      buckets[address] = static_cast<Node*>(inserted.get_node_ptr());
    }
    
    return {inserted.get_node_ptr(), true};
  }
  
public:
  
  std::pair<iterator, bool> insert(const NodeType& node) {
    return private_insert(node);
  }
  
  std::pair<iterator, bool> insert(NodeType&& node) {
    return private_insert(std::move(node));
  }
  
  template <class InputIt>
  void insert(InputIt first, InputIt second) {
    while (first != second) {
      insert(*first);
      ++first;
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    update();
    
    NodeType* node = NodeTypeAlloc.allocate(1);
    try {
      std::allocator_traits<Alloc>::construct(
          NodeTypeAlloc,
          node,
          std::forward<Args>(args)...
          );
    } catch (...) {
      NodeTypeAlloc.deallocate(node, 1);
      throw;
    }

    auto [iter, found] = find_of_mine(node->first);
    if (found) {
      std::allocator_traits<Alloc>::destroy(NodeTypeAlloc, node);
      std::allocator_traits<Alloc>::deallocate(NodeTypeAlloc, node, 1);
      return {iter, false};
    }

    size_t address = hasher(node->first) % buckets.size();
    
    auto inserted = list.emplace(
      iter.ptr,
      std::move(*node),
      hasher
    );

    std::allocator_traits<Alloc>::destroy(NodeTypeAlloc, node);
    std::allocator_traits<Alloc>::deallocate(NodeTypeAlloc, node, 1);
      
    if (buckets[address] == nullptr) {
      buckets[address] = static_cast<Node*>(inserted.get_node_ptr());
    }

    return {inserted.get_node_ptr(), true};
  }
  
  iterator erase(const_iterator pos) {
    if (pos == cend()) {
      return end();
    }
    
    Node* node = static_cast<Node*>(pos.ptr);
    size_t address = node->value.hash % buckets.size();
    
    Node* next_node = static_cast<Node*>(node->next);
    if (static_cast<NodeBase*>(next_node) == &list.endNode or
        (node->value.hash - next_node->value.hash) % buckets.size() != 0) {
      buckets[address] = nullptr;
    } else {
      buckets[address] = next_node;
    }
    
    iterator following(pos.ptr);
    ++following;
    list.erase(pos.ptr);
    
    return following;
  }
  
  iterator erase(const_iterator first, const_iterator last) {
    while (first != last) {
      first = erase(first);
    }
    return iterator(last.ptr);
  }
  
  iterator find(const Key& key) {
    auto [iter, b] = find_of_mine(key);
    if (b) {
      return iter;
    }
    return end();
  }
  
  Value& operator[](const Key& key) {
    auto [iter, found] = find_of_mine(key);
    if (found) {
      return (*iter).second;
    }
    return insert({key, Value()}).first->second;
  }
  
  Value& operator[](Key&& key) {
    auto [iter, found] = find_of_mine(key);
    if (found) {
      return (*iter).second;
    }
    return insert({std::move(key), Value()}).first->second;
  }
  
  Value& at(const Key& key ) {
    auto [iter, found] = find_of_mine(key);
    if (found) {
      return (*iter).second;
    }
    throw std::out_of_range("Memory is out of your possession");
  }
  
  const Value& at(const Key& key) const {
    auto [iter, found] = find_of_mine(key);
    if (found) {
      return (*iter).second;
    }
    throw std::out_of_range("Memory is out of your possession");
  }
  
  size_t size() const {
    return list.size();
  }
  
  float load_factor() const {
    return static_cast<float>(list.size()) / buckets.size();
  }
  
  float max_load_factor() const {
    return MaxLoadFactor;
  }
  
  void max_load_factor(float value) {
    MaxLoadFactor = value;
  }
  
  void rehash(size_t count) {
    size_t n = std::ceil(count / MaxLoadFactor);
    buckets.assign(n, nullptr);
    
    if (!list.empty()) {
      List<M_Node, ListNodeAlloc> new_list;
      auto elem = list.begin();
      while (elem != list.end()) {
        auto node = static_cast<Node*>(elem.get_node_ptr());
        size_t address = node->value.hash % n;
        
        if (buckets[address] == nullptr) {
          new_list.push_back(std::move(node->value));
          buckets[address] = static_cast<Node*>(new_list.endNode.prev);
        } else {
          new_list.insert(buckets[address]->next, std::move(node->value));
        }
        
        ++elem;
      }
      list = std::move(new_list);
    }
  }
  
  void reserve(size_t count) {
    rehash(std::ceil(count / MaxLoadFactor));
  }
  
  void swap(UnorderedMap& other)
  noexcept(std::allocator_traits<Alloc>::is_always_equal::value && std::is_nothrow_swappable_v<Hash> &&
      std::is_nothrow_swappable_v<Equal>)
  {
    if (std::allocator_traits<Alloc>::propagate_on_container_swap::value) {
      std::swap(NodeTypeAlloc, other.NodeTypeAlloc);
    }
    std::swap(hasher, other.hasher);
    std::swap(equalizer, other.equalizer);
    std::swap(list, other.list);
    std::swap(buckets, other.buckets);
  }
  
  Alloc get_allocator() const noexcept {
    return list.nodeAlloc;
  }
};



//#include "unordered_map.h"
//#include <unordered_map>

#include <vector>
#include <string>
#include <iterator>
#include <cassert>

#include <iostream>

/*template <typename Key, typename Value, typename Hash = std::hash<Key>,
         typename EqualTo = std::equal_to<Key>,
         typename Alloc = std::allocator<std::pair<const Key, Value>> >
using UnorderedMap = std::unordered_map<Key, Value, Hash, EqualTo, Alloc>;
*/

void SimpleTest() {
    // std::cerr << "starting simple test" << std::endl;
    UnorderedMap<std::string, int> m;

    m["aaaaa"] = 5;
    m["bbb"] = 6;
    // std::cerr << "two first assignments with [] passed" << std::endl;
    m.at("bbb") = 7;
    // std::cerr << "assignment with at() passed" << std::endl;
    assert(m.size() == 2);
    
    assert(m["aaaaa"] == 5);
    assert(m["bbb"] == 7);
    assert(m["ccc"] == 0);

    assert(m.size() == 3);
   
    // std::cerr << "assertions passed; before try block now" << std::endl;

    try {
        m.at("xxxxxxxx");
        assert(false);
    } catch (...) {
        // std::cerr << "in catch block" << std::endl;
    }
    
    auto it = m.find("dddd");
    assert(it == m.end());
    // std::cerr << "finding dddd passed, it == m.end()" << std::endl;

    it = m.find("bbb");
    assert(it->second == 7);
    // std::cerr << "finding bbb passed, it->second == 7" << std::endl;
    ++it->second;
    assert(it->second == 8);
    // std::cerr << "incrementing it->second passed, it->second==8" << std::endl;

    for (auto& item : m) {
        --item.second;
    }
    assert(m.at("aaaaa") == 4);
    // std::cerr << "decrementing each value passed, value at aaaaa == 4" << std::endl;

    {
        auto mm = m;
        // std::cerr << "m copied into mm" << std::endl;
        m = std::move(mm);
        // std::cerr << "mm moved into m" << std::endl;
    }
    // std::cerr << "copying and moving passed" << std::endl;
    
    auto res = m.emplace("abcde", 2);
    assert(res.second);
    // std::cerr << "emplacing abcde passed, value at abcde == 2" << std::endl;
}

void TestIterators() {
    UnorderedMap<double, std::string> m;

    std::vector<double> keys = {0.4, 0.3, -8.32, 7.5, 10.0, 0.0};
    std::vector<std::string> values = {
        "Summer has come and passed",
        "The innocent can never last",
        "Wake me up when September ends",
        "Like my fathers come to pass",
        "Seven years has gone so fast",
        "Wake me up when September ends",
    };

    m.reserve(1'000'000);

    for (int i = 0; i < 6; ++i) {
        m.insert({keys[i], values[i]});
    }

    auto beg = m.cbegin();
    std::string s = beg->second;
    auto it = m.begin();
    ++it;
    m.erase(it++);
    it = m.begin();
    m.erase(++it);

    assert(beg->second == s);
    assert(m.size() == 4);
 
    UnorderedMap<double, std::string> mm;
    std::vector<std::pair<const double, std::string>> elements = {
        {3.0, values[0]},
        {5.0, values[1]},
        {-10.0, values[2]},
        {35.7, values[3]}
    };
    mm.insert(elements.cbegin(), elements.cend());
    s = mm.begin()->second;

    m.insert(mm.begin(), mm.end());
    assert(mm.size() == 4);
    assert(mm.begin()->second == s);


    // Test traverse efficiency
    m.reserve(1'000'000); // once again, nothing really should happen
    assert(m.size() == 8);
    // Actions below must be quick (~ 1000 * 8 operations) despite reserving space for 1M elements
    for (int i = 0; i < 10000; ++i) {
        long long h = 0;
        for (auto it = m.cbegin(); it != m.cend(); ++it) {
            // just some senseless action
            h += int(it->first) + int((it->second)[0]);
        }
        std::ignore = h;
    }

    it = m.begin();
    ++it;
    s = it->second;
    // I asked to reserve space for 1M elements so actions below adding 100'000 elements mustn't cause reallocation
    for (double d = 100.0; d < 10100.0; d += 0.1) {
        m.emplace(d, "a");
    }
    // And my iterator must point to the same object as before
    assert(it->second == s);

    auto dist = std::distance(it, m.end());
    auto sz = m.size();
    m.erase(it, m.end());
    assert(sz - dist == m.size());

    // Must be also fast
    for (double d = 200.0; d < 10200.0; d += 0.35) {
        auto it = m.find(d);
        if (it != m.end()) m.erase(it);
    }
}

// Just a simple SFINAE trick to check CE presence when it's necessary
// Stay tuned, we'll discuss this kind of tricks in our next lectures ;)
template<typename T>
decltype(UnorderedMap<T, T>().cbegin()->second = 0, int()) TestConstIteratorDoesntAllowModification(T) {
    assert(false);
}
template<typename... FakeArgs>
void TestConstIteratorDoesntAllowModification(FakeArgs...) {}


struct VerySpecialType {
    int x = 0;
    explicit VerySpecialType(int x): x(x) {}
    VerySpecialType(const VerySpecialType&) = delete;
    VerySpecialType& operator=(const VerySpecialType&) = delete;

    VerySpecialType(VerySpecialType&&) = default;
    VerySpecialType& operator=(VerySpecialType&&) = default;
};

struct NeitherDefaultNorCopyConstructible {
    VerySpecialType x;

    NeitherDefaultNorCopyConstructible() = delete;
    NeitherDefaultNorCopyConstructible(const NeitherDefaultNorCopyConstructible&) = delete;
    NeitherDefaultNorCopyConstructible& operator=(const NeitherDefaultNorCopyConstructible&) = delete;

    NeitherDefaultNorCopyConstructible(VerySpecialType&& x): x(std::move(x)) {}
    NeitherDefaultNorCopyConstructible(NeitherDefaultNorCopyConstructible&&) = default;
    NeitherDefaultNorCopyConstructible& operator=(NeitherDefaultNorCopyConstructible&&) = default;

    bool operator==(const NeitherDefaultNorCopyConstructible& other) const {
        return x.x == other.x.x;
    }
};

namespace std {
    template<>
    struct hash<NeitherDefaultNorCopyConstructible> {
        size_t operator()(const NeitherDefaultNorCopyConstructible& x) const {
            return std::hash<int>()(x.x.x);
        }
    };
}

void TestNoRedundantCopies() {
// std::cerr << "Test no redundant copies started" << std::endl;
    UnorderedMap<NeitherDefaultNorCopyConstructible, NeitherDefaultNorCopyConstructible> m;
// std::cerr << "m created" << std::endl;
    m.reserve(10);
// std::cerr << "m.reserve(10) done" << std::endl;
    m.emplace(VerySpecialType(0), VerySpecialType(0));
// std::cerr << "m.emplace(VerySpecialType(0), VerySpecialType(0)) done" << std::endl;
    m.reserve(1'000'000);
// std::cerr << "m.reserve(1000000) done" << std::endl;
    std::pair<NeitherDefaultNorCopyConstructible, NeitherDefaultNorCopyConstructible> p{VerySpecialType(1), VerySpecialType(1)};

    m.insert(std::move(p));
// std::cerr << "m.insert(std::move(p)) done" << std::endl;

    assert(m.size() == 2);

    // this shouldn't compile
    // m[VerySpecialType(0)] = VerySpecialType(1);
    
    // but this should
    m.at(VerySpecialType(1)) = VerySpecialType(0);
// std::cerr << "m.at(VerySpecialType(1)) = VerySpecialType(0) done" << std::endl;

    {
        auto mm = std::move(m);
// std::cerr << "m moved to mm" << std::endl;
        m = std::move(mm);
// std::cerr << "mm moved to m" << std::endl;
    }
// std::cerr << "the scope of mm has finished" << std::endl;
    m.erase(m.begin());
// std::cerr << "m.erase(m.begin()) done" << std::endl;
    m.erase(m.begin());
// std::cerr << "m.erase(m.begin()) done once again" << std::endl;
    assert(m.size() == 0);
}


template<typename T>
struct MyHash {
    size_t operator()(const T& p) const {
        return std::hash<int>()(p.second / p.first);
    }
};

template<typename T>
struct MyEqual {
    bool operator()(const T& x, const T& y) const {
        return y.second / y.first == x.second / x.first;
    }
};

struct OneMoreStrangeStruct {
    int first;
    int second;
};

bool operator==(const OneMoreStrangeStruct&, const OneMoreStrangeStruct&) = delete;


void TestCustomHashAndCompare() {
    /*UnorderedMap<std::pair<int, int>, char, MyHash<std::pair<int, int>>,
            MyEqual<std::pair<int, int>>> m;

    m.insert({{1, 2}, 0});
    m.insert({{2, 4}, 1});
    assert(m.size() == 1);

    m[{3, 6}] = 3;
    assert(m.at({4, 8}) == 3);*/

    UnorderedMap<OneMoreStrangeStruct, int, MyHash<OneMoreStrangeStruct>, MyEqual<OneMoreStrangeStruct>> mm;
    mm[{1, 2}] = 3;
    assert(mm.at({5, 10}) == 3);

    mm.emplace(OneMoreStrangeStruct{3, 9}, 2);
    assert(mm.size() == 2);
    mm.reserve(1'000);
    mm.erase(mm.begin());
    mm.erase(mm.begin());
    for (int k = 1; k < 100; ++k) {
        for (int i = 1; i < 10; ++i) {
            mm.insert({{i, k*i}, 0});
        }
    }
    std::string ans;
    std::string myans;
    for (auto it = mm.cbegin(); it != mm.cend(); ++it) {
        ans += std::to_string(it->second);
        myans += '0';
    }
    assert(ans == myans);
}


// Finally, some tricky fixtures to test custom allocator.
// Done by professional, don't try to repeat
class Chaste {
private:
    int x = 0;
    Chaste() = default;
    Chaste(int x): x(x) {}

    // Nobody can construct me except this guy
    template<typename T>
    friend struct TheChosenOne;

public:
    Chaste(const Chaste&) = default;
    Chaste(Chaste&&) = default;

    bool operator==(const Chaste& other) const {
        return x == other.x;
    }
};

namespace std {
    template<>
    struct hash<Chaste> {
        size_t operator()(const Chaste& x) const noexcept {
            return std::hash<int>()(reinterpret_cast<const int&>(x));
        }
    };
}

template<typename T>
struct TheChosenOne: public std::allocator<T> {
    TheChosenOne() {}

    template<typename U>
    TheChosenOne(const TheChosenOne<U>&) {}

    template<typename... Args>
    void construct(T* p, Args&&... args) const {
        new(p) T(std::forward<Args>(args)...);
    }

    void construct(std::pair<const Chaste, Chaste>* p, int a, int b) const {
        new(p) std::pair<const Chaste, Chaste>(Chaste(a), Chaste(b));
    }

    void destroy(T* p) const {
        p->~T();
    }

    template<typename U>
    struct rebind {
        using other = TheChosenOne<U>;
    };
};

/*
template<>
struct TheChosenOne<std::pair<const Chaste, Chaste>>
        : public std::allocator<std::pair<const Chaste, Chaste>> {
    using PairType = std::pair<const Chaste, Chaste>;

    TheChosenOne() {}

    template<typename U>
    TheChosenOne(const TheChosenOne<U>&) {}

    void construct(PairType* p, int a, int b) const {
        new(p) PairType(Chaste(a), Chaste(b));
    }

    void destroy(PairType* p) const {
        p->~PairType();
    }

    template<typename U>
    struct rebind {
        using other = TheChosenOne<U>;
    };
};
*/

void TestCustomAlloc() {
    // This container mustn't construct or destroy any objects without using TheChosenOne allocator
    UnorderedMap<Chaste, Chaste, std::hash<Chaste>, std::equal_to<Chaste>,
        TheChosenOne<std::pair<const Chaste, Chaste>>> m;
    
    m.emplace(0, 0);
    
    {
        auto mm = m;
        mm.reserve(1'000);
        mm.erase(mm.begin());
    }

    for (int i = 0; i < 1'000'000; ++i) {
        m.emplace(i, i);
    }

    for (int i = 0; i < 500'000; ++i) {
        auto it = m.begin();
        ++it, ++it;
        m.erase(m.begin(), it);
    }
}

int main() {
    std::cerr << "Starting tests" << std::endl;
    SimpleTest();
    std::cerr << "SimpleTest (1 of 6) passed" << std::endl;
    TestIterators();
    std::cerr << "TestIterators (2 of 6) passed" << std::endl;
    TestConstIteratorDoesntAllowModification(0);
    std::cerr << "TestConstIteratorDoesntAllowModification (3 of 6) passed" << std::endl;
    TestNoRedundantCopies();
    std::cerr << "TestRedundantCopies (4 of 6) passed" << std::endl;
    TestCustomHashAndCompare();
    std::cerr << "TestCustomHashAndCompare (5 of 6) passed" << std::endl;
    TestCustomAlloc();
    std::cerr << "TestCustomAlloc (6 of 6) passed" << std::endl;
    std::cout << 0;
}

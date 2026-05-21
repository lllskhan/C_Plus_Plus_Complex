#include <memory>

struct BaseControlBlock {
  size_t shared_counter{0};
  size_t weak_counter{0};
  virtual void destroy() = 0;
  virtual void deallocate() = 0;
  virtual ~BaseControlBlock() = default;
};

template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
struct ControlBlockStandard : BaseControlBlock {
  U* object;
  [[no_unique_address]] Deleter del;
  [[no_unique_address]] Alloc alloc;
  
  ControlBlockStandard(U* object, Deleter del = Deleter(), Alloc alloc = Alloc()) : object(object), del(del), alloc(alloc) {}
  
  void destroy() {
    del(object);
  }
  
  void deallocate() {
    del.~Deleter();
    using ControlBlockDealloc = std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockStandard>;
    ControlBlockDealloc dealloc = alloc;
    alloc.~Alloc();
    std::allocator_traits<ControlBlockDealloc>::deallocate(dealloc, this, 1);
  }
};

template <typename U, typename Alloc = std::allocator<U>>
struct ControlBlockMakeShared : BaseControlBlock {
  U object;
  [[no_unique_address]] Alloc alloc;
  
  template <typename... Args>
  ControlBlockMakeShared(Alloc alloc = Alloc(), Args&&... args)
                        : object(std::forward<Args>(args)...)
                        , alloc(alloc)
  {}
  
  
  void destroy() {
    std::allocator_traits<Alloc>::destroy(alloc, &object);
  }
  
  void deallocate() {
    using ControlBlockDealloc = std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared>;
    ControlBlockDealloc dealloc = alloc;
    alloc.~Alloc();
    std::allocator_traits<ControlBlockDealloc>::deallocate(dealloc, this, 1);
  }
};


template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis;

template <typename T>
class SharedPtr {
private:
  
  T* ptr{nullptr};
  BaseControlBlock* cb{nullptr};
  
  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&... args);
  
  template <typename >
  friend class SharedPtr;
  
  template <typename >
  friend class WeakPtr;
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  SharedPtr(const WeakPtr<U>& other) : ptr(other.ptr), cb(other.cb) {
    if (cb != nullptr) {
      ++(cb->shared_counter);
    }
  }
  
  template <typename U, typename Alloc = std::allocator<U>>
  SharedPtr(ControlBlockMakeShared<U, Alloc>* pointer) : ptr(&(pointer->object)), cb(pointer) {
    ++(cb->shared_counter);
    if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
      if (ptr) {
        ptr->assign_shared_ptr(*this);
      }
    }
  }
  
public:
  constexpr SharedPtr() noexcept {}
  
  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  SharedPtr(U* ptr, Deleter del = Deleter(), Alloc alloc = Alloc()) : ptr(ptr) {
    using ControlBlockAlloc = std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockStandard<U, Deleter, Alloc>>;
    ControlBlockAlloc block_alloc = alloc;
    auto* block = std::allocator_traits<ControlBlockAlloc>::allocate(block_alloc, 1);
    
    try {
        ::new (static_cast<void*>(block)) ControlBlockStandard<U, Deleter, Alloc>(ptr, del, alloc);
      } catch (...) {
        std::allocator_traits<ControlBlockAlloc>::deallocate(block_alloc, block, 1);
        throw;
      }
    
    cb = block;
    ++(cb->shared_counter);
    
    if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
      if (ptr) {
        ptr->assign_shared_ptr(*this);
      }
    }
  }
  
  SharedPtr(const SharedPtr& other) noexcept : ptr(other.ptr), cb(other.cb) {
    if (cb != nullptr) {
      ++(cb->shared_counter);
    }
  }
  
  SharedPtr(SharedPtr&& other) noexcept : ptr(other.ptr), cb(other.cb)
  {
    other.ptr = nullptr;
    other.cb = nullptr;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  SharedPtr(const SharedPtr<U>& other) noexcept
           : ptr(other.ptr)
           , cb(const_cast<BaseControlBlock*>(other.cb))
  {
    if (cb != nullptr) {
      ++(cb->shared_counter);
    }
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  SharedPtr(SharedPtr<U>&& other) noexcept
           : ptr(other.ptr)
           , cb(other.cb)
  {
    other.ptr = nullptr;
    other.cb = nullptr;
  }
  
  SharedPtr& operator=(const SharedPtr& other) noexcept {
    SharedPtr<T>(other).swap(*this);
    return *this;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  SharedPtr& operator=(const SharedPtr<U>& other) noexcept {
    SharedPtr<T>(other).swap(*this);
    return *this;
  }
  
  SharedPtr& operator=(SharedPtr&& other) noexcept {
    SharedPtr<T>(std::move(other)).swap(*this);
    return *this;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
    SharedPtr<T>(std::move(other)).swap(*this);
    return *this;
  }
  
  void swap(SharedPtr& other) noexcept {
    std::swap(ptr, other.ptr);
    std::swap(cb, other.cb);
  }
  
  long use_count() const noexcept {
    return (cb != nullptr ? cb->shared_counter : 0);
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  void reset(U* ptr) {
    SharedPtr<T>(ptr).swap(*this);
  }
  
  T& operator*() const noexcept {
    return *ptr;
  }
  
  T* operator->() const noexcept {
    return ptr;
  }
  
  void reset() noexcept {
    SharedPtr().swap(*this);
  }
  
  T* get() const noexcept {
    return ptr;
  }
  
  ~SharedPtr() noexcept {
    if (cb != nullptr) {
      --(cb->shared_counter);
      if (cb->weak_counter == 0) {
        if (cb->shared_counter == 0) {
          cb->destroy();
          cb->deallocate();
        }
      } else {
        if (cb->shared_counter == 0) {
          cb->destroy();
        }
      }
    }
  }
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using ControlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
  ControlBlockAlloc block_alloc = alloc;
  
  auto* block = std::allocator_traits<ControlBlockAlloc>::allocate(block_alloc, 1);
  try {
    std::allocator_traits<ControlBlockAlloc>::construct(block_alloc, block, alloc,  std::forward<Args>(args)...);
  } catch (...) {
    std::allocator_traits<ControlBlockAlloc>::deallocate(block_alloc, block, 1);
    throw;
  }
  return SharedPtr<T>(block);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}



template <typename T>
class EnableSharedFromThis {
private:
  mutable WeakPtr<T> weak_this;
  
  template <typename U>
  friend class SharedPtr;
  
protected:
  void assign_shared_ptr(const SharedPtr<T>& sp) const {
    weak_this = sp;
  }
  
public:
  SharedPtr<T> shared_from_this() {
    return weak_this.lock();
  }
    
  SharedPtr<const T> shared_from_this() const {
    return weak_this.lock();
  }
    
  virtual ~EnableSharedFromThis() = default;
};


template <typename T>
class WeakPtr {
private:
  template <typename >
  friend class SharedPtr;
  
  template <typename >
  friend class WeakPtr;
  
  template <typename >
  friend class EnableSharedFromThis;
  
  T* ptr{nullptr};
  BaseControlBlock* cb{nullptr};
  
public:
  constexpr WeakPtr() {};
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  WeakPtr(const SharedPtr<U>& other) noexcept
         : ptr(other.ptr), cb(other.cb)
  {
    if (cb != nullptr) {
      ++(cb->weak_counter);
    }
  }
  
  WeakPtr(const WeakPtr& other) noexcept : ptr(other.ptr), cb(other.cb) {
    if (cb != nullptr) {
      ++(cb->weak_counter);
    }
  }
  
  WeakPtr(WeakPtr&& other) noexcept : ptr(other.ptr), cb(other.cb)
  {
    other.ptr = nullptr;
    other.cb = nullptr;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  WeakPtr(const WeakPtr<U>& other) noexcept
           : ptr(other.ptr)
           , cb(other.cb)
  {
    if (cb != nullptr) {
      ++(cb->weak_counter);
    }
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  WeakPtr(WeakPtr<U>&& other) noexcept
           : ptr(other.ptr)
           , cb(other.cb)
  {
    other.ptr = nullptr;
    other.cb = nullptr;
  }
  
  WeakPtr& operator=(const WeakPtr& other) noexcept {
    WeakPtr<T>(other).swap(*this);
    return *this;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  WeakPtr& operator=(const WeakPtr<U>& other) noexcept {
    WeakPtr<T>(other).swap(*this);
    return *this;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  WeakPtr& operator=(const SharedPtr<U>& other) noexcept {
    WeakPtr<T>(other).swap(*this);
    return *this;
  }
  
  WeakPtr& operator=(WeakPtr&& other) noexcept {
    WeakPtr<T>(std::move(other)).swap(*this);
    return *this;
  }
  
  template <typename U>
  requires (std::is_base_of_v<T, U> or std::is_same_v<T, U>)
  WeakPtr& operator=(WeakPtr<U>&& other) noexcept {
    WeakPtr<T>(std::move(other)).swap(*this);
    return *this;
  }
  
  void swap(WeakPtr& other) noexcept {
    std::swap(ptr, other.ptr);
    std::swap(cb, other.cb);
  }
  
  bool expired() const noexcept {
    return (cb != nullptr ? cb->shared_counter == 0 : true);
  }
  
  SharedPtr<T> lock() const noexcept {
    return (expired() ? SharedPtr<T>() : SharedPtr<T>(*this));
  }
  
  long use_count() const noexcept {
    return (cb != nullptr ? cb->shared_counter : 0);
  }
  
  ~WeakPtr() noexcept {
    if (cb != nullptr) {
      --(cb->weak_counter);
      if (cb->weak_counter == 0 and cb->shared_counter == 0) {
        cb->deallocate();
      }
    }
  }
};

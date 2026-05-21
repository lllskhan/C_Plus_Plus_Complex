#include <algorithm>
#include <cstring>
#include <cctype>
#include <iostream>

class String {
private:
  char* str_ = nullptr;
  size_t amount_ = 0;
  size_t cap_ = 0;
  
  String(size_t count)
        : str_(new char[count + 1])
        , amount_(count)
        , cap_(count)
  {
    str_[amount_] = '\0';
  }
  
  size_t consist_f(const String& one) const {
    for (size_t i = 0; i < amount_ - one.amount_ + 1; ++i) {
      if (memcmp(one.str_, str_ + i, one.amount_) == 0) {
        return i;
      }
        }
    return amount_;
  }
  
  size_t consist_r(const String& one) const {
    for (size_t i = amount_ - 1; i >= one.amount_ - 1; --i) {
      if (memcmp(one.str_, str_ + i - one.amount_ + 1, one.amount_) == 0) {
        return i - one.amount_ + 1;
      }
    }
    return amount_;
  }
  
public:
  String(size_t count, char i)
        : String(count)
  {
    std::fill(str_, str_ + amount_, i);
  }
  
  String(){}
  
  String(const String& one)
        : String(one.amount_)
  {
    std::copy(one.str_, one.str_ + one.amount_, str_);
  }
  
  String(const char* i)
        : String(strlen(i))
  {
    std::copy(i, i + strlen(i), str_);
  }
  
  String& operator=(const String& one)
  {
    if (&one != this) {
      if (cap_ >= one.cap_) {
        std::copy(one.str_, one.str_ + one.amount_, str_);
      } else {
        char* change = new char[one.amount_ + 1];
        std::copy(one.str_, one.str_ + one.amount_, change);
        delete[] str_;
        str_ = change;
      }
      amount_ = one.amount_;
      str_[amount_] = '\0';
    }
    return *this;
  }
  
  String& operator+=(const String& one) {
    if (cap_ < amount_ + one.amount_) {
      cap_ = (amount_ + one.amount_) * 2;
      char* change = new char[cap_ + 1];
      std::copy(str_, str_ + amount_, change);
      delete[] str_;
      str_ = change;
    }
    std::copy(one.str_, one.str_ + one.amount_, str_ + amount_);
    str_[amount_ + one.amount_] = '\0';
    amount_ += one.amount_;
    return *this;
  }
  
  String& operator+=(char i) {
    if (cap_ < amount_ + 1) {
      cap_ = (amount_ + 1) * 2;
      char* change = new char[cap_ + 1];
      std::copy(str_, str_ + amount_, change);
      delete[] str_;
      str_ = change;
    }
    amount_ += 1;
    str_[amount_ - 1] = i;
    str_[amount_] = '\0';
    return *this;
  }
  
  const size_t& length() const {
    return amount_;
  }
  
  const size_t& size() const {
    return amount_;
  }
  
  const size_t& capacity() const {
    return cap_;
  }
  
  void push_back(char i) {
    *this += i;
  }
  
  void pop_back() {
    if (amount_ >= 1) {
      amount_ -= 1;
      str_[amount_] = '\0';
    }
  }
  
  const char& front() const {
    return str_[0];
  }
  
  const char& back() const {
    return str_[amount_ - 1];
  }
  
  char& front() {
    return str_[0];
  }
  
  char& back() {
    return str_[amount_ - 1];
  }
  
  const char& operator[](size_t ind) const {
    return str_[ind];
  }
  
  char& operator[](size_t ind) {
    return str_[ind];
  }
  
  size_t find(const String& a) const {
    if (amount_ >= a.amount_) {
      return consist_f(a);
    }
    return amount_;
  }
  
  size_t rfind(const String& a) const {
    if (amount_ >= a.amount_) {
      return consist_r(a);
    }
    return amount_;
  }
  
  String substr(size_t start, size_t count) const {
    String a(count);
    std::copy(str_ + start, str_ + start + count, a.str_);
    a[count] = '\0';
    return a;
  }
  
  bool empty() const {
    return amount_ == 0;
  }
  
  void clear() {
    amount_ = 0;
  }
  
  void shrink_to_fit() {
    char* change = new char[amount_ + 1];
    std::copy(str_, str_ + amount_, change);
    change[amount_] = '\0';
    delete[] str_;
    str_ = change;
    cap_ = amount_;
  }
  
  const char* data() const {
    return &str_[0];
  }
  
  char* data() {
    return &str_[0];
  }
  
  ~String() {
    delete[] str_;
  }
};

bool operator<(const String& one, const String& two) {
  const char* lhs = one.data();
  const char* rhs = two.data();
  size_t sz_1 = one.size();
  size_t sz_2 = two.size();
  if (sz_1 != sz_2) {
    if (memcmp(lhs, rhs, std::min(sz_1, sz_2)) == 0) {
      return sz_1 < sz_2;
    }
    return memcmp(lhs, rhs, std::min(sz_1, sz_2)) < 0;
  }
  return memcmp(lhs, rhs, sz_1) < 0;
}

String operator+(char a, const String& one) {
  String change(1, a);
  change += one;
  return change;
}

String operator+(const String& one, char a) {
  String change = one;
  change += a;
  return change;
}

String operator+(const String& one, const String& two) {
  String change = one;
  change += two;
  return change;
}

std::ostream& operator<<(std::ostream& out, const String& one) {
  size_t sz = one.size();
  for (size_t i = 0; i < sz; ++i) {
    out << one[i];
  }
  return out;
}

std::istream& operator>>(std::istream& in, String& one) {
  one.clear();
  char i;
  while(in.get(i) && !isspace(i)) {
    one += i;
  }
  return in;
}

bool operator==(const String& one, const String& two) {
  const char* lhs = one.data();
  const char* rhs = two.data();
  size_t sz_1 = one.size();
  size_t sz_2 = two.size();
  if (sz_1 != sz_2) {
    return false;
  }
  return memcmp(lhs, rhs, sz_1) == 0;
}

bool operator!=(const String& one, const String& two) {
  return !(one == two);
}

bool operator>(const String& one, const String& two) {
  return (two < one);
}

bool operator>=(const String& one, const String& two) {
  return !(one < two);
}

bool operator<=(const String& one, const String& two) {
  return !(one > two);
}

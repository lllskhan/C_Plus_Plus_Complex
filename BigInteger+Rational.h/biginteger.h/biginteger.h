#include <algorithm>
#include <array>
#include <iostream>
#include <initializer_list>
#include <string>

const long long base = 1e9;
const int rank = 9;

class BigInteger {
private:
  std::vector<long long> storage;
  int sign = 7;

  friend std::ostream& operator<<(std::ostream& out, const BigInteger& one);
  friend std::istream& operator>>(std::istream& in, BigInteger& one);
  friend bool operator<(const BigInteger& one, const BigInteger& two);
  friend bool operator==(const BigInteger& one, const BigInteger& two);
  friend class Rational;
  
  void verification() {
    while (storage.size() > 1 and storage[storage.size() - 1] == 0) {
      storage.pop_back();
    }
    if (storage.size() == 1 and storage[0] == 0) {
      sign = 0;
    }
  }
  
  bool abs_more(const BigInteger& another) const {
    if (storage.size() != another.storage.size()) {
      return storage.size() > another.storage.size();
    }
    for (int i = static_cast<int>(storage.size()) - 1; i >= 0; --i) {
      if (storage[i] > another.storage[i]) {
        return true;
      }
      if (storage[i] < another.storage[i]) {
        return false;
      }
    }
    return false;
  }
  
  BigInteger& plus_case(const BigInteger& other) {
    long long prev_addition = 0;
    long long rank_sum = 0;
    for (size_t i = 0; i < storage.size(); ++i) {
      if (i < other.storage.size()) {
        rank_sum = storage[i] + prev_addition + other.storage[i];
      } else {
        rank_sum = storage[i] + prev_addition;
      }
      storage[i] = rank_sum % base;
      prev_addition = rank_sum / base;
    }
    if (prev_addition > 0) {
      storage.push_back(prev_addition);
    }
    return *this;
  }
  
  BigInteger& minus_case(const BigInteger& other) {
    if (this == &other) {
      storage.clear();
      storage.push_back(0);
      sign = 0;
      return *this;
    }
    
    bool taken = false;
    long long rank_dif = 0;
    for (size_t i = 0; i < storage.size(); ++i) {
      if (i < other.storage.size()) {
        rank_dif = storage[i] - taken - other.storage[i];
      } else {
        rank_dif = storage[i] - taken;
      }
      taken = false;
      if (rank_dif < 0) {
        taken = true;
        rank_dif += base;
      }
      storage[i] = rank_dif;
    }
    verification();
    return *this;
  }
  
  BigInteger multiply(const BigInteger& another) const {
    if (sign == 0 || another.sign == 0) {
      return BigInteger(0);
    }
    
    long long rank_multipl = 0;
    BigInteger result(0);
    result.storage.resize(storage.size() + another.storage.size());
    for (size_t i = 0; i < storage.size(); ++i) {
      for (size_t j = 0; j < another.storage.size(); ++j) {
        rank_multipl = storage[i] * another.storage[j] + result.storage[i + j];
        result.storage[i + j] = rank_multipl % base;
        rank_multipl /= base;
        result.storage[i + j + 1] += rank_multipl;
      }
    }
    result.verification();
    result.sign = sign * another.sign;
    return result;
  }
  
  BigInteger divide(BigInteger another) const {
    if (sign == 0 or another.abs_more(*this)) {
      return BigInteger(0);
    }
    
    BigInteger result;
    result.sign = sign * another.sign;
    long long addition = 0;
    BigInteger remainder;
    remainder.sign = 1;
    another.sign = 1;
    
    BigInteger check;
    for (int i = static_cast<int>(storage.size()) - 1; i >= 0; --i) {
      remainder.storage.insert(remainder.storage.begin(), storage[i]);
      remainder.verification();
      if (remainder.storage[remainder.storage.size() - 1] != 0) {
        remainder.sign = 1;
      }
      check = remainder;
      check -= another;
      int var = check.sign;
      if (!var) {
        addition = 1;
      } else if (var == 1) {
        long long left = 0;
        long long right = base + 1;
        long long mid;
        while (left + 1 < right) {
          mid = (left + right) / 2;
          check = another.multiply(mid);
          check -= remainder;
          var = check.sign;
          if (var == 1) {
            right = mid;
          } else {
            left = mid;
            if (var == 0) {
              break;
            }
          }
        }
        addition = left;
      }
      result.storage.insert(result.storage.begin(), addition);
      if (addition) {
        remainder -= another.multiply(addition);
      }
      addition = 0;
    }
    
    result.verification();
    return result;
  }
  
public:
  
  BigInteger() {}
  
  BigInteger(const BigInteger& other)
            : storage(other.storage)
            , sign(other.sign)
  {}
  
  BigInteger(long long number)
  : storage(0)
  , sign(0)
  {
    if (number == 0) {
      sign = 0;
      storage.push_back(0);
      return;
    }
    if (number < 0) {
      sign = -1;
      number = -number;
    } else {
      sign = 1;
    }
    while (number != 0) {
      long long remainder = number % base;
      storage.push_back(remainder);
      number /= base;
    }
    verification();
  }
  
  BigInteger(const std::string& i)
  : storage(0)
  , sign(0)
  {
    if (i.empty()) return;
    
    size_t begin = 0;
    if (i[0] == '-') {
      sign = -1;
      ++begin;
    } else {
      sign = 1;
    }
    
    while (begin < i.size() and i[begin] == '0') {
      ++begin;
    }
    if (begin == i.size()) {
      storage.push_back(0);
      sign = 0;
      return;
    }
    
    std::string num_str = i.substr(begin);
    for (int i = static_cast<int>(num_str.size()); i > 0; i -= rank) {
      int start = std::max(0, i - rank);
      storage.push_back(std::stoll(num_str.substr(start, i - start)));
    }
    verification();
  }
  
  BigInteger& operator=(const BigInteger& other) {
    if (this != &other) {
      storage = other.storage;
      sign = other.sign;
    }
    return *this;
  }
  
  explicit operator bool() const {
    return sign != 0 && sign != 7;
  }
  
  BigInteger operator-() const {
    BigInteger copy = *this;
    if (sign == 1) {
      copy.sign = -1;
    } else if (sign == -1) {
      copy.sign = 1;
    }
    return copy;
  }
  
  std::string toString() const {
    if (sign == 0) {
      return "0";
    }
    if (sign == 7) {
      return "";
    }
    std::string result;
    if (sign == -1) {
      result += "-";
    }
    
    result += std::to_string(storage.back());
    for (int i = static_cast<int>(storage.size() - 2); i >= 0; --i) {
      std::string add = std::to_string(storage[i]);
      add = std::string(rank - add.size(), '0') + add;
      result += add;
    }
    return result;
  }
  
  BigInteger& operator+=(const BigInteger& another) {
    if (sign == 0) {
      *this = another;
      return *this;
    }
    if (another.sign == 0) {
      return *this;
    }
    
    BigInteger copy(another);
    bool var = abs_more(another);
    
    if (sign == another.sign) {
      *this = var ? plus_case(another) : copy.plus_case(*this);
      return *this;
    }
    *this = var ? minus_case(another) : copy.minus_case(*this);
    return *this;
  }
  
  BigInteger& operator-=(const BigInteger& two) {
    *this += -two;
    return *this;
  }
  
  BigInteger& operator++() {
    *this += 1;
    return *this;
  }
  
  BigInteger operator++(int) {
    BigInteger copy = *this;
    *this += 1;
    return copy;
  }
  
  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }
  
  BigInteger operator--(int) {
    BigInteger copy = *this;
    *this -= 1;
    return copy;
  }
  
  friend BigInteger operator-(const BigInteger& one, const BigInteger& two) {
    BigInteger copy = one;
    copy -= two;
    return copy;
  }

  BigInteger& operator*=(const BigInteger& another) {
    *this = multiply(another);
    return *this;
  }
  
  BigInteger& operator/=(const BigInteger& another) {
    *this = divide(another);
    return *this;
  }
  
  BigInteger& operator%=(const BigInteger& another) {
    BigInteger copy(*this);
    copy /= another;
    copy *= another;
    *this -= copy;
    return *this;
  }
  
  ~BigInteger() {
    storage.clear();
    sign = 0;
  }
};

bool operator<(const BigInteger& one, const BigInteger& two) {
  if (one.sign != two.sign) {
    return one.sign < two.sign;
  }
  if (one.sign >= 0) {
    return two.abs_more(one);
  }
  return one.abs_more(two);
}

bool operator>(const BigInteger& one, const BigInteger& two) {
  return two < one;
}

bool operator==(const BigInteger& one, const BigInteger& two) {
  if (one.sign != two.sign or one.storage.size() != two.storage.size()) {
    return false;
  }
  for (int i = static_cast<int>(one.storage.size()) - 1; i >= 0; --i) {
    if (one.storage[i] != two.storage[i]) {
      return false;
    }
  }
  return true;
}

bool operator!=(const BigInteger& one, const BigInteger& two) {
  return !(one == two);
}

bool operator<=(const BigInteger& one, const BigInteger& two) {
  return !(one > two);
}

bool operator>=(const BigInteger& one, const BigInteger& two) {
  return !(one < two);
}

BigInteger operator+(const BigInteger& one, const BigInteger& two) {
  BigInteger copy = one;
  copy += two;
  return copy;
}

BigInteger operator*(const BigInteger& one, const BigInteger& two) {
  BigInteger copy = one;
  copy *= two;
  return copy;
}

BigInteger operator/(const BigInteger& one, const BigInteger& two) {
  BigInteger copy = one;
  copy /= two;
  return copy;
}

BigInteger operator%(const BigInteger& one, const BigInteger& two) {
  BigInteger temp = one;
  temp %= two;
  return temp;
}

BigInteger operator""_bi(const char* i, size_t length) {
  std::string number = i;
  number.resize(length);
  return BigInteger(number);
}

BigInteger operator""_bi(unsigned long long i) {
  std::string number = std::to_string(i);
  return BigInteger(number);
}

std::ostream& operator<<(std::ostream& out, const BigInteger& one) {
  std::string output = one.toString();
  out << output;
  return out;
}

std::istream& operator>>(std::istream& in, BigInteger& one) {
  std::string number;
  in >> number;
  one = number;
  return in;
}

class Rational {
private:
  BigInteger numerator = 0;
  BigInteger denominator = 1;
  friend bool operator<(const Rational& one, const Rational& two);
  friend bool operator==(const Rational& one, const Rational& two);
  
  BigInteger find_gcd(const BigInteger& one, const BigInteger& two) const {
    if (two.sign == 0) {
      return one;
    }
    return find_gcd(two, one % two);
  }
  
  void irreducable() {
    if (numerator.sign == 0) {
      denominator = 1;
      return;
    }
    BigInteger one = numerator;
    BigInteger two = denominator;
    one.sign = 1;
    two.sign = 1;
    if (denominator != 1) {
      BigInteger gcd = find_gcd(one, two);
      if (gcd != 1) {
        numerator /= gcd;
        denominator /= gcd;
      }
    }
    
    if (denominator.sign == -1) {
      numerator.sign *= -1;
      denominator.sign = 1;
    }
  }
  
public:
  Rational(const BigInteger& one, const BigInteger& two)
          : numerator(one)
          , denominator(two)
  {}
  
  Rational(const BigInteger& one)
          : numerator(one)
          , denominator(1)
  {}
  
  Rational(int number = 0)
          : numerator(number)
          , denominator(1)
  {}
  
  Rational& operator=(const Rational& other) {
    if (this != &other) {
      numerator = other.numerator;
      denominator = other.denominator;
    }
    return *this;
  }
  
  Rational operator-() {
    Rational copy = *this;
    if (numerator.sign == 1) {
      copy.numerator.sign = -1;
    } else if (numerator.sign == -1) {
      copy.numerator.sign = 1;
    }
    return copy;
  }
  
  Rational& operator-=(const Rational& two) {
    numerator = numerator * two.denominator - two.numerator * denominator;
    denominator = denominator * two.denominator;
    irreducable();
    return *this;
  }
  
  Rational& operator+=(const Rational& two) {
    numerator = numerator * two.denominator + two.numerator * denominator;
    denominator = denominator * two.denominator;
    irreducable();
    return *this;
  }
  
  Rational& operator*=(const Rational& two) {
    numerator *=two.numerator;
    denominator *= two.denominator;
    irreducable();
    return *this;
  }
  
  Rational& operator/=(const Rational& two) {
    if (this == &two) {
      numerator = 1;
      denominator = 1;
      numerator.sign = denominator.sign = 1;
      return *this;
    }
    numerator *= two.denominator;
    denominator *= two.numerator;
    irreducable();
    return *this;
  }
  
  std::string toString() const {
    std::string str = numerator.toString();
    if (denominator != 1) {
      str += "/" + denominator.toString();
    }
    return str;
  }
  
  std::string asDecimal(size_t precision = 0) const {
    BigInteger copy(numerator);
    std::string decimal;
    if (numerator.sign == -1) {
      copy.sign = 1;
      decimal += "-";
    }
    decimal += (copy / denominator).toString() + ".";
    
    BigInteger specifier;
    specifier.sign = 1;
    for (size_t i = 0; i < precision / 9; ++i) {
      specifier.storage.push_back(0);
    }
    specifier.storage.push_back(std::pow(10, precision % 9));
    specifier = specifier * copy / denominator - specifier * (copy / denominator);
    std::string spec = specifier.toString();
    
    decimal += std::string(precision - spec.size(), '0') + spec;
    return decimal;
  }
  
  ~Rational() {}
  
  explicit operator double() const {
    double mine;
    std::string str = asDecimal(15);
    mine = std::stod(str);
    return mine;
  }
  
  friend std::ostream& operator<<(std::ostream& out, const Rational& one) {
    out << one.numerator;
    if (one.denominator != 1) {
      out << "/" << one.denominator;
    }
    return out;
  }
};

Rational operator+(const Rational& one, const Rational& two) {
  Rational copy = one;
  copy += two;
  return copy;
}

Rational operator-(const Rational& one, const Rational& two) {
  Rational copy = one;
  copy -= two;
  return copy;
}

Rational operator*(const Rational& one, const Rational& two) {
  Rational copy = one;
  copy *= two;
  return copy;
}

Rational operator/(const Rational& one, const Rational& two) {
  Rational copy = one;
  copy /= two;
  return copy;
}

bool operator<(const Rational& one, const Rational& two) {
  return (one.numerator * two.denominator < one.denominator * two.numerator);
}

bool operator>(const Rational& one, const Rational& two) {
  return (two < one);
}

bool operator==(const Rational& one, const Rational& two)  {
  return (one.numerator * two.denominator == two.numerator * one.denominator);
}

bool operator!=(const Rational& one, const Rational& two)  {
  return !(one == two);
}

bool operator>=(const Rational& one, const Rational& two) {
  return !(one < two);
}

bool operator<=(const Rational& one, const Rational& two) {
  return !(two < one);
}

std::istream& operator>>(std::istream& in, Rational& one) {
  BigInteger input;
  in >> input;
  one = input;
  return in;
}

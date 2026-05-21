#include <iostream>


template <typename... Args>
void print(Args... args) {
    (std::cout << ... << args) << "\n";  // Унарная правая свертка
    // Эквивалентно: std::cout << arg1 << arg2 << ... << argN
}

int main () {
  print(1, 2.5, "hello");  // 1 2.5 hello
}

#include <cstring>
#include <cstdio>
#include <iostream>

char* getline() {
  char* line = new char[10];
  int length = 0;
  int capacity = 10;
  char add = getchar();
  while (!isspace(add) && add != EOF) {
    if (length == capacity - 1) {
      capacity *= 2;
      char* temp = new char[capacity];
      for (int i = 0; i < length; ++i) {
        temp[i] = line[i];
      }
      delete[] line;
      line = temp;
      temp = nullptr;
    }
    line[length] = add;
    length += 1;
    add = getchar();
  }
  line[length] = '\0';
  return line;
}

void push(char**& ultimate, int& elements, int& size) {
  char* add = getline();
  if (elements == size) {
    size *= 2;
    char** temp = new char*[size];
    for (int i = 0; i < elements; ++i) {
        temp[i] = ultimate[i];
      }
    delete[] ultimate;
    ultimate = temp;
    temp = nullptr;
  }
  ultimate[elements] = add;
  elements += 1;
  std::cout << "ok\n";
}

void pop(char**& ultimate, int& elements) {
  if (elements == 0) {
    std::cout << "error\n";
  } else {
    std::cout << ultimate[elements - 1] << "\n";
    delete[] ultimate[elements - 1];
    elements -= 1;
  }
}

void back(char**& ultimate, int& elements) {
  if (elements == 0) {
    std::cout << "error\n";
  } else {
    std::cout << ultimate[elements - 1] << "\n";
  }
}

void clean(char**& ultimate, int& elements) {
  for (int i = 0; i < elements; ++i) {
    delete[] ultimate[i];
  }
  elements = 0;
  std::cout << "ok\n";
}

void end(char**& ultimate, int& elements) {
  for (int i = 0; i < elements; ++i) {
      delete[] ultimate[i];
  }
  delete[] ultimate;
  std::cout << "bye\n";
}

int main() {
  char** ultimate = new char*[5];
  int elements = 0;
  int size = 5;
  char* input = getline();
  while (input[3] != 't') {
    if (input[2] == 'z') {
      std::cout << elements << "\n";
    } else if (input[2] == 'p') {
      pop(ultimate, elements);
    } else if (input[3] == 'h') {
      push(ultimate, elements, size);
    } else if (input[3] == 'k') {
      back(ultimate, elements);
    } else if (input[3] == 'a') {
      clean(ultimate, elements);
    }
    delete[] input;
    input = getline();
    if (std::string(input) == " ") {
      break;
    }
  }
  end(ultimate, elements);
  delete[] input;
}

#include <iostream>
#include <string>

std::string getCppVersion() {
#if __cplusplus == 199711L
    return "C++98";
#elif __cplusplus == 201103L
    return "C++11";
#elif __cplusplus == 201402L
    return "C++14";
#elif __cplusplus == 201703L
    return "C++17";
#elif __cplusplus == 202002L
    return "C++20";
#elif __cplusplus > 202002L
    return "C++23 or later";
#else
    return "Unknown or pre-standard C++";
#endif
}

int main() {
    std::cout << "C++ Version: " << getCppVersion()
        << " (__cplusplus = " << __cplusplus << ")\n";
    return 0;
}

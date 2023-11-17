#include <iostream>
#include <cstdint>
#include <chrono>
#include <string>
#include <fstream>

#include <stdio.h>

int main()
{
    using namespace std::chrono;
    auto start = steady_clock::now();
    std::string_view str = "abcdefghijklmnopqrstuvwxyz0123456789\n";
    auto&& out = std::cout;
    // std::ofstream out("test.txt", std::ios::binary);
    for (uint32_t i = 0; i < 100'000; ++i) {
        out << str;
    }
    out.flush();
    auto end = steady_clock::now();
    std::cout << "Finished in: " << duration_cast<milliseconds>(end - start) << '\n';
}
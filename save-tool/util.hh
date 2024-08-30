#pragma once

#include <string>
#include <span>

void check_m_f(bool x, std::string x_str) {
    if (!x) {
        using namespace std::string_literals;
        std::string error = "failed! "s + x_str + " is false"s;
        throw std::runtime_error(error);
    }
}

#define check_m(x) check_m_f(x, #x)

template<typename A>
std::span<std::byte> span_bytes(std::span<A> x) {
    return std::span<std::byte>(
        reinterpret_cast<std::byte*>(x.data()),
        x.size() * sizeof(A)
    );
}

template<typename To, typename From>
std::span<To> span_cast(std::span<From> from) {
    return std::span<To>(
        reinterpret_cast<To*>(from.data()),
        from.size() * sizeof(From) / sizeof(To)
    );
}

void xor_bytes(std::span<std::byte> data, std::unsigned_integral auto key) {
    auto resized_data = span_cast<decltype(key)>(data);
    for (auto& word: resized_data) {
        word ^= key;
    }
}

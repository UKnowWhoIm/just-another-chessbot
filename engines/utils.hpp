#ifndef CHESS_UTILS_HPP
#define CHESS_UTILS_HPP 1

#include <vector>
#include <string>
#include <ranges>

namespace stringUtils {
    std::vector<std::string> split(const std::string& str, char delimiter = ' ') {
        auto to_string = [](auto&& r) -> std::string {
            const auto data = &*r.begin();
            const auto size = static_cast<std::size_t>(std::ranges::distance(r));

            return std::string{data, size};
        };
        const auto range = str | 
            std::ranges::views::split(delimiter) | 
            std::ranges::views::transform(to_string);

        return {std::ranges::begin(range), std::ranges::end(range)};
    }
}

#endif
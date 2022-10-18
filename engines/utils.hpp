#ifndef CHESS_UTILS_HPP
#define CHESS_UTILS_HPP 1

#include <iostream>
#include <vector>
#include <string>
#include <ranges>

#include "board.cpp"

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

namespace debug {
    void printBoard(boardType board, bool useStdOut=false) {
        logging::d("", "");
        string str = "";
        for (int i=0; i < 64;i++) {
            str += std::to_string(board[i]) + " ";
            if (i % 8 == 7) {
                str += "\n";
            }
        }
        if (useStdOut) {
            std::cout << str << std::endl;
        } else {
            logging::d("ChessBoard", str);
            logging::d("", "");
        }
    }

    void printBoard(Board &boardInstance, bool useStdOut=false) {
        logging::d("", "");
        string str = " ";
        for (int i=0; i < 64;i++) {
            if (boardInstance.pieceAt(i) == ' ') 
                str += '.';
            else
                str += boardInstance.pieceAt(i);
            if (i % 8 == 7) {
                str += "\n";
            }
            str += " ";
        }
        if (useStdOut) {
            std::cout << str << std::endl;
        } else {
            logging::d("ChessBoard", str);
            logging::d("", "");
        }
    }

}

#endif
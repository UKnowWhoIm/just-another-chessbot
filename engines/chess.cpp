#include <iostream>
#include <memory>
#include <bits/stdc++.h>

#include "board.cpp"
#include "chessBot.hpp"
#include "log.hpp"
#include "transpositionTables.hpp"
#include "statsutil.hpp"
#include "uci.hpp"
#include "utils.hpp"


using std::string;
using std::array;
using std::map;

namespace client {
    struct serverInput {
        string fen;
        string gameId;
        moveType move;
        uint8_t maxDepth;
        bool enableTT;
        bool enableAlphaBetaPruning;
    };

    serverInput parseInput(int count, char **args) {
        logging::d("Input", args);
        return {
            args[1],
            args[2],
            {std::stoi(args[3]), std::stoi(args[4])},
            std::stoi(args[5]),
            std::stoi(args[6]),
            std::stoi(args[7])
        };
    }

    string output(const array<string, 2> outputData) {
        return outputData[0] + " " + outputData[1];
    }

    void exitWithError() {
        std::cout << output({"EXC", "ERROR"});
    }

    void run(int argc, char **argv) {
        client::serverInput data = client::parseInput(argc, argv);
        preCalculation::preCalcType preCalculatedData = preCalculation::load();
        ChessBot bot(data.gameId, preCalculatedData);

        Board boardInstance(data.fen, preCalculatedData->PRN);
        array<string, 2> output;
        if (boardInstance.makeMoveIfLegal(preCalculatedData, data.move)) {
            bot.getNextMove(boardInstance);
            // moveType nextMove = bot.getLastCalculatedMove();
            output[0] = "OK";
        } else {
            output[0] = "ERR";
        }
        output[0] = "OK";
        output[1] = boardInstance.getFen();

        std::cout << client::output(output);
        stats::printStats();
        bot.dumpCache();
    }
};

int main(int argc, char **argv) {
    stats::reset();
    if (argc > 1) {
        client::run(argc, argv);
    } else {
        uci::run();
    }
    return 0;
}
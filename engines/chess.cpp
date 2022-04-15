#include<iostream>
#include<string>
#include<bitset>
#include<iterator>
#include<map>

#include <random>
#include <time.h>

#include "chess_core.hpp"

#define MAX_SCORE 9999999
#define MIN_SCORE -9999999

using std::string;
using std::array;
using std::map;
using json = nlohmann::json;

typedef std::bitset<64> boardType;
typedef uint8_t playerType;
typedef array<uint8_t, 2>  moveType;


void printBoard(boardType board) {
    printData("");
    string str = "";
    for (int i=0; i < 64;i++) {
        str += std::to_string(board[i]) + " ";
        if (i % 8 == 7) {
            str += "\n";
        }
    }
    printData(str);
    printData("");
}

void printBoard(Board &boardInstance) {
    printData("");
    string str = "";
    for (int i=0; i < 64;i++) {
        str += boardInstance.pieceAt(i);
        str += ' ';
        if (i % 8 == 7) {
            str += "\n";
        }
    }
    printData(str);
    printData("");
}


namespace ChessBot {
    bool enableAlphaBetaPruning = true;
    uint8_t maxDepth = 3;
    bool debug = true;

    unsigned long getPieceScore(char piece) {
        switch (tolower(piece)) {
        case 'k':
            return 1000000;
        case 'q':
            return 500;
        case 'r':
            return 300;
        case 'b':
            return 200;
        case 'n':
            return 150;
        case 'p':
            return 50;
        }
        return 0;
    }

    long heuristic(Board &boardInstance) {
        long score = 0;
        for (int i=0; i < 64; i++) {
            char piece = boardInstance.pieceAt(i);
            if (piece == ' ')
                continue;
            short int multiplier = Board::getPlayer(piece) == boardInstance.player ? 1 : -1;
            score += multiplier * getPieceScore(piece);
        }
        return score;
    }

    long negaMax(Board &boardInstance, preCalculation::preCalc &preCalcData,  uint8_t depth, long alpha = MIN_SCORE, long beta = MAX_SCORE) {
        if (depth == 0) {
            return heuristic(boardInstance);
        }
        map<uint8_t, boardType> nextMoves = boardInstance.getNextMoves(preCalcData, boardInstance.player);
        long currentMax = MIN_SCORE;
        for (auto itr = nextMoves.begin(); itr != nextMoves.end(); itr++) {
            for (uint8_t i = itr->second._Find_first(); i < 64; i = itr->second._Find_next(i)) {
                Board newBoard(boardInstance);
                newBoard.makeMove({itr->first, i});
                long score = -negaMax(newBoard, preCalcData, depth - 1, -beta, -alpha);
                currentMax = score > currentMax ? score : currentMax;
                alpha = currentMax > alpha ? currentMax : alpha;
                if (enableAlphaBetaPruning && beta <= alpha) {
                    if (debug) {
                        printData("Pruned node");
                    }
                    break;
                }
            }
        }
        return currentMax;
    }

    moveType getNextMove(Board &boardInstance, preCalculation::preCalc &preCalcData) {
        map<uint8_t, boardType> nextMoves = boardInstance.getNextMoves(preCalcData, boardInstance.player);
        if (nextMoves.size() == 0) {
            // TODO Game over
            return {64, 64};
        }
        long currentMax = MIN_SCORE;
        moveType currentBestMove;
        for (auto itr = nextMoves.begin(); itr != nextMoves.end(); itr++) {
            for (uint8_t i = itr->second._Find_first(); i < 64; i = itr->second._Find_next(i)) {
                moveType currentMove = {itr->first, i};
                Board newBoard(boardInstance);
                newBoard.makeMove(currentMove);
                long score = -negaMax(newBoard, preCalcData, maxDepth - 1);
                if (score > currentMax) {
                    currentBestMove = currentMove;
                    currentMax = score;
                }
            }
        }
        return currentBestMove;
    }
}

namespace communicate {
    json parseInput(int count, char **args) {
        string jsonStr = "";
        for (int i=1; i < count; i++) {
            jsonStr += args[i];
        }
        return json::parse(jsonStr);
    }

    string output(map<string, string> outputData) {
        return json(outputData).dump();
    }

    void exitWithError() {
        std::cout << output({{"status", "EXC"}});
    }
}

moveType pickRandomMove(map<uint8_t,boardType> allMoves) {
    for (auto itr = allMoves.begin(); itr != allMoves.end(); itr++) {
        if (rand() % 2) {
            uint8_t start = itr->first;
            for (uint8_t i=itr->second._Find_first(); i < 64; i = itr->second._Find_next(i)) {
                if (rand() % 2)
                    return {start, i};
            }
        }
    }
    return {allMoves.begin()->first, allMoves.begin()->second[0]};
}

map<string, string> pickFirstMove(Board &boardInstance, preCalculation::preCalc &preCalculatedData, moveType move) {
    map<string, string> output = map<string, string>();
    map<uint8_t, boardType> availableMoves = boardInstance.getNextMoves(preCalculatedData, boardInstance.player);

    if (boardInstance.isValidMove(availableMoves, move, preCalculatedData)) {
        boardInstance.makeMove(move);
        moveType nextMove = ChessBot::getNextMove(boardInstance, preCalculatedData);
        if (nextMove[0] != 64) {
            boardInstance.makeMove(nextMove);
        }
        output["status"] = "OK";
    } else {
        output["status"] = "ERR";
    }
    output["fen"] = boardInstance.getFen();
    return output;
}

int main(int argc, char **argv) {
    srand(time(0));
    json data = communicate::parseInput(argc, argv);

    preCalculation::preCalc preCalculatedData = preCalculation::loadJSON();

    Board boardInstance(data["data"]["fen"].get<string>());
    moveType move = data["data"]["move"].get<moveType>();

    std::cout << communicate::output(pickFirstMove(boardInstance, preCalculatedData, move));

    return 0;
}
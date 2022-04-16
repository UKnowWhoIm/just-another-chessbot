#include<iostream>
#include<string>
#include<bitset>
#include<iterator>
#include<map>

#include "chess_core.hpp"

#define MAX_SCORE 9999999
#define MIN_SCORE -9999999

using std::string;
using std::array;
using std::map;
using json = nlohmann::json;


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
    unsigned long prunedNodes;
    uint8_t maxDepth = 5;
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
        vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
        long currentMax = MIN_SCORE;
        for (auto itr = nextMoves.begin(); itr != nextMoves.end(); itr++) {
                moveType currentMove = *itr;
                Board newBoard(boardInstance);
                newBoard.makeMove(currentMove);
                long score = -negaMax(newBoard, preCalcData, depth - 1, -beta, -alpha);
                currentMax = score > currentMax ? score : currentMax;
                alpha = currentMax > alpha ? currentMax : alpha;
                if (enableAlphaBetaPruning && beta <= alpha) {
                    if (debug) {
                        prunedNodes++;
                    }
                    break;
                }
        }
        return currentMax;
    }

    moveType getNextMove(Board &boardInstance, preCalculation::preCalc &preCalcData) {
        vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
        if (nextMoves.size() == 0) {
            // TODO Game over
            return {INVALID_POS, INVALID_POS};
        }
        prunedNodes = 0;
        long currentMax = MIN_SCORE;
        moveType currentBestMove;
        for (auto itr = nextMoves.begin(); itr != nextMoves.end(); itr++) {
            moveType currentMove = *itr;
            Board newBoard(boardInstance);
            newBoard.makeMove(currentMove);
            long score = -negaMax(newBoard, preCalcData, maxDepth - 1);
            if (score > currentMax) {
                currentBestMove = currentMove;
                currentMax = score;
            }
        }
        if (debug) {
            printData((long)prunedNodes);
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

map<string, string> pickFirstMove(Board &boardInstance, preCalculation::preCalc &preCalculatedData, moveType move) {
    map<string, string> output = map<string, string>();
    map<uint8_t, boardType> availableMoves = boardInstance.getNextMoves(preCalculatedData, boardInstance.player);

    if (boardInstance.isValidMove(availableMoves, move, preCalculatedData)) {
        boardInstance.makeMove(move);
        moveType nextMove = ChessBot::getNextMove(boardInstance, preCalculatedData);
        if (nextMove[0] != INVALID_POS) {
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
    json data = communicate::parseInput(argc, argv);

    preCalculation::preCalc preCalculatedData = preCalculation::loadJSON();

    Board boardInstance(data["data"]["fen"].get<string>());
    moveType move = data["data"]["move"].get<moveType>();

    std::cout << communicate::output(pickFirstMove(boardInstance, preCalculatedData, move));

    return 0;
}
#include <iostream>
#include <memory>

#include "board.cpp"
#include "log.hpp"
#include "transpositionTables.hpp"
#include "statsutil.hpp"

using std::string;
using std::array;
using std::map;


void printBoard(boardType board) {
    logging::d("", "");
    string str = "";
    for (int i=0; i < 64;i++) {
        str += std::to_string(board[i]) + " ";
        if (i % 8 == 7) {
            str += "\n";
        }
    }
    logging::d("ChessBoard", str);
    logging::d("", "");
}

void printBoard(Board &boardInstance) {
    logging::d("", "");
    string str = "";
    for (int i=0; i < 64;i++) {
        str += boardInstance.pieceAt(i);
        str += ' ';
        if (i % 8 == 7) {
            str += "\n";
        }
    }
    logging::d("ChessBoard", str);
    logging::d("", "");
}


namespace ChessBot {
    bool enableAlphaBetaPruning = true;
    bool enableTT = true;
    const uint8_t maxDepth = 6;
    array<bool, maxDepth> depthTTFlags;

    void setTTAncientForDepth(ttType transpositionTable, uint8_t depth) {
        if (!depthTTFlags[depth]) {
            depthTTFlags[depth] = true;
            transpositionTable->resetAge();
        }
    }

    void resetDepthTTFlags() {
        for (int i=0; i < maxDepth; i++) {
            depthTTFlags[i] = false;
        }
    }

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
        stats::heruistic();
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

    long negaMax(Board &boardInstance, preCalculation::preCalcType preCalcData, uint8_t depth, ttType transpositionTable, long alpha = MIN_SCORE, long beta = MAX_SCORE) {
        if (depth == 0) {
            return heuristic(boardInstance);
        }
        setTTAncientForDepth(transpositionTable, depth);
        std::shared_ptr<HashEntry> ttEntry = transpositionTable->get(boardInstance.getZobristHash());
        if (ttEntry != nullptr) {
            if (ttEntry->depth >= depth) {
                if (!ttEntry->isBetaCutOff) {
                    // Exact
                    stats::hitTTExact();
                    return ttEntry->score;
                } else {
                    // Beta cutoff
                    stats::hitTTBeta();
                    beta = beta > ttEntry->score ? ttEntry->score : beta;
                }
                if (enableAlphaBetaPruning && beta <= alpha) {
                    stats::prune();
                    return ttEntry->score;
                }
            }
        }
        moveType bestMove;
        vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
        long currentMax = MIN_SCORE;
        for (auto currentMove = nextMoves.begin(); currentMove != nextMoves.end(); currentMove++) {
                Board newBoard(boardInstance);
                newBoard.makeMove(*currentMove, preCalcData->PRN, true);
                long score = -negaMax(newBoard, preCalcData, depth - 1, transpositionTable, -beta, -alpha);
                if (currentMax < score) {
                    currentMax = score;
                    bestMove = *currentMove;
                }
                alpha = currentMax > alpha ? currentMax : alpha;
                if (enableAlphaBetaPruning && beta <= alpha) {
                    HashEntry t = HashEntry(boardInstance.getZobristHash(), depth, beta, true, *currentMove);
                    transpositionTable->set(t);
                    stats::prune();
                    return beta;
                }
        }
        return currentMax;
    }

    moveType getNextMove(Board &boardInstance, preCalculation::preCalcType preCalcData, ttType transpositionTable) {
        vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
        if (nextMoves.size() == 0) {
            // TODO Game over
            return {INVALID_POS, INVALID_POS};
        }
        long currentMax = MIN_SCORE;
        moveType currentBestMove;
        for (auto itr = nextMoves.begin(); itr != nextMoves.end(); itr++) {
            moveType currentMove = *itr;
            Board newBoard(boardInstance);
            newBoard.makeMove(currentMove, preCalcData->PRN, true);
            long score = -negaMax(newBoard, preCalcData, maxDepth - 1, transpositionTable);
            if (score > currentMax) {
                currentBestMove = currentMove;
                currentMax = score;
            }
        }
        return currentBestMove;
    }
};

namespace communicate {
    struct serverInput {
        string fen;
        string gameId;
        moveType move;
    };

    serverInput parseInput(int count, char **args) {
        logging::d("Input", args[1]);
        return {
            args[1],
            args[2],
            {std::stoi(args[3]), std::stoi(args[4])}
        };
    }

    string output(const array<string, 2> outputData) {
        return outputData[0] + " " + outputData[1];
    }

    void exitWithError() {
        std::cout << output({"EXC", "ERROR"});
    }
};

bool makeMoveIfLegal(Board &boardInstance, preCalculation::preCalcType preCalculatedData, moveType move) {
    map<uint8_t, boardType> availableMoves = boardInstance.getNextMoves(preCalculatedData, boardInstance.player);

    if (boardInstance.isValidMove(availableMoves, move, preCalculatedData)) {
        boardInstance.makeMove(move, preCalculatedData->PRN, true);
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    stats::reset();
    communicate::serverInput data = communicate::parseInput(argc, argv);
    ttType cache;
    if (ChessBot::enableTT) {
        cache = std::make_shared<TranspositionTable>(data.gameId);
    } else {
        cache = std::make_shared<TranspositionTable>();
    }
    ChessBot::resetDepthTTFlags();
    std::shared_ptr<preCalculation::preCalc> preCalculatedData = preCalculation::load();

    Board boardInstance(data.fen, preCalculatedData->PRN);
    array<string, 2> output;
    if (makeMoveIfLegal(boardInstance, preCalculatedData, data.move)) {
        moveType nextMove = ChessBot::getNextMove(boardInstance, preCalculatedData, cache);
        if (nextMove[0] != INVALID_POS) {
            boardInstance.makeMove(nextMove, preCalculatedData->PRN);
        }
        output[0] = "OK";
    } else {
        output[0] = "ERR";
    }
    output[0] = "OK";
    output[1] = boardInstance.getFen();

    std::cout << communicate::output(output);

    cache->dumpCache();
    stats::printStats();
    return 0;
}
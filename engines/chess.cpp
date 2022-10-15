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


namespace chessBot {
    bool enableAlphaBetaPruning = true;
    bool enableTT = true;
    const uint8_t maxDepth = 5;
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

    long heuristic(Board &boardInstance, preCalculation::preCalcType &preCalcData) {
        stats::heruistic();
        long score = 0;
        float attackMultiplier = 10;
        float defenceMultiplier = 8;
        float spaceMultiplier = 3;
        boardType playerPieces = boardType(0), enemyPieces = boardType(0);
        for (int i=0; i < 64; i++) {
            char piece = boardInstance.pieceAt(i);
            if (piece == ' ')
                continue;
            if (Board::getPlayer(piece) == boardInstance.player) {
                playerPieces.set(i);
                score += getPieceScore(piece);
            } else {
                enemyPieces.set(i);
                score -= getPieceScore(piece);
            }
        }
        boardType playerAttacks = boardInstance.getAttackArea(preCalcData, boardInstance.player);
        boardType enemyAttacks = boardInstance.getAttackArea(preCalcData, !boardInstance.player);
        score += (playerAttacks & enemyPieces).count() * attackMultiplier;
        score += (playerAttacks & playerPieces).count() * defenceMultiplier;
        score -= (enemyAttacks & enemyPieces).count() * defenceMultiplier;
        score -= (enemyAttacks & playerPieces).count() * attackMultiplier;

        score += (playerAttacks & ~(playerPieces | enemyPieces)).count() * spaceMultiplier;
        return score;
    }

    long negaMax(Board &boardInstance, preCalculation::preCalcType preCalcData, uint8_t depth, ttType transpositionTable, long alpha = MIN_SCORE, long beta = MAX_SCORE) {
        if (depth == 0) {
            return heuristic(boardInstance, preCalcData);
        }
        setTTAncientForDepth(transpositionTable, depth);
        long currentMax = MIN_SCORE;
        long oldAlpha = alpha;
        moveType bestMove;
        std::shared_ptr<HashEntry> ttEntry = transpositionTable->get(boardInstance.getZobristHash());
        if (ttEntry != nullptr) {
            if (ttEntry->depth == depth) {
                if (ttEntry->flag == TT_EXACT) {
                    // Exact
                    stats::hitTTExact();
                    // TODO: Fix this fucker
                    // return ttEntry->score;
                    // Board newBoard(boardInstance);
                    // newBoard.makeMove(ttEntry->bestMove, preCalcData->PRN, true);
                    // long score = -negaMax(newBoard, preCalcData, depth - 1, transpositionTable, -beta, -alpha);
                    // if (score != ttEntry->score) {
                    //     stats::ttExactInconsistent++;
                    // }
                    // return score;
                } else if (ttEntry->flag == TT_LB) {
                    // Alpha cutoff
                    stats::hitTTAlpha();
                    alpha = std::max(alpha, ttEntry->score);
                } else if (ttEntry->flag == TT_UB) {
                    // Beta cutoff
                    stats::hitTTBeta();
                    beta = std::min(beta, ttEntry->score);
                } else {
                    logging::e("TT", "Invalid flag");
                }
                if (enableAlphaBetaPruning && beta <= alpha) {
                    stats::prune();
                    return ttEntry->score;
                }
            } else {

            }
        }
        vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
        for (auto currentMove = nextMoves.begin(); currentMove != nextMoves.end(); currentMove++) {
            Board newBoard(boardInstance);
            newBoard.makeMove(*currentMove, preCalcData->PRN, true);
            long score = -negaMax(newBoard, preCalcData, depth - 1, transpositionTable, -beta, -alpha);
            if (currentMax < score) {
                currentMax = score;
                alpha = std::max(alpha, currentMax);
                bestMove = *currentMove;
                if (enableAlphaBetaPruning && beta <= currentMax) {
                    stats::prune();
                    break;
                }
            }
        }
        uint8_t ttFlag = TT_EXACT;
        if (currentMax <= oldAlpha) {
            ttFlag = TT_UB;
        } else if (currentMax >= beta) {
            ttFlag = TT_LB;
        }
        HashEntry t(boardInstance.getZobristHash(), depth, currentMax, ttFlag, bestMove);
        transpositionTable->set(t);
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
        for (auto currentMove = nextMoves.begin(); currentMove != nextMoves.end(); currentMove++) {
            Board newBoard(boardInstance);
            newBoard.makeMove(*currentMove, preCalcData->PRN, true);
            long score = -negaMax(newBoard, preCalcData, maxDepth - 1, transpositionTable, currentMax);
            if (score > currentMax) {
                currentBestMove = *currentMove;
                currentMax = score;
            }
        }
        logging::d("ChessBot", "Best move score: " + std::to_string(currentMax));
        return currentBestMove;
    }
};

namespace communicate {
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
    if (chessBot::enableTT) {
        cache = std::make_shared<TranspositionTable>(data.gameId);
    } else {
        cache = std::make_shared<TranspositionTable>();
    }
    chessBot::resetDepthTTFlags();
    std::shared_ptr<preCalculation::preCalc> preCalculatedData = preCalculation::load();

    Board boardInstance(data.fen, preCalculatedData->PRN);
    array<string, 2> output;
    if (makeMoveIfLegal(boardInstance, preCalculatedData, data.move)) {
        moveType nextMove = chessBot::getNextMove(boardInstance, preCalculatedData, cache);
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
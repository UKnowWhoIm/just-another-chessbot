#ifndef CHESS_BOT_H
#define CHESS_BOT_H 1
#include <iostream>
#include <memory>
#include <bits/stdc++.h>

#include "board.cpp"
#include "log.hpp"
#include "transpositionTables.hpp"
#include "statsutil.hpp"
#include "utils.hpp"

class ChessBot {
    private:
        bool enableAlphaBetaPruning;
        bool enableTT;
        bool enableIterativeDeepening;
        bool enableQuiescenceSearch;
        bool enableNullMovePruning;
        uint8_t maxDepth;
        uint8_t maxQuiescenceDepth;
        array<bool, MAX_ALLOWED_DEPTH> depthTTFlags;
        ttType transpositionTable;
        preCalculation::preCalcType preCalcData;
        bool isInterrupted;
        moveType lastCalculatedMove;

        void setTTAncientForDepth(uint8_t depth) {
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

        long quiescenceSearch(Board &boardInstance, uint8_t depth, long alpha, long beta) {
            if (isInterrupted) {
                return INTERRUPTED_SCORE;
            }
            long score = heuristic(boardInstance);
            if (depth == 0 || !enableQuiescenceSearch) {
                return score;
            }
            if (score >= beta) {
                return score;
            }
            if (score > alpha) {
                alpha = score;
            }
            stats::quiescenceSearch();
            vector<moveType> moves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
            for (moveType move : moves) {
                if (isInterrupted) {
                    return INTERRUPTED_SCORE;
                }
                if (boardInstance.isCapture(move)) {
                    Board newBoard(boardInstance);
                    newBoard.makeMove(move, preCalcData->PRN);
                    score = -quiescenceSearch(newBoard, depth - 1, -beta, -alpha);
                    if (std::abs(score) == INTERRUPTED_SCORE) {
                        return INTERRUPTED_SCORE;
                    }
                    alpha = std::max(alpha, score);
                    if (score >= beta) {
                        break;
                    }
                }
            }
            return alpha;
        }

        long negaMax(Board &boardInstance, uint8_t depth, bool isNullMove = false, long alpha = MIN_SCORE, long beta = MAX_SCORE) {
            if (isInterrupted) {
                return INTERRUPTED_SCORE;
            }
            setTTAncientForDepth(depth);
            long currentMax = MIN_SCORE;
            long oldAlpha = alpha;
            moveType bestMove;
            std::shared_ptr<HashEntry> ttEntry = transpositionTable->get(boardInstance.getZobristHash());
            if (ttEntry != nullptr) {
                if (ttEntry->depth >= depth) {
                    if (ttEntry->flag == TT_EXACT) {
                        // Exact
                        stats::hitTTExact();
                        // TODO: Fix this
                        // return ttEntry->score;
                        // Board newBoard(boardInstance);
                        // newBoard.makeMove(ttEntry->bestMove, preCalcData->PRN, true);
                        // long score = -negaMax(newBoard, preCalcData, depth - 1, transpositionTable, -beta, -alpha);
                        // if (score != ttEntry->score) {
                        //     stats::ttExactInconsistent++;
                        // }
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
            if (depth == 0) {
                long score = quiescenceSearch(boardInstance, maxQuiescenceDepth, alpha, beta);
                if (std::abs(score) == INTERRUPTED_SCORE) {
                    return INTERRUPTED_SCORE;
                }
                return score;
            }
            if (enableNullMovePruning && !isNullMove && !boardInstance.isInCheck(preCalcData, boardInstance.player)) {
                // Try a null move
                Board newBoard(boardInstance);
                newBoard.player = !newBoard.player;
                long score = -negaMax(newBoard, depth - 1, true, -beta, -beta + 1);
                if (std::abs(score) == INTERRUPTED_SCORE) {
                    return INTERRUPTED_SCORE;
                }
                if (score >= beta) {
                    stats::nullMovePrune();
                    return score;
                }
                stats::nullMoveNotPrune();
            }
            vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
            for (auto currentMove = nextMoves.begin();!isInterrupted && currentMove != nextMoves.end(); currentMove++) {
                long score;
                moveType move = *currentMove;
                if (tolower(boardInstance.pieceAt(move[1])) == 'k') {
                    // King capture is illegal, prune
                    stats::illegalKingCapture();
                    return MAX_SCORE;
                }
                Board newBoard(boardInstance);
                newBoard.makeMove(*currentMove, preCalcData->PRN, true);
                if (currentMove != nextMoves.begin()) {
                    // Perform a null window search
                    score = -negaMax(newBoard, depth - 1, isNullMove, -alpha - 1, -alpha);
                    if (std::abs(score) == INTERRUPTED_SCORE) {
                        return INTERRUPTED_SCORE;
                    }
                    if (score > alpha && score < beta) {
                        // Perform a full search
                        stats::pvZWSFail();
                        score = -negaMax(newBoard, depth - 1, isNullMove, -beta, -alpha);
                        if (std::abs(score) == INTERRUPTED_SCORE) {
                            return INTERRUPTED_SCORE;
                        }
                    } else {
                        stats::pvZWSSuccess();
                    }
                } else {
                    score = -negaMax(newBoard, depth - 1, isNullMove, -beta, -alpha);
                    if (std::abs(score) == INTERRUPTED_SCORE) {
                        return INTERRUPTED_SCORE;
                    }
                }
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

        static bool cmpForMovePair(std::pair<moveType, long> a, std::pair<moveType, long> b) {
            return a.second > b.second;
        }
        
    public:
        void getNextMove(Board &boardInstance) {
            stats::reset();
            lastCalculatedMove = {INVALID_POS, INVALID_POS};
            isInterrupted = false;
            resetDepthTTFlags();
            vector<moveType> nextMoves = boardInstance.orderedNextMoves(preCalcData, boardInstance.player);
            if (nextMoves.size() == 0) {
                // TODO Game over
                return;
            }
            vector<std::pair<moveType, long>> moveScoreMap;
            for (const auto &move: nextMoves) {
                moveScoreMap.push_back({move, MIN_SCORE});
            }
            long currentMax = MIN_SCORE;
            uint8_t currentDepth = enableIterativeDeepening ? std::min(3, maxDepth - 1) : maxDepth - 1;
            for (; !isInterrupted && currentDepth < maxDepth; currentDepth++) {
                for (auto currentPair = moveScoreMap.begin();!isInterrupted && currentPair != moveScoreMap.end(); currentPair++) {
                    moveType currentMove = currentPair->first;
                    if (tolower(boardInstance.pieceAt(currentMove[1])) == 'k') {
                        // King capture is illegal, TODO handle this    
                    }
                    Board newBoard(boardInstance);
                    newBoard.makeMove(currentMove, preCalcData->PRN, true);
                    long score = -negaMax(newBoard, currentDepth);
                    if (std::abs(score) == INTERRUPTED_SCORE) {
                        break;
                    }
                    if (score > currentMax) {
                        lastCalculatedMove = currentMove;
                        currentMax = score;
                    }
                    currentPair->second = std::max(currentPair->second, score);
                }
                std::sort(moveScoreMap.begin(), moveScoreMap.end(), cmpForMovePair);
            }
            logging::d("ChessBot", "Best move score: " + std::to_string(currentMax) + "with depth: " + std::to_string(currentDepth));
        }

        void interrupt(bool isUCIMode=false) {
            isInterrupted = true;
            stats::printStats();
            if (isUCIMode) {
                std::cout << "bestmove " << getLastCalculatedMoveAsNotation() << std::endl;
            }
        }

        ChessBot(string ttFileName, preCalculation::preCalcType preCalcData) {
            enableAlphaBetaPruning = true;
            enableIterativeDeepening = true;
            enableTT = true;
            enableNullMovePruning = false;
            enableQuiescenceSearch = false;
            maxDepth = 6;
            maxQuiescenceDepth = 3;
            isInterrupted = false;
            lastCalculatedMove = {INVALID_POS, INVALID_POS};
            if (enableTT) {
                this->transpositionTable = std::make_unique<TranspositionTable>(ttFileName);
            } else {
                this->transpositionTable = std::make_unique<TranspositionTable>();
            }
            this->preCalcData = preCalcData;
        }

        void newGame() {
            transpositionTable->clear();
        }

        void dumpCache() {
            transpositionTable->dumpCache();
        }

        void setEnableAlphaBetaPruning(bool enable) {
            enableAlphaBetaPruning = enable;
        }

        void setEnableIterativeDeepening(bool enable) {
            enableIterativeDeepening = enable;
        }

        void setEnableTT(bool enable) {
            bool isChanged = enableTT != enable;
            enableTT = enable;
            if (isChanged && !enable) {
                transpositionTable = std::make_unique<TranspositionTable>();
            } else if (isChanged && enable) {
                transpositionTable = std::make_unique<TranspositionTable>("random");
            }
        }

        void setEnableNullMovePruning(bool enable) {
            enableNullMovePruning = enable;
        }

        void setEnableQuiescenceSearch(bool enable) {
            enableQuiescenceSearch = enable;
        }

        void setMaxDepth(uint8_t depth) {
            maxDepth = depth;
        }

        void setMaxQuiescenceDepth(uint8_t depth) {
            maxQuiescenceDepth = depth;
        }

        bool getIsInterrupted() {
            return isInterrupted;
        }

        moveType getLastCalculatedMove() {
            return lastCalculatedMove;
        }

        string getLastCalculatedMoveAsNotation() {
            return Board::getNotation(lastCalculatedMove[0]) + Board::getNotation(lastCalculatedMove[1]);
        }

        bool getEnableAlphaBetaPruning() {
            return enableAlphaBetaPruning;
        }

        bool getEnableIterativeDeepening() {
            return enableIterativeDeepening;
        }

        bool getEnableTT() {
            return enableTT;
        }

        bool getEnableNullMovePruning() {
            return enableNullMovePruning;
        }

        bool getEnableQuiescenceSearch() {
            return enableQuiescenceSearch;
        }

        uint8_t getMaxDepth() {
            return maxDepth;
        }

        uint8_t getMaxQuiescenceDepth() {
            return maxQuiescenceDepth;
        }
};
#endif
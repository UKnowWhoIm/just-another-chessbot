#include <iostream>
#include <memory>
#include <bits/stdc++.h>

#include "board.cpp"
#include "log.hpp"
#include "transpositionTables.hpp"
#include "statsutil.hpp"
#include "utils.hpp"

#define MAX_ALLOWED_DEPTH 10
#define MIN_ALLOWED_DEPTH 1

using std::string;
using std::array;
using std::map;


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

namespace uci {
    void displayOptions() {
        std::cout << "option name Depth type spin default 5 min 1 max 7" << std::endl;
        std::cout << "option name EnableAlphaBetaPruning type check default true" << std::endl;
        std::cout << "option name EnableTranspositionTable type check default true" << std::endl;
        std::cout << "option name EnableIterativeDeepening type check default true" << std::endl;
        std::cout << "option name EnableNullMovePruning type check default false" << std::endl;
        std::cout << "option name EnableQuiescenceSearch type check default false" << std::endl;
    }

    bool validateCheckType(const string& input, const string optionName) {
        if (input == "true" || input == "false") {
            return true;
        }
        std::cout << "option name " << optionName << " type check default true" << std::endl;
        return false;
    }

    void run() {
        const string startPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 1 1";
        string input;
        preCalculation::preCalcType preCalculatedData = preCalculation::load();
        ChessBot bot("random", preCalculatedData);
        bool isCalculatingMove = false;
        Board board(startPos, preCalculatedData->PRN);
        do {
            std::getline(std::cin, input);
            vector<string> inputArgs = stringUtils::split(input);
            for (auto &arg: inputArgs) {
                std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
            }
            if (input == "uci") {
                std::cout << "id name JustAnotherChessBot" << std::endl;
                std::cout << "id author UKnowWhoIm" << std::endl << std::endl;
                displayOptions();
                std::cout << "uciok" << std::endl;
            } else if (input == "isready") {
                std::cout << "readyok" << std::endl;
            } else if (input == "quit") {
                exit(0);
            } else if (inputArgs[0] == "setoption") {
                if ((inputArgs.size() >= 3
                    && inputArgs.size() != 5)
                    || inputArgs[1] != "name"
                    || (inputArgs.size() != 5 || inputArgs[3] != "value")) {

                    std::cout << "Invalid setoption command" << std::endl;
                    continue;
                }
                if (inputArgs[2] == "depth") {
                    uint8_t depth = std::stoi(inputArgs[3]);
                    if (depth > MAX_ALLOWED_DEPTH) {
                        std::cout << "Depth cannot be greater than " << MAX_ALLOWED_DEPTH << std::endl;
                    } else if (depth < MIN_ALLOWED_DEPTH) {
                        std::cout << "Depth cannot be lesser than " << MIN_ALLOWED_DEPTH << std::endl;
                    } else {
                        bot.setMaxDepth(std::stoi(inputArgs[3]));
                    }
                } else if (inputArgs[2] == "enabletranspositiontable") {
                    if (validateCheckType(inputArgs[4], "enabletranspositiontable")) {
                        bot.setEnableTT(inputArgs[4] == "true");
                    }
                } else if (inputArgs[2] == "enablealphabetapruning") {
                    if (validateCheckType(inputArgs[4], "enablealphabetapruning")) {
                        bot.setEnableAlphaBetaPruning(inputArgs[4] == "true");
                    }
                } else if (inputArgs[2] == "enableiterativedeepening") {
                    if (validateCheckType(inputArgs[4], "enableiterativedeepening")) {
                        bot.setEnableIterativeDeepening(inputArgs[4] == "true");
                    }
                } else if (inputArgs[2] == "enablenullmovepruning") {
                    if (validateCheckType(inputArgs[4], "enablenullmovepruning")) {
                        bot.setEnableNullMovePruning(inputArgs[4] == "true");
                    }
                } else if (inputArgs[2] == "enablequiescencesearch") {
                    if (validateCheckType(inputArgs[4], "enablequiescencesearch")) {
                        bot.setEnableQuiescenceSearch(inputArgs[4] == "true");
                    }
                } else {
                    std::cout << "Invalid setoption command" << std::endl;
                }
            } else if (input == "ucinewgame") {
                bot.newGame();
            } else if (inputArgs[0] == "position") {
                uint8_t movesStartIndex = 1;
                if (inputArgs[1] == "startpos") {
                    movesStartIndex++;
                    board = Board(startPos, preCalculatedData->PRN);
                } else if (inputArgs[1] == "fen") {
                    string fen = "";
                    movesStartIndex += 6; // 5 spaces in fen + 1 "fen" string
                    for (uint8_t i = 2; i < inputArgs.size() && inputArgs[i] != "moves"; i++) {
                        fen += inputArgs[i] + " ";
                    }
                    board = Board(fen, preCalculatedData->PRN);
                }
                if (inputArgs.size() > movesStartIndex && inputArgs[movesStartIndex] == "moves") {
                    for (uint8_t i = movesStartIndex + 1; i < inputArgs.size(); i++) {
                        board.makeMove(inputArgs[i], preCalculatedData->PRN);
                    }
                }
            } else if (inputArgs[0] == "go") {
                if (inputArgs.size() == 1 || inputArgs[1] == "infinite") {
                    // Infinite search
                    isCalculatingMove = true;
                    std::thread blockingThread([](ChessBot &bot, Board &board) {
                        std::thread aiThread(&ChessBot::getNextMove, &bot, std::ref(board));
                        aiThread.join();
                        if (!bot.getIsInterrupted()) {
                            // Calculation is done
                            std::cout << "bestmove " << bot.getLastCalculatedMoveAsNotation() << std::endl;
                        }
                    }, std::ref(bot), std::ref(board));
                    blockingThread.detach();
                } else if (inputArgs[1] == "movetime") {
                    // Search for a specific amount of time(ms)
                    isCalculatingMove = true;
                    std::thread timerThread([](Board &boardInstance, ChessBot &bot, unsigned long timeMs) {
                        std::thread internalThread(&ChessBot::getNextMove, &bot, std::ref(boardInstance));
                        std::this_thread::sleep_for(std::chrono::milliseconds(timeMs));
                        bot.interrupt(true);
                        internalThread.join();
                    }, std::ref(board), std::ref(bot), std::stoi(inputArgs[2]));
                    timerThread.detach();
                }
            } else if (isCalculatingMove && input == "stop") {
                isCalculatingMove = false;
                bot.interrupt(true);
            } else if (input == "d") {
                printBoard(board, true);
            }
        } while (true);
    }
};

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

    bool makeMoveIfLegal(Board &boardInstance, preCalculation::preCalcType preCalculatedData, moveType move) {
        map<uint8_t, boardType> availableMoves = boardInstance.getNextMoves(preCalculatedData, boardInstance.player);

        if (boardInstance.isValidMove(availableMoves, move, preCalculatedData)) {
            boardInstance.makeMove(move, preCalculatedData->PRN, true);
            return true;
        }
        return false;
    }

    void run(int argc, char **argv) {
        client::serverInput data = client::parseInput(argc, argv);
        preCalculation::preCalcType preCalculatedData = preCalculation::load();
        ChessBot bot(data.gameId, preCalculatedData);

        Board boardInstance(data.fen, preCalculatedData->PRN);
        array<string, 2> output;
        if (makeMoveIfLegal(boardInstance, preCalculatedData, data.move)) {
            bot.getNextMove(boardInstance);
            moveType nextMove = bot.getLastCalculatedMove();
            if (nextMove[0] != INVALID_POS) {
                boardInstance.makeMove(nextMove, preCalculatedData->PRN);
            }
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
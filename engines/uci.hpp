#ifndef CHESS_UCI_H
#define CHESS_UCI_H 1

#include <iostream>
#include <memory>
#include <bits/stdc++.h>
#include <thread>

#include "board.cpp"
#include "log.hpp"
#include "transpositionTables.hpp"
#include "statsutil.hpp"
#include "chessBot.hpp"
#include "utils.hpp"

namespace uci {
    void formatOption(std::string name, int defaultVal, int min, int max) {
        // Type: Spin
        std::cout << "option name " << name << " type spin default " << defaultVal << " min "
            << min << " max " << max << std::endl;
    }

    void formatOption(std::string name, bool defaultVal) {
        // Type: Check
        std::cout << "option name " << name << " type check default " << (defaultVal ? "true" : "false")
            << std::endl;
    }

    void displayOptions() {
        formatOption("Depth", 5, 1, 7);
        formatOption("EnableAlphaBetaPruning", true);
        formatOption("EnableTranspositionTable", true);
        formatOption("EnableIterativeDeepening", true);
        formatOption("EnableNullMovePruning", false);
        formatOption("EnableQuiescenceSearch", false);
        formatOption("AttackMultiplier", 20, 1, 100);
        formatOption("DefenceMultiplier", 16, 1, 100);
        formatOption("SpaceMultiplier", 8, 1, 100);
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
        ChessBot bot(randomUtils::getHashFileName(), preCalculatedData);
        bool isCalculatingMove = false;
        Board board(startPos, preCalculatedData->PRN);
        do {
            std::getline(std::cin, input);
            vector<string> inputArgs = stringUtils::split(input), originalArgs(inputArgs.size());
            std::copy(inputArgs.begin(), inputArgs.end(), originalArgs.begin());
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
                } else if (inputArgs[2] == "attackmultiplier") {
                    bot.setAttackMultiplier(std::stof(inputArgs[4]));
                } else if (inputArgs[2] == "defencemultiplier") {
                    bot.setDefenceMultiplier(std::stof(inputArgs[4]));
                } else if (inputArgs[2] == "spacemultiplier") {
                    bot.setSpaceMultiplier(std::stof(inputArgs[4]));
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
                        fen += originalArgs[i] + " ";
                    }
                    board = Board(fen, preCalculatedData->PRN);
                }
                if (inputArgs.size() > movesStartIndex && inputArgs[movesStartIndex] == "moves") {
                    for (uint8_t i = movesStartIndex + 1; i < inputArgs.size(); i++) {
                        if (!board.makeMoveIfLegal(preCalculatedData, inputArgs[i])) {
                            logging::d("UCI", "Invalid move: " + inputArgs[i] + " from position " + board.getFen());
                        }
                    }
                }
            } else if (inputArgs[0] == "go") {
                logging::d("UCI", "Player: " + std::to_string(board.player));
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
                debug::printBoard(board, true);
            }
        } while (true);
    }
};
#endif
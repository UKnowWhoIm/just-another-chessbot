#include <iostream>

#include "board.cpp"
#include "chessBot.hpp"
#include "preCalculation.hpp"
#include "utils.hpp"

int main() {
    preCalculation::preCalcType preCalculatedData = preCalculation::load();
    Board b("r5kr/3q1pp1/1pp3n1/p2p4/P5P1/2PbPP2/NPKP1nB1/1RB3R1 w - - 9 36", preCalculatedData->PRN);
    ChessBot bot("test", preCalculatedData);
    bot.getNextMove(b);
    std::cout << bot.getLastCalculatedMoveAsNotation();
    return 0;
}

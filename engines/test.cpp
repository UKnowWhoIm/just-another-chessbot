#include <iostream>

#include "board.cpp"
#include "chessBot.hpp"
#include "preCalculation.hpp"
#include "utils.hpp"

class TestCase {
    private:
        Board board;
        ChessBot bot;
        preCalculation::preCalcType preCalculatedData;
    public:
        TestCase(preCalculation::preCalcType preCalcData) {
            preCalculatedData = preCalcData;
            board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 1 1", preCalculatedData->PRN);
            bot = ChessBot(randomUtils::getHashFileName(), preCalculatedData);
        }
        TestCase(const TestCase& otherTestCase) {
            this->preCalculatedData = otherTestCase.preCalculatedData;
            this->board = Board(otherTestCase.board);
            this->bot = ChessBot(otherTestCase.bot);
        }
        void playMove() {
            bot.getNextMove(board);
        }
        Board getBoard() {
            return board;
        }
        ChessBot getBot() {
            return bot;
        }
};

void verifyAlphaBetaPruning(preCalculation::preCalcType preCalcData) {
    TestCase baseCase(preCalcData);
    baseCase.getBot().setEnableNullMovePruning(false);
    baseCase.getBot().setEnableQuiescenceSearch(false);
    baseCase.getBot().setEnableTT(false);
    baseCase.getBot().setEnableIterativeDeepening(false);
    baseCase.getBot().setEnableAlphaBetaPruning(true);
    
    TestCase withAlphaBeta(baseCase);
    TestCase withoutAlphaBeta(withAlphaBeta);
    withoutAlphaBeta.getBot().setEnableAlphaBetaPruning(false);

    for (int i=3; i < 6; i++) {
        withAlphaBeta.getBot().setMaxDepth(i);
        withoutAlphaBeta.getBot().setMaxDepth(i);
        for (int j=0; j < 3; j++) {
            withAlphaBeta.playMove();
            withoutAlphaBeta.playMove();
            assert(withAlphaBeta.getBot().getLastCalculatedMoveAsNotation() == withoutAlphaBeta.getBot().getLastCalculatedMoveAsNotation());
            assert(withAlphaBeta.getBot().getLastCalculatedState().score == withoutAlphaBeta.getBot().getLastCalculatedState().score);
        }
    }
}

int main() {
    preCalculation::preCalcType preCalcData = preCalculation::load();
    verifyAlphaBetaPruning(preCalcData);
}

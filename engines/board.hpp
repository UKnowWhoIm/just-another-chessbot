#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H 1

#include<string>
#include<map>
#include<vector>

#include "definitions.hpp"
#include "preCalculation.hpp"

using std::string;
using std::map;
using std::vector;

class Board {
    private:
        string fen;
        unsigned long long zobristHash;
        char _board[64];

        short int getDirection(playerType player);

        boardType getPawnMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea = false);

        boardType getKnightMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea = false);

        boardType getKingMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea = false);

        uint16_t getMagicHash(boardType &board, unsigned long long magic, boardType& blockerMask, bool isBishop);

        boardType getBishopMoves(preCalculation::preCalcType data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea = false);

        boardType getRookMoves(preCalculation::preCalcType data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea = false);

        boardType getQueenMoves(preCalculation::preCalcType data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea = false);

        boardType getCastlingMoves(preCalculation::preCalcType preCalculatedData, uint8_t kingPos, bool isLeftClear, bool isRightClear);

        void parseCastlingRights(int &index);

        uint16_t parseIntInFEN(int &index);

        static unsigned short getZobristStartIndex(char piece);

        void changeZobristOnMove(prnType &PRN, moveType &move);

        void disableQueenSideCastling(prnType &PRN);

        void disableKingSideCastling(prnType &PRN);

        void changeZobristOnPromotion(prnType &PRN, char piece, uint8_t pos);
    public:
        uint8_t enPassantSquare;
        bool castlingRights[2][2]; // [color][side] Queen -> 0, King -> 1
        playerType player;
        uint8_t halfMoves;
        uint16_t fullMoves;

        boardType getPiecesOfPlayer(playerType player);

        static uint8_t parseNotation(string notation);

        static string getNotation(int pos);

        void calcZobristHash(prnType &PRN);

        unsigned long long getZobristHash() {
            return zobristHash;
        }

        bool getCastlingRights(int color, int side) {
            return castlingRights[color][side];
        }

        string getFen() {
            return fen;
        }

        static playerType getPlayer(char piece);

        char pieceAt(uint8_t pos);

        void parseFen();

        Board(string _fen, prnType &PRN);

        Board(Board &original);

        bool isCapture(moveType move);

        bool isInCheck(preCalculation::preCalcType& preCalculatedData, playerType player);

        map<uint8_t, boardType> getNextMoves(preCalculation::preCalcType preCalculatedData, playerType player, bool quick = false);

        boardType getAttackArea(preCalculation::preCalcType preCalculatedData, playerType player);

        vector<moveType> orderedNextMoves(preCalculation::preCalcType preCalculatedData, playerType player);
        
        uint8_t findKing(playerType player);

        string exportFEN();

        void makeMove(array<uint8_t, 2> move, prnType &PRN, bool isComputer=false, bool changeFEN=true);

        void makeMove(const string& move, prnType &PRN, bool isComputer=false, bool changeFEN=true);

        bool makeMoveIfLegal(preCalculation::preCalcType preCalculatedData, moveType move);

        bool makeMoveIfLegal(preCalculation::preCalcType preCalculatedData, const string& move);

        bool isValidMove(map<uint8_t, boardType> &moves, array<uint8_t, 2> move, preCalculation::preCalcType preCalculatedData);

        bool isValidMove(array<uint8_t, 2> move, preCalculation::preCalcType preCalculatedData);
};

#endif
#include<iostream>
#include<fstream>
#include<string>
#include<bitset>
#include<vector>
#include<iterator>
#include<map>

#include "json.hpp"

#define BLACK 0
#define WHITE 1

using std::string;
using std::array;
using std::vector;
using json = nlohmann::json;

typedef std::bitset<64> boardType;
typedef u_int8_t playerType;


namespace communicate {
    json parseInput(int count, char **args) {
        string jsonStr = "";
        for (int i=1; i < count; i++) {
            jsonStr += args[i];
        }
        return json::parse(jsonStr);
    }

    string output(json j) {
        return j.dump();
    }
}

namespace preCalculation {
    struct preCalc {
        array<array<boardType, 512>, 64> bishopLookup;
        array<array<boardType, 4096>, 64> rookLookup;
        array<unsigned long long, 64> bishopMagic;
        array<unsigned long long, 64> rookMagic;
    };

    preCalc loadJSON() {
        string attacksFile = "chess_utils/attacks.json";
        string magicFile = "chess_utils/magics.json";

        json attacks, magics;
        std::ifstream aF(attacksFile), mF(magicFile);
        aF >> attacks;
        mF >> magics;
        array<array<unsigned long long, 512>, 64> rawBishop = attacks.at("bishop").get<array<array<unsigned long long, 512>, 64>>();
        array<array<unsigned long long, 4096>, 64> rawRook =  attacks.at("rook").get<array<array<unsigned long long, 4096>, 64>>();
        array<array<boardType, 512>, 64> bishopLookup;
        array<array<boardType, 4096>, 64> rookLookup;
        for (int i=0; i<64; i++) {
            for (int j=0; j<4096; j++) {
                if (j < 512) {
                    bishopLookup[i][j] = boardType(rawBishop[i][j]);
                } 
                rookLookup[i][j] = boardType(rawRook[i][j]);   
            }
        }
        
        return {
            bishopLookup,
            rookLookup,
            magics.at("bishop").get<array<unsigned long long, 64>>(),
            magics.at("rook").get<array<unsigned long long, 64>>()
        };
    }
}

class Board {
    private:
        string fen;
        char _board[64];

        boardType getPawnMoves(boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t pos) {
            u_int8_t direction = (player == BLACK) ? 1 : -1;
            boardType allPieces = whitePieces | blackPieces;
            boardType nonCaptures = boardType(0);
            boardType captures = boardType(0);
            boardType* enemyPieces = (player == BLACK) ? &whitePieces : &blackPieces;
            
            // Normal Move
            u_int8_t nextMove = pos + direction * 8;
            if (!(allPieces[nextMove]) && nextMove >= 0 && nextMove < 64) {
                nonCaptures |= nextMove;
            }
            // Double Move at initial position
            nextMove = pos + direction * 16;
            if (player == BLACK && pos / 8 == 1 && !allPieces[nextMove] && nextMove >= 0 && nextMove < 64) {
                nonCaptures |= nextMove;
            } else if (player == WHITE && pos / 8 == 6 && !allPieces[nextMove] && nextMove >= 0 && nextMove < 64) {
                nonCaptures |= nextMove;
            }

            // Capture Left
            nextMove = pos + direction * 8 - 1;
            if (pos % 8 != 0 && nextMove >= 0 && nextMove < 64) {
                captures |= nextMove;
            }
            // Capture Right
            nextMove = pos + direction * 8 + 1;
            if (pos % 8 != 7 && nextMove >= 0 && nextMove < 64) {
                captures |= nextMove;
            }
            captures &= *enemyPieces;
            nonCaptures &= allPieces;

            return captures | nonCaptures;
        }

        boardType getKnightMoves(boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t pos) {
            boardType* ownPieces = (player == BLACK) ? &blackPieces : &whitePieces;;
            boardType* enemyPieces = (player == BLACK) ? &whitePieces : &blackPieces;
            boardType movesBoard = boardType(0);
            
            // Left Moves
            if (pos % 8 > 0) {
                if (pos / 8 < 6) 
                    movesBoard |= pos + 15;
                if (pos / 8 > 1)
                    movesBoard |= pos - 17;
                
                if (pos % 8 > 1) {
                    if (pos / 8 > 0)
                        movesBoard |= pos - 10;
                    if (pos / 8 < 7)
                        movesBoard |= pos + 6;
                }
            }

            // Right Moves
            int rightMoves[] = {17, -15, -6, 10};
            if (pos % 8 < 7) {
                if (pos / 8 > 0)
                    movesBoard |= pos - 6;
                if (pos / 8 < 7)
                    movesBoard |= pos + 10;
                
                if (pos % 8 < 6) {
                    if (pos / 8 < 6) 
                        movesBoard |= pos + 17;
                    if (pos / 8 > 1)
                        movesBoard |= pos - 15;
                }
            }
            // Remove capturing own piece
            movesBoard &= ~*ownPieces;
            
            return movesBoard;
        }

        boardType getKingMoves(boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t pos) {
            // DOES NOT INCLUDE CASTLING
            boardType* ownPieces = (player == BLACK) ? &blackPieces : &whitePieces;
            boardType movesBoard = boardType(0);

            if (pos / 8 != 0)
                    movesBoard |= pos - 8;
            if (pos / 8 != 7)
                movesBoard |= pos + 8;
                
            if (pos % 8 != 0) {
                movesBoard |= pos - 1;
                if (pos / 8 != 0)
                    movesBoard |= pos - 7;
                if (pos / 8 != 7)
                    movesBoard |= pos + 9;
            }
            
            if (pos % 8 != 7) {
                movesBoard |= pos + 1;
                if (pos / 8 != 0)
                    movesBoard |= pos - 7;
                if (pos / 8 != 7)
                    movesBoard |= pos + 9;
            }
            
            return movesBoard &= ~*ownPieces;
        }

        u_int16_t getHash(boardType board, unsigned long long magic, bool isBishop) {
            return (board.to_ullong() * magic) >> 64 - (isBishop ? 9 : 12);
        }

        boardType getBishopMoves(preCalculation::preCalc data, boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t pos) {
            boardType allPieces = whitePieces | blackPieces;
            boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
            boardType allMoves = data.bishopLookup[pos][getHash(allPieces, data.bishopMagic[pos], true)];
            return allMoves & ~*ownPieces;
        }

        boardType getRookMoves(preCalculation::preCalc data, boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t pos) {
            boardType allPieces = whitePieces | blackPieces;
            boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
            boardType allMoves = data.bishopLookup[pos][getHash(allPieces, data.rookMagic[pos], false)];
            return allMoves & ~*ownPieces;
        }

        boardType getQueenMoves(preCalculation::preCalc data, boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t pos) {
            boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
            boardType queenMoves = getRookMoves(data, whitePieces, blackPieces, player, pos) | getBishopMoves(data, whitePieces, blackPieces, player, pos);
            return queenMoves & ~*ownPieces;
        }

        boardType getCastlingMoves(preCalculation::preCalc &preCalculatedData, boardType &whitePieces, boardType &blackPieces, playerType player, u_int8_t kingPos) {
            boardType castlingMoves = boardType(0);
            boardType allPieces = boardType(0);
            std::map<u_int8_t, boardType> enemyMoves = getNextMoves(preCalculatedData, !player, true);
            boardType enemyAttacks = boardType(0);

            for (auto itr = enemyMoves.begin(); itr != enemyMoves.end(); itr++) {
                enemyAttacks |= itr->second;
            }

            if (!castlingRights[player][0] && !castlingRights[player][1]) {
                return castlingMoves;
            }
            if (castlingRights[player][0]) {
                bool fail = false;
                if (allPieces[kingPos - 1] || allPieces[kingPos - 2] || allPieces[kingPos - 3]) {
                    fail = true;
                }
                if (enemyAttacks[kingPos - 1] || enemyAttacks[kingPos - 2] || enemyAttacks[kingPos - 3]) {
                    fail = true;
                }
                if (!fail) {
                    castlingMoves |= kingPos - 3;
                }
            }
            if (castlingRights[player][1]) {
                bool fail = false;
                if (allPieces[kingPos + 1] || allPieces[kingPos + 2]) {
                    fail = true;
                }
                if (enemyAttacks[kingPos + 1] || enemyAttacks[kingPos + 2]) {
                    fail = true;
                }
                if (!fail) {
                    castlingMoves |= kingPos + 2;
                }
            }
            return castlingMoves;
        }

    public:
        vector<int> whitePositions;
        vector<int> blackPositions;
        playerType enPassantSquare;
        bool castlingRights[2][2]; // [color][side] Queen -> 0, King -> 1
        playerType player;

        bool getCastlingRights(int color, int side) {
            return castlingRights[color][side];
        }

        playerType getPlayer(char piece) {
            if (piece > 'a' && piece < 'z')
                return WHITE;
            return BLACK;
        }

        Board(string _fen) {
            fen = _fen;
            strcpy(_board, "");
            for (int i=0; fen[i] != ' '; ) {
                if (fen[i] != '/') {
                    if (fen[i] > '0' && fen[i] < '9') {
                        // Number of empty squares
                        i += fen[i] - '0';
                    } else {
                        // Piece
                        _board[i] = fen[i];
                        if (getPlayer(fen[i]) == WHITE) {
                            whitePositions.push_back(i);
                        } else {
                            blackPositions.push_back(i);
                        }
                        i++;
                    }
                } else {
                    i++;
                }
            }
        }

        std::map<u_int8_t, boardType> getNextMoves(preCalculation::preCalc &preCalculatedData, playerType player, bool quick = false) {
            boardType whitePieces = boardType(0), blackPieces = boardType(0);
            u_int8_t kingPos;
            std::map<u_int8_t, boardType> moves = std::map<u_char, boardType>();
            for (auto i=whitePositions.begin(); i != whitePositions.end(); i++) {
                whitePieces |= *i;
            }
            for (auto i=blackPositions.begin(); i != blackPositions.end(); i++) {
                blackPieces |= *i;
            }
            vector<int>* playerSquares = (player == WHITE) ? &whitePositions : &blackPositions;

            for (auto i=playerSquares->begin(); i != playerSquares->end(); i++) {
                u_char piece = tolower(_board[*i]);
                if (piece == 'p') {
                    moves[*i] = getPawnMoves(whitePieces, blackPieces, player, *i);
                } else if (piece == 'n') {
                    moves[*i] = getKnightMoves(whitePieces, blackPieces, player, *i);
                } else if (piece == 'b') {
                    moves[*i] = getBishopMoves(preCalculatedData, whitePieces, blackPieces, player, *i);
                } else if (piece == 'r') {
                    moves[*i] = getRookMoves(preCalculatedData, whitePieces, blackPieces, player, *i);
                } else if (piece == 'q') {
                    moves[*i] = getQueenMoves(preCalculatedData, whitePieces, blackPieces, player, *i);
                } else if (piece == 'k') {
                    moves[*i] = getKingMoves(whitePieces, blackPieces, player, *i);
                    kingPos = *i;
                }
            }
            if (quick)
                return moves;
            moves[kingPos] |= getCastlingMoves(preCalculatedData, whitePieces, blackPieces, player, kingPos);
            return moves;
        }
};

int main(int argc, char **argv) {
    json input = communicate::parseInput(argc, argv);
    preCalculation::preCalc preCalculatedData = preCalculation::loadJSON();


    std::cout << communicate::output("{\"data\": \"Testing\", \"status\": \"OK\"}");
    return 0;
}
#include<fstream>
#include<string>
#include<bitset>
#include<iterator>
#include<map>
#include<vector>

#include "json.hpp"
#include "log.hpp"


#define BLACK 0
#define WHITE 1

using std::string;
using std::array;
using std::map;
using std::vector;
using json = nlohmann::json;

typedef std::bitset<64> boardType;
typedef uint8_t playerType;


namespace preCalculation {
    struct preCalc {
        array<array<boardType, 512>, 64> bishopLookup;
        array<array<boardType, 4096>, 64> rookLookup;
        array<unsigned long long, 64> bishopMagic;
        array<unsigned long long, 64> rookMagic;
        array<boardType, 64> bishopBlockers;
        array<boardType, 64> rookBlockers;
    };

    preCalc loadJSON() {
        // Load the magics and blockers
        string attacksFile = "/app/engines/chess_utils/attacks.json";
        string magicFile = "/app/engines/chess_utils/magics.json";
        string blockerFile = "/app/engines/chess_utils/blockers.json";

        json attacks, magics, blockers;
        std::ifstream aF(attacksFile), mF(magicFile), bF(blockerFile);
        aF >> attacks;
        mF >> magics;
        bF >> blockers;
        array<array<unsigned long long, 512>, 64> rawBishop = attacks.at("bishop").get<array<array<unsigned long long, 512>, 64>>();
        array<array<unsigned long long, 4096>, 64> rawRook =  attacks.at("rook").get<array<array<unsigned long long, 4096>, 64>>();
        array<array<boardType, 512>, 64> bishopLookup;
        array<array<boardType, 4096>, 64> rookLookup;
        array<unsigned long long, 64> rawBishopBlockers = blockers.at("bishop").get<array<unsigned long long, 64>>();
        array<unsigned long long, 64> rawRookBlockers = blockers.at("rook").get<array<unsigned long long, 64>>();
        array<boardType, 64> bishopBlockers;
        array<boardType, 64> rookBlockers;

        for (int i=0; i<64; i++) {
            for (int j=0; j<4096; j++) {
                if (j < 512) {
                    bishopLookup[i][j] = boardType(rawBishop[i][j]);
                } 
                rookLookup[i][j] = boardType(rawRook[i][j]);   
            }
            bishopBlockers[i] = boardType(rawBishopBlockers[i]);
            rookBlockers[i] = boardType(rawRookBlockers[i]);
        }
        
        return {
            bishopLookup,
            rookLookup,
            magics.at("bishop").get<array<unsigned long long, 64>>(),
            magics.at("rook").get<array<unsigned long long, 64>>(),
            bishopBlockers,
            rookBlockers
        };
    }
}

class Board {
    private:
        string fen;
        char _board[64];

        short int getDirection(playerType player) {
            return (player == BLACK) ? 1 : -1;
        }

        boardType getPawnMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos) {
            boardType allPieces = whitePieces | blackPieces;
            boardType nonCaptures = boardType(0);
            boardType captures = boardType(0);
            boardType* enemyPieces = (player == BLACK) ? &whitePieces : &blackPieces;
            
            // Normal Move
            uint8_t nextMove = pos + getDirection(player) * 8;
            if (!(allPieces[nextMove]) && nextMove >= 0 && nextMove < 64) {
                nonCaptures.set(nextMove);
            }
            
            // Double Move at initial position
            nextMove = pos + getDirection(player) * 16;
            uint8_t intermediateSquare = pos + getDirection(player) * 8;
            if (player == BLACK && pos / 8 == 1 && !allPieces[intermediateSquare] && !allPieces[nextMove] && nextMove >= 0 && nextMove < 64) {
                nonCaptures.set(nextMove);
            } else if (player == WHITE && pos / 8 == 6 && !allPieces[intermediateSquare] && !allPieces[nextMove] && nextMove >= 0 && nextMove < 64) {
                nonCaptures.set(nextMove);
            }

            // Capture Left
            nextMove = pos + getDirection(player) * 8 - 1;
            if (pos % 8 != 0 && nextMove >= 0 && nextMove < 64) {
                captures.set(nextMove);
            }
            // Capture Right
            nextMove = pos + getDirection(player) * 8 + 1;
            if (pos % 8 != 7 && nextMove >= 0 && nextMove < 64) {
                captures.set(nextMove);
            }
            captures &= *enemyPieces;
            nonCaptures &= ~allPieces;

            return captures | nonCaptures;
        }

        boardType getKnightMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos) {
            boardType* ownPieces = (player == BLACK) ? &blackPieces : &whitePieces;;
            boardType* enemyPieces = (player == BLACK) ? &whitePieces : &blackPieces;
            boardType movesBoard = boardType(0);

            // Left Moves
            if (pos % 8 > 0) {
                if (pos / 8 < 6) 
                    movesBoard.set(pos + 15);
                if (pos / 8 > 1)
                    movesBoard.set(pos - 17);
                
                if (pos % 8 > 1) {
                    if (pos / 8 > 0)
                        movesBoard.set(pos - 10);
                    if (pos / 8 < 7)
                        movesBoard.set(pos + 6);
                }
            }

            // Right Moves
            if (pos % 8 < 7) {
                if (pos / 8 > 1)
                    movesBoard.set(pos - 15);
                if (pos / 8 < 6)
                    movesBoard.set(pos + 17);

                if (pos % 8 < 6) {
                    if (pos / 8 < 7) 
                        movesBoard.set(pos + 10);                      
                    if (pos / 8 > 0)
                        movesBoard.set(pos - 6);
                }
            }
            // Remove capturing own piece
            movesBoard &= ~*ownPieces;
            
            return movesBoard;
        }

        boardType getKingMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos) {
            // DOES NOT INCLUDE CASTLING
            boardType* ownPieces = (player == BLACK) ? &blackPieces : &whitePieces;
            boardType movesBoard = boardType(0);

            if (pos / 8 != 0)
                movesBoard.set(pos - 8);
            if (pos / 8 != 7)
                movesBoard.set(pos + 8);
                
            if (pos % 8 != 0) {
                movesBoard.set(pos - 1);
                if (pos / 8 != 0)
                    movesBoard.set(pos - 7);
                if (pos / 8 != 7)
                    movesBoard.set(pos + 9);
            }
            
            if (pos % 8 != 7) {
                movesBoard.set(pos + 1);
                if (pos / 8 != 0)
                    movesBoard.set(pos - 7);
                if (pos / 8 != 7)
                    movesBoard.set(pos + 9);
            }
            
            return movesBoard & ~*ownPieces;
        }

        uint16_t getHash(boardType board, unsigned long long magic, boardType blockerMask, bool isBishop) {
            // Mask board with blockers to get relevant squares
            return ((board & blockerMask).to_ullong() * magic) >> 64 - (isBishop ? 9 : 12);
        }

        boardType getBishopMoves(preCalculation::preCalc data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos) {
            boardType allPieces = whitePieces | blackPieces;
            boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
            uint16_t index = getHash(allPieces, data.bishopMagic[pos], data.bishopBlockers[pos], true);
            boardType allMoves = data.bishopLookup[pos][index];
            return allMoves & ~*ownPieces;
        }

        boardType getRookMoves(preCalculation::preCalc data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos) {
            boardType allPieces = whitePieces | blackPieces;
            boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
            uint16_t index = getHash(allPieces, data.rookMagic[pos], data.rookBlockers[pos], false);
            boardType allMoves = data.rookLookup[pos][index];
            return allMoves & ~*ownPieces;
        }

        boardType getQueenMoves(preCalculation::preCalc data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos) {
            boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
            boardType queenMoves = getRookMoves(data, whitePieces, blackPieces, player, pos) | getBishopMoves(data, whitePieces, blackPieces, player, pos);
            return queenMoves & ~*ownPieces;
        }

        boardType getCastlingMoves(preCalculation::preCalc &preCalculatedData, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t kingPos) {
            // Will check legality too, as castle can't pass through check
            boardType castlingMoves = boardType(0);
            boardType allPieces = boardType(0);
            map<uint8_t, boardType> enemyMoves = getNextMoves(preCalculatedData, !player, true);
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
                    castlingMoves.set(kingPos - 3);
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
                    castlingMoves.set(kingPos + 2);
                }
            }
            return castlingMoves;
        }

        void parseCastlingRights(int &index) {
            // Convert String(KQkQ) to array
            if (fen[index] != '-') {
                for (;fen[index] != ' '; index++) {
                    switch (fen[index]) {
                        case 'K':
                            castlingRights[WHITE][0] = true;
                            break;
                        case 'Q':
                            castlingRights[WHITE][1] = true;
                            break;
                        case 'k':
                            castlingRights[BLACK][0] = true;
                            break;
                        case 'q':
                            castlingRights[BLACK][1] = true;
                    }
                }
            }
        }

        uint16_t parseIntInFEN(int &index) {
            uint16_t num = 0;
            for (; fen[index] != ' '; index++) {
                num *= 10;
                num += fen[index] - '0';
            }
            return num;
        }

    public:
        short int enPassantSquare;
        bool castlingRights[2][2]; // [color][side] Queen -> 0, King -> 1
        playerType player;
        uint8_t halfMoves;
        uint16_t fullMoves;

        static uint8_t parseNotation(string notation) {
            return (7 - (notation[1] - '1')) * 8 + notation[0] - 'a';
        }

        static string getNotation(int pos) {
            string out;
            out.push_back((pos % 8 + 'a'));
            out.push_back((7 - pos / 8) + '1');
            return out;
        }

        bool getCastlingRights(int color, int side) {
            return castlingRights[color][side];
        }

        static playerType getPlayer(char piece) {
            if (piece > 'a' && piece < 'z')
                return BLACK;
            return WHITE;
        }

        char pieceAt(uint8_t pos) {
            return this->_board[pos];
        }

        void parseFen() {
            for (uint8_t j=0; j < 64; j++) {
                _board[j] = ' ';
            }
            int i, boardIndex=0;
            for (i=0; fen[i] != ' '; i++) {
                if (fen[i] == '/')
                    continue;
                if (fen[i] > '0' && fen[i] < '9') {
                    // Number of empty squares
                    boardIndex += fen[i] - '0';
                } else {
                    // Piece
                    _board[boardIndex] = fen[i];
                    boardIndex++;
                }
            }
            player = fen[++i] == 'b' ? BLACK : WHITE;
            i += 2; // Shift to castling
            parseCastlingRights(i);
            i++;  // Eat Space
            if (fen[i] != '-') {
                enPassantSquare = parseNotation(fen.substr(i, 2));
                i += 2;
            } else {
                enPassantSquare = -1;
                i++; // Eat Space
            }
            halfMoves = (uint8_t) parseIntInFEN(i);
            i++; // Eat Space
            fullMoves = parseIntInFEN(i);
        }

        Board(string _fen) {
            fen = _fen;
            parseFen();
        }

        Board(Board &original) {
            this->fen = original.fen;
            parseFen();
        }

        map<uint8_t, boardType> getNextMoves(preCalculation::preCalc &preCalculatedData, playerType player, bool quick = false) {
            // PSUEDO LEGAL ONLY, DOES NOT CHECK FOR LEGALITY
            boardType whitePieces = boardType(0), blackPieces = boardType(0);
            uint8_t kingPos;
            map<uint8_t, boardType> moves = map<uint8_t, boardType>();
            vector<uint8_t> playerSquares = vector<uint8_t>();
            for (uint8_t i=0; i < 64; i++) {
                if (_board[i] == ' ')
                    continue;
                if (getPlayer(_board[i]) == WHITE)
                    whitePieces.set(i);
                else
                    blackPieces.set(i);
                if (getPlayer(_board[i]) == player)
                    playerSquares.push_back(i);
            }

            for (auto i=playerSquares.begin(); i != playerSquares.end(); i++) {
                char piece = tolower(_board[*i]);
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

        uint8_t findKing(playerType player) {
            char target = player == BLACK ? 'k' : 'K';
            if (_board[4] == target) {
                return 4;
            }
            if (_board[60] == target) {
                return 60;
            }
            for (int i=0; i < 64; i++) {
                if (_board[i] == target) {
                    return i;
                }
            }
            // Should never reach this
            return -1;
        }

        string getFen() {
            return fen;
        }

        string exportFEN() {
            string newFen = "";
            int blankSpaces = 0;
            for (uint8_t i=0; i < 64; i++) {
                if (_board[i] != ' ') {
                    if (blankSpaces > 0) {
                        newFen.push_back('0' + blankSpaces);
                    }
                    newFen.push_back(_board[i]);
                    blankSpaces = 0;
                } else {
                    blankSpaces++;
                }
                if (i % 8 == 7 && i != 63){
                    if (blankSpaces > 0) {
                        newFen.push_back('0' + blankSpaces);
                    }
                    newFen.push_back('/');
                    blankSpaces = 0;
                }
            }
            
            newFen += player == BLACK ? " b "  : " w ";

            bool isCastleAvailable = false;
            string castleString = "";
            if (castlingRights[WHITE][0]) {
                castleString.push_back('K');
                isCastleAvailable = true;
            }
            if (castlingRights[WHITE][1]) {
                castleString.push_back('Q');
                isCastleAvailable = true;
            }
            if (castlingRights[BLACK][0]) {
                castleString.push_back('k');
                isCastleAvailable = true;
            }
            if (castlingRights[BLACK][1]) {
                castleString.push_back('q');
                isCastleAvailable = true;
            }
            if (!isCastleAvailable)
                castleString.push_back('-');
            
            newFen += castleString + " ";
            newFen += enPassantSquare != -1 ? getNotation(enPassantSquare) : "-";
            newFen += " ";

            newFen += std::to_string(halfMoves);
            newFen += " ";
            newFen += std::to_string(fullMoves);

            return newFen;
        }

        void makeMove(array<uint8_t, 2> move, bool changeFEN=true) {
            // Does no validation, make sure move is psuedo legal beforehand
            int castleRookInitialPos = -1;
            int castleRookFinalPos = -1;
            bool isHalfMove = _board[move[1]] == ' ';
            
            // Castling
            if (tolower(_board[move[0]]) == 'k' ) {
                if (abs(move[0] - move[1]) == 2) {
                    if (player == WHITE) {
                        _board[61] = _board[63];
                        _board[61] = ' ';
                    } else {
                        _board[5] = _board[7];
                        _board[7] = ' ';
                    }
                } else if (abs(move[0] - move[1]) == 3) {
                    if (player == WHITE) {
                        _board[58] = _board[56];
                        _board[56] = ' ';
                    } else {
                        _board[2] = _board[0];
                        _board[0] = ' ';
                    }
                }
                castlingRights[player][0] = false;
                castlingRights[player][1] = false;
            }
            
            if (tolower(_board[move[0]]) == 'p') {
                isHalfMove = false;
                // En Passant
                if (move[1] == enPassantSquare) {
                    _board[enPassantSquare - getDirection(player) * 8] = ' ';
                }
                enPassantSquare = -1;
                if (abs(move[0] - move[1]) / 8 == 2) {       
                    enPassantSquare = move[1] - getDirection(player) * 8;
                }
            } else {
                enPassantSquare = -1;
            }

            if (tolower(_board[move[0]]) == 'r') {
                if (move[0] == 0 && player == WHITE) {
                    castlingRights[WHITE][0] = false;
                } else if (move[0] == 7 && player == WHITE) {
                    castlingRights[WHITE][1] = false;
                } else if (move[0] == 56 && player == BLACK) {
                    castlingRights[BLACK][0] = false;
                } else if (move[0] == 63 && player == BLACK) {
                    castlingRights[BLACK][1] = false;
                }
            }
            
            _board[move[1]] = _board[move[0]];
            _board[move[0]] = ' ';

            if (player == BLACK) {
                fullMoves++;
            }

            player = !player;
            
            if (isHalfMove) {
                halfMoves++;
            } else {
                halfMoves = 0;
            }

            if (changeFEN) {
                fen = exportFEN();
            }
        }

        bool isValidMove(map<uint8_t, boardType> &moves, array<uint8_t, 2> move, preCalculation::preCalc &preCalculatedData) {
            boardType availableMoves = moves[move[0]];

            if (!availableMoves[move[1]]) {
                return false;
            }

            Board newBoard(*this);
            newBoard.makeMove(move, false);

            uint8_t kingPos = newBoard.findKing(this->player);
            map<uint8_t, boardType> enemyMoves = newBoard.getNextMoves(preCalculatedData, newBoard.player, true);
            for (auto itr = enemyMoves.begin(); itr != enemyMoves.end(); itr++) {
                if (itr->second[kingPos]) {
                    return false;
                }
            }
            return true;
        }

        bool isValidMove(array<uint8_t, 2> move, preCalculation::preCalc &preCalculatedData) {
            map<uint8_t, boardType> nextMoves = this->getNextMoves(preCalculatedData, !this->player, true);
            return this->isValidMove(nextMoves, move, preCalculatedData);
        }
};
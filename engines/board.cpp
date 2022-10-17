#include<fstream>
#include<string>
#include<bitset>
#include<iterator>
#include<map>
#include<vector>
#include<chrono>

#include "definitions.hpp"
#include "preCalculation.hpp"
#include "board.hpp"
#include "log.hpp"

using std::string;
using std::array;
using std::map;
using std::vector;

short int Board::getDirection(playerType player) {
    return player == WHITE ? -1 : 1;
}

boardType Board::getPawnMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea) {
    boardType allPieces = whitePieces | blackPieces;
    boardType nonCaptures = boardType(0);
    boardType captures = boardType(0);
    boardType enemyPieces = (player == BLACK) ? whitePieces : blackPieces;
    
    if (this->enPassantSquare != INVALID_POS)
        enemyPieces.set(this->enPassantSquare);

    // Normal Move
    uint8_t nextMove = pos + getDirection(player) * 8;
    if (!(allPieces[nextMove]) && nextMove < 64) {
        nonCaptures.set(nextMove);
    }

    // Double Move at initial position
    nextMove = pos + getDirection(player) * 16;
    uint8_t intermediateSquare = pos + getDirection(player) * 8;
    if (player == BLACK && pos / 8 == 1 && !allPieces[intermediateSquare] && !allPieces[nextMove] && nextMove < 64) {
        nonCaptures.set(nextMove);
    } else if (player == WHITE && pos / 8 == 6 && !allPieces[intermediateSquare] && !allPieces[nextMove] && nextMove < 64) {
        nonCaptures.set(nextMove);
    }

    // Capture Left
    nextMove = pos + getDirection(player) * 8 - 1;
    if (pos % 8 != 0 && nextMove < 64) {
        captures.set(nextMove);
    }
    // Capture Right
    nextMove = pos + getDirection(player) * 8 + 1;
    if (pos % 8 != 7 && nextMove < 64) {
        captures.set(nextMove);
    }
    if (isAttackArea) {
        return captures;
    }
    captures &= enemyPieces;
    nonCaptures &= ~allPieces;
    return captures | nonCaptures;
}

boardType Board::getKnightMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea) {
    boardType* ownPieces = (player == BLACK) ? &blackPieces : &whitePieces;;
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
    if (isAttackArea) {
        return movesBoard;
    }    
    movesBoard &= ~*ownPieces;
    
    return movesBoard;
}

boardType Board::getKingMoves(boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea) {
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
            movesBoard.set(pos - 9);
        if (pos / 8 != 7)
            movesBoard.set(pos + 7);
    }

    if (pos % 8 != 7) {
        movesBoard.set(pos + 1);
        if (pos / 8 != 0)
            movesBoard.set(pos - 7);
        if (pos / 8 != 7)
            movesBoard.set(pos + 9);
    }
    if (isAttackArea) {
        return movesBoard;
    }
    return movesBoard & ~*ownPieces;
}

uint16_t Board::getMagicHash(boardType &board, unsigned long long magic, boardType& blockerMask, bool isBishop) {
    // Mask board with blockers to get relevant squares
    return ((board & blockerMask).to_ullong() * magic) >> (64 - (isBishop ? 9 : 12));
}

boardType Board::getBishopMoves(preCalculation::preCalcType data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea) {
    boardType allPieces = whitePieces | blackPieces;
    boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
    uint16_t index = getMagicHash(allPieces, data->bishopMagic[pos], data->bishopBlockers[pos], true);
    boardType allMoves = data->bishopLookup[pos][index];
    if (isAttackArea) {
        return allMoves;
    }
    return allMoves & ~*ownPieces;
}

boardType Board::getRookMoves(preCalculation::preCalcType data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea) {
    boardType allPieces = whitePieces | blackPieces;
    boardType* ownPieces = (player == WHITE) ? &whitePieces : &blackPieces;
    uint16_t index = getMagicHash(allPieces, data->rookMagic[pos], data->rookBlockers[pos], false);
    boardType allMoves = data->rookLookup[pos][index];
    if (isAttackArea) {
        return allMoves;
    }
    return allMoves & ~*ownPieces;
}

boardType Board::getQueenMoves(preCalculation::preCalcType data, boardType &whitePieces, boardType &blackPieces, playerType player, uint8_t pos, bool isAttackArea) {
    return getRookMoves(data, whitePieces, blackPieces, player, pos, isAttackArea) | getBishopMoves(data, whitePieces, blackPieces, player, pos, isAttackArea);
}

boardType Board::getCastlingMoves(preCalculation::preCalcType preCalculatedData, uint8_t kingPos, bool isLeftClear, bool isRightClear) {
    // Will check legality too, as castle can't pass through check

    boardType castlingMoves = boardType(0);
    boardType enemyAttacks = boardType(0);
    map<uint8_t, boardType> enemyMoves = getNextMoves(preCalculatedData, !player, true);

    for (auto itr = enemyMoves.begin(); itr != enemyMoves.end() && (isLeftClear || isRightClear); itr++) {
        enemyAttacks |= itr->second;
        if (enemyAttacks[kingPos]) {
            // King can't be in check
            return boardType(0);
        }
        if (enemyAttacks[kingPos - 1] || enemyAttacks[kingPos - 2] || enemyAttacks[kingPos - 3]) {
            isLeftClear = false;
        } 
        if (enemyAttacks[kingPos + 1] || enemyAttacks[kingPos + 2]) {
            isRightClear = false;
        }
    }

    if (castlingRights[player][0] && isLeftClear) {
        castlingMoves.set(kingPos - 3);
    }
    if (castlingRights[player][1] && isRightClear) {
        castlingMoves.set(kingPos + 2);
    }
    return castlingMoves;
}

void Board::calcZobristHash(prnType &PRN) {
    zobristHash = 0;
    for (uint8_t i=0; i < 64; i++) {
        if (_board[i] == ' ') {
            continue;
        }
        char piece = tolower(_board[i]);
        if (piece == 'p') {
            zobristHash ^= PRN[prnPawnValStart + getPlayer(_board[i]) * i];
        } else if (piece == 'n') {
            zobristHash ^= PRN[prnKnightValStart + getPlayer(_board[i]) * i];
        } else if (piece == 'b') {
            zobristHash ^= PRN[prnBishopValStart + getPlayer(_board[i]) * i];
        } else if (piece == 'r') {
            zobristHash ^= PRN[prnRookValStart + getPlayer(_board[i]) * i];
        } else if (piece == 'q') {
            zobristHash ^= PRN[prnQueenValStart + getPlayer(_board[i]) * i];
        } else if (piece == 'k') {
            zobristHash ^= PRN[prnKingValStart + getPlayer(_board[i]) * i];
        }
    }

    if (player == BLACK)
        zobristHash ^= PRN[prnBoardEnds];

    zobristHash ^= PRN[prnBoardEnds + (uint8_t)castlingRights[0][0]];

    zobristHash ^= PRN[prnBoardEnds + 2 + (uint8_t)castlingRights[0][1]];

    zobristHash ^= PRN[prnBoardEnds + 4 + (uint8_t)castlingRights[1][0]];

    zobristHash ^= PRN[prnBoardEnds + 6 + (uint8_t)castlingRights[1][1]];
    
    if (enPassantSquare != INVALID_POS) {
        zobristHash ^= PRN[prnEnPassantStart + enPassantSquare % 8];
    }
}

void Board::parseCastlingRights(int &index) {
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
    } else {
        // Eat -
        index++;
    }
}

uint16_t Board::parseIntInFEN(int &index) {
    uint16_t num = 0;
    for (; fen[index] != ' '; index++) {
        num *= 10;
        num += fen[index] - '0';
    }
    return num;
}

uint8_t Board::parseNotation(string notation) {
    return (7 - (notation[1] - '1')) * 8 + notation[0] - 'a';
}

string Board::getNotation(int pos) {
    string out;
    out.push_back((pos % 8 + 'a'));
    out.push_back((7 - pos / 8) + '1');
    return out;
}

playerType Board::getPlayer(char piece) {
    if (piece > 'a' && piece < 'z')
        return BLACK;
    return WHITE;
}

char Board::pieceAt(uint8_t pos) {
    return this->_board[pos];
}

void Board::parseFen() {
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
        enPassantSquare = INVALID_POS;
        i++; // Eat Space
    }
    halfMoves = (uint8_t) parseIntInFEN(i);
    i++; // Eat Space
    fullMoves = parseIntInFEN(i);
}

Board::Board(string _fen, prnType& PRN) {
    fen = _fen;
    calcZobristHash(PRN);
    parseFen();
}

Board::Board(Board &original) {
    this->fen = original.fen;
    this->zobristHash = original.zobristHash;
    parseFen();
}

bool Board::isCapture(moveType move) {
    return this->pieceAt(move[1]) != ' ';
}

map<uint8_t, boardType> Board::getNextMoves(preCalculation::preCalcType preCalculatedData, playerType player, bool quick) {
    // PSUEDO LEGAL ONLY, DOES NOT CHECK FOR LEGALITY
    boardType whitePieces = boardType(0), blackPieces = boardType(0);
    // King not found intially
    uint8_t kingPos = INVALID_POS;
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
    if (quick || kingPos == INVALID_POS) {
        return moves;
    }

    // Saves ~10us of function calling overhead if preliminary checks are done here
    boardType allPieces = whitePieces | blackPieces;
    bool isLeftClear = allPieces[kingPos - 1] && allPieces[kingPos - 2] && allPieces[kingPos - 3];
    bool isRightClear = allPieces[kingPos + 1] && allPieces[kingPos + 2];
    bool isCastlePossible = !castlingRights[player][0] && !castlingRights[player][1] 
        && (player == WHITE && kingPos == 4 || player == BLACK && kingPos == 60)
        && (isLeftClear || isRightClear);

    if (isCastlePossible) {
        moves[kingPos] |= getCastlingMoves(preCalculatedData, kingPos, isLeftClear, isRightClear);
    }
    return moves;
}

boardType Board::getAttackArea(preCalculation::preCalcType preCalculatedData, playerType player) {
    boardType whitePieces = boardType(0), blackPieces = boardType(0), attackArea = boardType(0);
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
    for (auto &square: playerSquares) {
        char piece = tolower(_board[square]);
        if (piece == 'p') {
           attackArea |= getPawnMoves(whitePieces, blackPieces, player, square, true);
        } else if (piece == 'n') {
            attackArea |= getKnightMoves(whitePieces, blackPieces, player, square, true);
        } else if (piece == 'b') {
            attackArea |= getBishopMoves(preCalculatedData, whitePieces, blackPieces, player, square, true);
        } else if (piece == 'r') {
            attackArea |= getRookMoves(preCalculatedData, whitePieces, blackPieces, player, square, true);
        } else if (piece == 'q') {
            attackArea |= getQueenMoves(preCalculatedData, whitePieces, blackPieces, player, square, true);
        } else if (piece == 'k') {
            attackArea |= getKingMoves(whitePieces, blackPieces, player, square, true);
        }
    }
    return attackArea;
}

vector<moveType> Board::orderedNextMoves(preCalculation::preCalcType preCalculatedData, playerType player) {
    map<uint8_t, boardType> nextMoves = this->getNextMoves(preCalculatedData, player, false);
    vector<moveType> captures;
    vector<moveType> nonCaptures;

    for (auto itr = nextMoves.begin(); itr != nextMoves.end(); itr++) {
        for (uint8_t i = itr->second._Find_first(); i < 64; i = itr->second._Find_next(i)) {
            moveType currentMove = {itr->first, i};
            if (isCapture(currentMove)) {
                captures.push_back(currentMove);
            } else {
                nonCaptures.push_back(currentMove);
            }
        }
    }
    // Add non captures to end of captures
    captures.insert(captures.end(), nonCaptures.begin(), nonCaptures.end());
    return captures;
}

uint8_t Board::findKing(playerType player) {
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

    return INVALID_POS;
}

unsigned short Board::getZobristStartIndex(char piece) {
    piece = tolower(piece);
    if (piece == 'p') {
        return prnPawnValStart;
    } else if (piece == 'n') {
        return prnKnightValStart;
    } else if (piece == 'b') {
        return prnBishopValStart;
    } else if (piece == 'r') {
       return prnRookValStart;
    } else if (piece == 'q') {
        return prnQueenValStart;
    }
    // else
    return prnKingValStart;
}

void Board::changeZobristOnMove(prnType &PRN, moveType &move) {
    if (move[1] == INVALID_POS) {
        return;
    }

    unsigned short startIndex = getZobristStartIndex(_board[move[0]]);
    if (_board[move[1]] != ' ') {
        unsigned short enemyPieceStartIndex = getZobristStartIndex(_board[move[0]]);
        // Remove piece from board
        this->zobristHash ^= PRN[enemyPieceStartIndex + player * move[1]];
    }

    // Remove piece from START
    this->zobristHash ^= PRN[startIndex + player * move[0]];
    // Add piece to END
    this->zobristHash ^= PRN[startIndex + player * move[1]];
}

void Board::changeZobristOnPromotion(prnType &PRN, char piece, uint8_t pos) {
    unsigned short startIndex = getZobristStartIndex(piece);
    this->zobristHash ^= PRN[startIndex + player * pos];
}

void Board::disableQueenSideCastling(prnType &PRN) {
    if (castlingRights[player][0]) {
        // Enable false
        this->zobristHash ^= PRN[prnBoardEnds + 1 + 4 * player];
        // Disable true
        this->zobristHash ^= PRN[prnBoardEnds + 1 + 4 * player + 1];
    }
    castlingRights[player][0] = false;
}

void Board::disableKingSideCastling(prnType &PRN) {
    if (castlingRights[player][1]) {
        // Enable false
        this->zobristHash ^= PRN[prnBoardEnds + 1 + 4 * player + 2];
        // Disable true
        this->zobristHash ^= PRN[prnBoardEnds + 1 + 4 * player + 3];
    }
    castlingRights[player][1] = false;
}

string Board::exportFEN() {
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
    newFen += enPassantSquare != INVALID_POS ? getNotation(enPassantSquare) : "-";
    newFen += " ";

    newFen += std::to_string(halfMoves);
    newFen += " ";
    newFen += std::to_string(fullMoves);

    return newFen;
}

void Board::makeMove(moveType move, prnType &PRN, bool isComputer, bool changeFEN) {
    // Does no validation, make sure move is psuedo legal beforehand
    bool isHalfMove = _board[move[1]] == ' ';
    bool isPromotion = false;

    // Castling
    if (tolower(_board[move[0]]) == 'k') {
        moveType rookMove = {INVALID_POS, INVALID_POS};
        if (abs(move[0] - move[1]) == 2) {
            if (player == WHITE) {
                _board[61] = _board[63];
                _board[61] = ' ';
                rookMove = {63, 61};
            } else {
                _board[5] = _board[7];
                _board[7] = ' ';
                rookMove = {7, 5};
            }
        } else if (abs(move[0] - move[1]) == 3) {
            if (player == WHITE) {
                _board[58] = _board[56];
                _board[56] = ' ';
                rookMove = {56, 58};
            } else {
                _board[2] = _board[0];
                _board[0] = ' ';
                rookMove = {0, 2};
            }
        }

        changeZobristOnMove(PRN, rookMove);
        disableQueenSideCastling(PRN);
        disableKingSideCastling(PRN);
    }

    if (enPassantSquare != INVALID_POS) {
        // Disable enPassant on last file
        this->zobristHash ^= PRN[prnEnPassantStart + enPassantSquare % 8];
    }

    if (tolower(_board[move[0]]) == 'p') {
        if ((move[1] / 8 == 0 || move[1] / 8 == 7) && isComputer) {
            if (player == WHITE) {
                _board[move[0]] = 'Q';
            } else {
                _board[move[0]] = 'q';
            }
            changeZobristOnPromotion(PRN, 'q', move[1]);
            isPromotion = true;
        }
        isHalfMove = false;
        // En Passant
        if (move[1] == enPassantSquare) {
            uint8_t pos = enPassantSquare - getDirection(player) * 8;
            _board[pos] = ' ';
            // Remove piece from hash
            this->zobristHash ^= PRN[getZobristStartIndex('p') + pos * player];
        }
        enPassantSquare = INVALID_POS;
        if (abs(move[0] - move[1]) / 8 == 2) {
            enPassantSquare = move[1] - getDirection(player) * 8;
            // Enable enPassant on file
            this->zobristHash ^= PRN[prnEnPassantStart + enPassantSquare % 8];
        }
    } else {
        enPassantSquare = INVALID_POS;
    }

    if (tolower(_board[move[0]]) == 'r') {
        if (move[0] == 0 && player == WHITE || move[0] == 56 && player == BLACK) {
            disableQueenSideCastling(PRN);
        } else if (move[0] == 7 && player == WHITE || move[0] == 63 && player == BLACK) {
            disableKingSideCastling(PRN);
        }
    }

    if (isPromotion) {
        // Remove pawn from board
        this->zobristHash ^= PRN[getZobristStartIndex('p') + move[0] * player];
    } else {
        changeZobristOnMove(PRN, move);
    }
    _board[move[1]] = _board[move[0]];
    _board[move[0]] = ' ';

    if (player == BLACK) {
        fullMoves++;
    }
    
    player = !player;
    this->zobristHash ^= PRN[prnBoardEnds];

    if (isHalfMove) {
        halfMoves++;
    } else {
        halfMoves = 0;
    }

    if (changeFEN) {
        fen = exportFEN();
    }
    
    unsigned long long prevHash = zobristHash;
    calcZobristHash(PRN);
    if (prevHash != zobristHash) {
        logging::v("ZobristError", move[0]);
        logging::v("ZobristError", move[1]);
        logging::v("ZobristError", "FUCKED");
    }
}

void Board::makeMove(const string& move, prnType &PRN, bool isComputer, bool changeFEN) {
    // Make move in chess notation
    moveType moveType = {parseNotation(move.substr(0, 2)), parseNotation(move.substr(2, 2))};
    makeMove(moveType, PRN, isComputer, changeFEN);
}

bool Board::isInCheck(preCalculation::preCalcType &preCalc, playerType player) {
    // Check if player is in check
    uint8_t kingPos = findKing(player);
    if (kingPos == INVALID_POS) {
        logging::e("KingError", "King not found");
        return false;
    }
    return getAttackArea(preCalc, !player).test(kingPos);
}

bool Board::isValidMove(map<uint8_t, boardType> &moves, array<uint8_t, 2> move, preCalculation::preCalcType preCalculatedData) {
    boardType availableMoves = moves[move[0]];

    if (!availableMoves[move[1]]) {
        return false;
    }

    Board newBoard(*this);
    newBoard.makeMove(move, preCalculatedData->PRN, false, false);

    uint8_t kingPos = newBoard.findKing(this->player);
    if (kingPos < 64){
        map<uint8_t, boardType> enemyMoves = newBoard.getNextMoves(preCalculatedData, newBoard.player, true);
        for (auto itr = enemyMoves.begin(); itr != enemyMoves.end(); itr++) {
            if (itr->second[kingPos]) {
                return false;
            }
        }
    }
    return true;
}

bool Board::isValidMove(array<uint8_t, 2> move, preCalculation::preCalcType preCalculatedData) {
    map<uint8_t, boardType> nextMoves = this->getNextMoves(preCalculatedData, !this->player, true);
    return this->isValidMove(nextMoves, move, preCalculatedData);
}
#ifndef CHESS_DEFINITIONS
#define CHESS_DEFINITIONS 1

#include<bitset>
#include<array>

#define MAX_SCORE 9999999
#define MIN_SCORE -9999999
#define BLACK 0
#define WHITE 1
#define INVALID_POS 64
#define CACHE_SIZE 100000

typedef std::bitset<64> boardType;
typedef uint8_t playerType;
typedef std::array<uint8_t, 2> moveType;

/*
Random Num Array

1 number for each piece at each square, since pawns don't occupy the last rank(10 * 64 + 2 * 56)
1 number to indicate the side to move is black
8 numbers to indicate the castling rights(4 values, each can be 1 or 0, therefore 2*4)
8 numbers to indicate the file of a valid En passant square, if any
*/
typedef std::array<unsigned long long, 64 * 10 + 56 * 2 + 1 + 8 + 8> prnType;

// 64 * 2 increments for each player
const unsigned short prnKnightValStart = 0;

const unsigned short prnBishopValStart = 128;

const unsigned short prnRookValStart = 256;

const unsigned short prnQueenValStart = 384;

const unsigned short prnKingValStart = 512;

const unsigned short prnPawnValStart = 640;

const unsigned short prnBoardEnds = 64 * 10 + 56 * 2;

const unsigned short prnEnPassantStart = prnBoardEnds + 1 + 8; 

#endif
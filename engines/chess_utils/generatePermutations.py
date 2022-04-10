import json
from itertools import combinations
import logging
from random import randint

def get_uu64(positions):
    return int("0b" + "".join([str(int(i in positions)) for i in range(63, -1, -1)]), 2)

def random_uu64():
    return randint(0, 2**16 - 1) | (randint(0, 2**16 - 1) << 16) | (randint(0, 2**16 - 1) << 32) | (randint(0, 2**16 - 1) << 48)

def random_uu64_few_bits():
    return random_uu64() & random_uu64() & random_uu64() & random_uu64()

def count_bits(uu64):
    return bin(uu64).count("1")

def transform(board, magic, is_bishop):
    return (magic * board) % 2**64 >> (64 - (9 if is_bishop else 12))

def generate_blockers(origin, directions):
    moves = []

    for [direction_x, direction_y] in directions:
        move = [ origin // 8 + direction_x, origin % 8 + direction_y ]
        new_moves = []
        while move[0] >= 0 and move[0] <= 7 and move[1] >= 0 and move[1] <= 7:
            new_moves.append(move[0] * 8 + move[1])
            move = [ move[0] + direction_x, move[1] + direction_y ]
        if len(new_moves) > 0:
            new_moves.pop(len(new_moves) - 1)
        
        moves += new_moves
    return moves


def generate_bishop(origin):
    directions = [[-1, 1], [1, 1], [1, -1], [-1, -1]]
    return generate_blockers(origin, directions)

def generate_rook(origin):
    directions = [[-1, 0], [1, 0], [0, -1], [0, 1]]
    return generate_blockers(origin, directions)


def get_attck(origin, blockers, is_bishop):
    directions = [[-1, 1], [1, 1], [1, -1], [-1, -1]] if is_bishop else [[-1, 0], [1, 0], [0, -1], [0, 1]]
    moves = []
    for [direction_x, direction_y] in directions:
        move = [ origin // 8 + direction_x, origin % 8 + direction_y ]
        while move[0] >= 0 and move[0] <= 7 and move[1] >= 0 and move[1] <= 7:
            moves.append(move[0] * 8 + move[1])
            if move[0] * 8 + move[1] in blockers:
                break
            move = [ move[0] + direction_x, move[1] + direction_y ]
    return get_uu64(moves)

def find_magic_for_square(origin, is_bishop):
    blockers = bishop_blockers[origin] if is_bishop else rook_blockers[origin]
    for _ in range(10000000):
        magic = random_uu64_few_bits()
        used = [False for __ in range(4096)]
        fail = False
        for i in range(len(blockers)):
            pos = transform(get_uu64(blockers[i]), magic, is_bishop)
            if used[pos]:
                logging.debug(f"Failed {_} iteration at {i}")
                fail = True
                break
            used[pos] = True
        if not fail:
            return magic

bishop_blockers = []
rook_blockers = []

# logging.getLogger().setLevel(logging.INFO)
# Uncomment to show progress
logging.getLogger().setLevel(logging.DEBUG)

bishops = [[None for _ in range(512)] for __ in range(64)]
rooks = [[None for _ in range(4096)] for __ in range(64)]

def generate_all_blockers():
    for i in range(64):
        b = generate_bishop(i)
        r = generate_rook(i)
    
        blockers = []
        for j in range(len(b) + 1):
            blockers += list(combinations(b, j))
        bishop_blockers.append(blockers)
        blockers = []
        
        for j in range(len(r) + 1):
            blockers += list(combinations(r, j))
        rook_blockers.append(blockers)


bishop_magics = []
rooks_magics = []

generate_all_blockers()
for i in range(64):
    bm = find_magic_for_square(i, True)
    rm = find_magic_for_square(i, False)
    for x in bishop_blockers[i]:
        pos = transform(get_uu64(x), bm, True)
        bishops[i][pos] = get_attck(i, x, True)
    for y in rook_blockers[i]:
        pos = transform(get_uu64(y), rm, False)
        rooks[i][pos] = get_attck(i, y, False)
    
    bishop_magics.append(bm)
    rooks_magics.append(rm)

with open("magics.json", "w") as f:
    json.dump({"bishop": bishop_magics, "rook": rooks_magics}, f)

with open("attacks.json", "w") as f:
    json.dump({"bishop": bishops, "rook": rooks}, f)

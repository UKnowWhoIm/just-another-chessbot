import json

def get_uu64(positions):
    return int("0b" + "".join([str(int(i in positions)) for i in range(63, -1, -1)]), 2)


def print_board(board):
    bits = bin(board)[2:]
    bits = (64 - len(bits)) * '0' + bits
    for i in range(8):
        for j in range(8):
            print(bits[63 - (i * 8 + j)], end=" ")
        print()


def get_abs(coord):
    return coord[0] * 8 + coord[1]


def get_block(origin, directions):
    positions = []
    for [x, y] in directions:
        move = [origin // 8 + x, origin % 8 + y]
        moves = []
        while 0 <= move[0] <= 7 and 0 <= move[1] <= 7:
            moves.append(get_abs(move))
            move = [move[0] + x, move[1] + y]
        if len(moves) != 0:
            moves = moves[:-1]
        positions += moves
    return get_uu64(positions)

def get_rook(origin):
    directions = [[-1, 0], [1, 0], [0, -1], [0, 1]]
    return get_block(origin, directions)


def get_bishop(origin):
    directions = [[-1, -1], [1, 1], [1, -1], [-1, 1]]
    return get_block(origin, directions)

bishop_blockers = []
rook_blockers = []

for i in range(64):
    bishop_blockers.append(get_bishop(i))
    rook_blockers.append(get_rook(i))

with open("blockers.json", "w") as f:
    json.dump({"bishop": bishop_blockers, "rook": rook_blockers}, f)

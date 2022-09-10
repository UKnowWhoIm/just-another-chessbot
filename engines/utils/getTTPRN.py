from secrets import randbits

"""
1 number for each piece at each square, since pawns don't occupy the last rank(10 * 64 + 2 * 56)
1 number to indicate the side to move is black
8 numbers to indicate the castling rights(4 values, each can be 1 or 0, therefore 2*4)
8 numbers to indicate the file of a valid En passant square, if any
"""
count = 64*10 + 56*2 + 1 + 4 + 8

nums = []

i = 0
while i < count:
    new_num = randbits(64)
    if new_num in nums:
        print("WARNING: PRN not good enough")
        continue
    nums.append(new_num)
    i += 1


with open("PRN", "w") as f:
    f.write(" ".join([str(i) for i in nums]))
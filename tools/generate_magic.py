#!/usr/bin/env python3

import argparse
import random
import sys

# Bitboard utilities
def pop_bit(bb):
    lsb = bb & -bb
    return bb ^ lsb, lsb.bit_length() - 1

def popcount(bb):
    return bin(bb).count('1')

def get_rook_mask(sq):
    mask = 0
    r, f = sq // 8, sq % 8
    for i in range(r+1, 7): mask |= 1 << (i*8 + f)
    for i in range(r-1, 0, -1): mask |= 1 << (i*8 + f)
    for i in range(f+1, 7): mask |= 1 << (r*8 + i)
    for i in range(f-1, 0, -1): mask |= 1 << (r*8 + i)
    return mask

def get_bishop_mask(sq):
    mask = 0
    r, f = sq // 8, sq % 8
    for i in range(1, min(7-r, 7-f)): mask |= 1 << ((r+i)*8 + f+i)
    for i in range(1, min(7-r, f)): mask |= 1 << ((r+i)*8 + f-i)
    for i in range(1, min(r, 7-f)): mask |= 1 << ((r-i)*8 + f+i)
    for i in range(1, min(r, f)): mask |= 1 << ((r-i)*8 + f-i)
    return mask

def get_rook_attacks(sq, blockers):
    attacks = 0
    r, f = sq // 8, sq % 8
    # North
    for i in range(r+1, 8):
        s = i*8 + f
        attacks |= 1 << s
        if blockers & (1 << s): break
    # South
    for i in range(r-1, -1, -1):
        s = i*8 + f
        attacks |= 1 << s
        if blockers & (1 << s): break
    # East
    for i in range(f+1, 8):
        s = r*8 + i
        attacks |= 1 << s
        if blockers & (1 << s): break
    # West
    for i in range(f-1, -1, -1):
        s = r*8 + i
        attacks |= 1 << s
        if blockers & (1 << s): break
    return attacks

def get_bishop_attacks(sq, blockers):
    attacks = 0
    r, f = sq // 8, sq % 8
    # NE
    for i in range(1, 8):
        nr, nf = r+i, f+i
        if nr > 7 or nf > 7: break
        s = nr*8 + nf
        attacks |= 1 << s
        if blockers & (1 << s): break
    # NW
    for i in range(1, 8):
        nr, nf = r+i, f-i
        if nr > 7 or nf < 0: break
        s = nr*8 + nf
        attacks |= 1 << s
        if blockers & (1 << s): break
    # SE
    for i in range(1, 8):
        nr, nf = r-i, f+i
        if nr < 0 or nf > 7: break
        s = nr*8 + nf
        attacks |= 1 << s
        if blockers & (1 << s): break
    # SW
    for i in range(1, 8):
        nr, nf = r-i, f-i
        if nr < 0 or nf < 0: break
        s = nr*8 + nf
        attacks |= 1 << s
        if blockers & (1 << s): break
    return attacks

def generate_blockers(mask):
    blockers = []
    bits = []
    m = mask
    while m:
        m, bit = pop_bit(m)
        bits.append(bit)
    n = len(bits)
    for i in range(1 << n):
        b = 0
        for j in range(n):
            if i & (1 << j):
                b |= 1 << bits[j]
        blockers.append(b)
    return blockers

def find_magic(sq, mask, attacks_func, is_rook):
    blockers = generate_blockers(mask)
    n = len(blockers)
    shifts = 64 - popcount(mask)
    used = [0] * (1 << popcount(mask))
    magic = 0
    while True:
        magic = random.randint(0, 2**64 - 1)
        if popcount((magic * mask) & 0xFF00000000000000) < 6: continue
        used = [-1] * (1 << popcount(mask))
        fail = False
        for b in blockers:
            idx = ((b * magic) >> shifts) & ((1 << popcount(mask)) - 1)
            attacks = attacks_func(sq, b)
            if used[idx] == -1:
                used[idx] = attacks
            elif used[idx] != attacks:
                fail = True
                break
        if not fail:
            return magic

def main():
    parser = argparse.ArgumentParser(description='Generate magic bitboard constants')
    parser.add_argument('--output', required=True, help='Output header file')

    args = parser.parse_args()

    rook_magics = []
    bishop_magics = []
    rook_masks = []
    bishop_masks = []
    rook_shifts = []
    bishop_shifts = []

    print("Generating rook magics...")
    for sq in range(64):
        mask = get_rook_mask(sq)
        magic = find_magic(sq, mask, get_rook_attacks, True)
        rook_magics.append(magic)
        rook_masks.append(mask)
        rook_shifts.append(64 - popcount(mask))
        print(f"Rook {sq}: {magic:016x}")

    print("Generating bishop magics...")
    for sq in range(64):
        mask = get_bishop_mask(sq)
        magic = find_magic(sq, mask, get_bishop_attacks, False)
        bishop_magics.append(magic)
        bishop_masks.append(mask)
        bishop_shifts.append(64 - popcount(mask))
        print(f"Bishop {sq}: {magic:016x}")

    with open(args.output, 'w') as f:
        f.write("#pragma once\n\n")
        f.write("#include <cstdint>\n\n")
        f.write("// Rook magic bitboard constants\n")
        f.write("const uint64_t ROOK_MAGICS[64] = {\n")
        for i, m in enumerate(rook_magics):
            f.write(f"    0x{m:016x}ULL")
            f.write("," if i < 63 else "")
            f.write("\n")
        f.write("};\n\n")
        f.write("const uint64_t ROOK_MASKS[64] = {\n")
        for i, m in enumerate(rook_masks):
            f.write(f"    0x{m:016x}ULL")
            f.write("," if i < 63 else "")
            f.write("\n")
        f.write("};\n\n")
        f.write("const int ROOK_SHIFTS[64] = {\n")
        for i, s in enumerate(rook_shifts):
            f.write(f"    {s}")
            f.write("," if i < 63 else "")
            f.write("\n")
        f.write("};\n\n")
        f.write("// Bishop magic bitboard constants\n")
        f.write("const uint64_t BISHOP_MAGICS[64] = {\n")
        for i, m in enumerate(bishop_magics):
            f.write(f"    0x{m:016x}ULL")
            f.write("," if i < 63 else "")
            f.write("\n")
        f.write("};\n\n")
        f.write("const uint64_t BISHOP_MASKS[64] = {\n")
        for i, m in enumerate(bishop_masks):
            f.write(f"    0x{m:016x}ULL")
            f.write("," if i < 63 else "")
            f.write("\n")
        f.write("};\n\n")
        f.write("const int BISHOP_SHIFTS[64] = {\n")
        for i, s in enumerate(bishop_shifts):
            f.write(f"    {s}")
            f.write("," if i < 63 else "")
            f.write("\n")
        f.write("};\n\n")

    print(f"Generated magics to {args.output}")

if __name__ == '__main__':
    main()
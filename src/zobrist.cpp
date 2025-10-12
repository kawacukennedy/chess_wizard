#include "zobrist.h"

// Initialize the global ZobristKeys object
ZobristKeys Zobrist;
uint64_t ZOBRIST_HISTORY[MAX_PLY];

// SplitMix64 for deterministic key generation as specified
struct SplitMix64 {
    uint64_t state;

    explicit SplitMix64(uint64_t seed) : state(seed) {}

    uint64_t next() {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};

ZobristKeys::ZobristKeys(uint64_t seed) {
    SplitMix64 rng(seed);
    for (int p = 0; p < 12; ++p) {
        for (int sq = 0; sq < 64; ++sq) {
            piece_keys[p][sq] = rng.next();
        }
    }
    for (int i = 0; i < 16; ++i) {
        castling_keys[i] = rng.next();
    }
    // Spec: zobrist_ep_file[8]
    for (int file = 0; file < 8; ++file) {
        en_passant_keys[file] = rng.next();
    }
    side_to_move_key = rng.next();
}

// This function can be called from main to ensure initialization happens.
void init_zobrist_keys(uint64_t seed) {
    // Reinitialize with seed
    Zobrist = ZobristKeys(seed);
}
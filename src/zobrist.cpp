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

// Use a single static RNG instance seeded as per the spec
uint64_t generate_deterministic_key() {
    static SplitMix64 rng(0x9E3779B97F4A7C15ULL);
    return rng.next();
}

ZobristKeys::ZobristKeys() {
    for (int p = 0; p < 12; ++p) {
        for (int sq = 0; sq < 64; ++sq) {
            piece_keys[p][sq] = generate_deterministic_key();
        }
    }
    for (int i = 0; i < 16; ++i) {
        castling_keys[i] = generate_deterministic_key();
    }
    // Spec: zobrist_ep_file[8]
    for (int file = 0; file < 8; ++file) {
        en_passant_keys[file] = generate_deterministic_key();
    }
    side_to_move_key = generate_deterministic_key();
}

// This function can be called from main to ensure initialization happens.
void init_zobrist_keys() {
    // The global Zobrist object's constructor already handles initialization.
    // This function can be a no-op or can be used to explicitly trigger
    // initialization if the global object construction order is a concern.
}
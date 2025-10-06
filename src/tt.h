#ifndef TT_H
#define TT_H

#include "types.h"
#include "move.h"

// Transposition Table Entry
struct TTEntry {
    uint64_t key;     // Zobrist hash key
    uint32_t move;         // Best move found from this position
    int32_t score;    // Evaluation score (centipawns)
    int8_t depth;    // Search depth at which this entry was stored
    uint8_t flags;    // EXACT, LOWERBOUND, UPPERBOUND
    uint8_t age;      // For replacement policy
    uint8_t pad[7]; // For 32-byte alignment
} __attribute__((aligned(64)));

// Transposition Table
class TranspositionTable {
public:
    TranspositionTable();
    ~TranspositionTable();

    void resize(size_t mb_size);
    void clear();
    void increment_age();

    // Probe the TT for an entry
    TTEntry* probe(uint64_t key);

    // Store an entry in the TT
    void store(uint64_t key, uint32_t move, int32_t score, int8_t depth, uint8_t flags);

private:
    TTEntry* table;
    size_t num_entries;
    uint8_t current_age;
};

extern TranspositionTable TT;

#endif // TT_H

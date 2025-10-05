#include "tt.h"
#include <cstring> // For memset

// Global instance of the TranspositionTable
TranspositionTable TT;

TranspositionTable::TranspositionTable() : table(nullptr), num_entries(0), current_age(0) {}

TranspositionTable::~TranspositionTable() {
    delete[] table;
}

void TranspositionTable::resize(size_t mb_size) {
    delete[] table;
    num_entries = (mb_size * 1024 * 1024) / sizeof(TTEntry);
    table = new TTEntry[num_entries];
    clear();
}

void TranspositionTable::clear() {
    if (table) {
        memset(table, 0, num_entries * sizeof(TTEntry));
    }
}

void TranspositionTable::increment_age() {
    current_age++;
}

TTEntry* TranspositionTable::probe(uint64_t key) {
    if (!table) return nullptr;
    TTEntry* entry = &table[key % num_entries];
    if (entry->key == key) {
        return entry;
    }
    return nullptr;
}

void TranspositionTable::store(uint64_t key, uint32_t move, int32_t score, int8_t depth, uint8_t flags) {
    if (!table) return;
    TTEntry* entry = &table[key % num_entries];

    // From spec: "Replace if new.depth > old.depth OR old.age != current_age OR (new.depth == old.depth && key_lowbits xor old_key_lowbits < threshold)"
    // We also replace empty entries.
    const int threshold = 8192; // A chosen value for the key xor comparison threshold.
    const uint64_t mask = 0xFFFF; // Use lower 16 bits for key_lowbits as per interpretation of spec.

    // Determine if we should replace the entry
    const bool is_empty = (entry->key == 0);
    const bool is_old = (entry->age != current_age);
    const bool is_deeper = (depth > entry->depth);
    const bool is_same_depth_tie_break = (depth == entry->depth && (((entry->key & mask) ^ (key & mask)) < threshold));

    if (is_empty || is_old || is_deeper || is_same_depth_tie_break) {
        entry->key = key;
        entry->move = move;
        entry->score = score;
        entry->depth = depth;
        entry->flags = flags;
        entry->age = current_age;
    }
}

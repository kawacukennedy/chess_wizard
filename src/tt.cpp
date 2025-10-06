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

    // Replacement policy: Prefer deeper; if tie use age; tie-break deterministic
    bool replace = false;
    if (entry->key == 0) {
        replace = true; // Empty entry
    } else if (depth > entry->depth) {
        replace = true; // Deeper
    } else if (depth == entry->depth) {
        if (entry->age != current_age) {
            replace = true; // Age tie
        } else {
            // Tie-break: ((new.key ^ old.key) & 0xFFFFFFFF) < ((old.key ^ new.key) & 0xFFFFFFFF)
            uint32_t new_xor = (key ^ entry->key) & 0xFFFFFFFF;
            uint32_t old_xor = (entry->key ^ key) & 0xFFFFFFFF;
            if (new_xor < old_xor) {
                replace = true;
            }
        }
    }

    if (replace) {
        entry->key = key;
        entry->move = move;
        entry->score = score;
        entry->depth = depth;
        entry->flags = flags;
        entry->age = current_age;
    }
}

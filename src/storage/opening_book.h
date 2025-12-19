#pragma once

#include "types.h"
#include <string>
#include <vector>

class Book {
public:
    bool load(const std::string& path);
    Move get_move(uint64_t hash);
    bool is_loaded() const { return !entries.empty(); }

private:
    struct BookEntry {
        uint64_t key;
        uint16_t move;
        uint16_t weight;
        uint32_t learn;
    };
    std::vector<BookEntry> entries;
};

#ifndef CHESS_WIZARD_API_H
#define CHESS_WIZARD_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Search result info flags enum
typedef enum {
    NO_INFO_FLAG = 0,
    BOOK = 1 << 0,
    TB = 1 << 1,
    CACHE = 1 << 2,
    MC_TIEBREAK = 1 << 3,
    RESIGN = 1 << 4,
    ERROR = 1 << 5
} InfoFlags;

// SearchResult struct as per specification
typedef struct {
    char best_move_uci[8];
    char* pv_json; // malloced JSON array of UCI strings, caller must free
    int32_t score_cp;
    double win_prob;
    double win_prob_stddev;
    uint8_t depth;
    uint64_t nodes;
    uint32_t time_ms;
    uint32_t info_flags; // bitmask using InfoFlags enum
    char* error_message; // optional error message, caller must free
} SearchResult;

// ChessWizardOptions struct as per specification
typedef struct {
    int use_nnue;
    const char *nnue_path;
    int use_syzygy;
    const char **tb_paths;
    const char *book_path;
    uint32_t tt_size_mb;
    uint8_t multi_pv;
    double resign_threshold;
    uint64_t seed;
} ChessWizardOptions;

// C API function as per specification
extern SearchResult chess_wizard_suggest_move(const char* fen_or_moves, uint32_t max_time_ms, uint8_t max_depth, const ChessWizardOptions* opts);

// Memory management
extern void free_api_memory(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // CHESS_WIZARD_API_H
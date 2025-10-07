#!/usr/bin/env python3

import sys
import os
import json
import argparse
from ctypes import *
import chess

# Load the shared library
lib_path = os.path.join(os.path.dirname(__file__), 'build', 'libchesswizard_shared.so')
if not os.path.exists(lib_path):
    print("Error: Shared library not found. Please build the project first.")
    sys.exit(1)

lib = CDLL(lib_path)

# Define C structs in Python

class ChessWizardOptions(Structure):
    _fields_ = [
        ("use_nnue", c_bool),
        ("nnue_path", c_char_p),
        ("use_syzygy", c_bool),
        ("tb_paths", POINTER(c_char_p)),
        ("book_path", c_char_p),
        ("tt_size_mb", c_uint32),
        ("multi_pv", c_uint8),
        ("resign_threshold", c_double),
        ("seed", c_uint64)
    ]

class SearchResult(Structure):
    _fields_ = [
        ("best_move_uci", c_char * 8),
        ("pv_json", c_char_p),
        ("score_cp", c_int32),
        ("win_prob", c_double),
        ("win_prob_stddev", c_double),
        ("depth", c_uint8),
        ("nodes", c_uint64),
        ("time_ms", c_uint32),
        ("info_flags", c_uint32),
        ("error_code", c_int),
        ("error_message", c_char * 256),
        ("last_valid_move", c_char * 8),
        ("debug_zobrist", c_uint64)
    ]

# Function signature
lib.chess_wizard_suggest_move.argtypes = [c_char_p, c_uint32, c_uint8, POINTER(ChessWizardOptions)]
lib.chess_wizard_suggest_move.restype = SearchResult

# Global options
options = ChessWizardOptions()
options.use_nnue = False
options.nnue_path = None
options.use_syzygy = False
options.tb_paths = None
options.book_path = None
options.tt_size_mb = 32
options.multi_pv = 1
options.resign_threshold = 0.01
options.seed = 0

# LRU Cache
CACHE_SIZE = 4096
cache = {}
cache_order = []

def get_cached_result(fen):
    if fen in cache:
        # Move to end
        cache_order.remove(fen)
        cache_order.append(fen)
        return cache[fen]
    return None

def put_cached_result(fen, result):
    if fen in cache:
        cache[fen] = result
        cache_order.remove(fen)
        cache_order.append(fen)
    else:
        if len(cache) >= CACHE_SIZE:
            oldest = cache_order.pop(0)
            del cache[oldest]
        cache[fen] = result
        cache_order.append(fen)

def clear_cache():
    global cache, cache_order
    cache.clear()
    cache_order.clear()

def print_result(result):
    # JSON
    pv_json = result.pv_json.decode('utf-8') if result.pv_json else '[]'
    json_output = {
        "best_move": result.best_move_uci.decode('utf-8'),
        "pv": json.loads(pv_json),
        "score_cp": result.score_cp,
        "win_prob": result.win_prob,
        "win_prob_stddev": result.win_prob_stddev,
        "depth": result.depth,
        "nodes": result.nodes,
        "time_ms": result.time_ms
    }
    print(json.dumps(json_output))

    # Human summary
    source = "ENGINE"
    if result.info_flags & 1:  # BOOK
        source = "BOOK"
    elif result.info_flags & 2:  # TB
        source = "TB"
    elif result.info_flags & 4:  # MC_TIEBREAK
        source = "MC"

    print(f"Recommended: {result.best_move_uci.decode('utf-8')}  Score: {result.score_cp}  WinProb: {result.win_prob*100:.1f}%  StdDev: {result.win_prob_stddev*100:.1f}%  Depth: {result.depth}  Nodes: {result.nodes}  Time: {result.time_ms}ms  Source: {source}")

def normalize_fen(board):
    return board.fen().rstrip()

def suggest_move(board, time_ms=5000, max_depth=64):
    fen = normalize_fen(board)
    cached = get_cached_result(fen)
    if cached:
        return cached

    fen_c = c_char_p(fen.encode('utf-8'))
    result = lib.chess_wizard_suggest_move(fen_c, time_ms, max_depth, byref(options))
    put_cached_result(fen, result)
    return result

def handle_case_me(board):
    # Check book - for now, assume no book
    result = suggest_move(board)
    print_result(result)
    board.push_uci(result.best_move_uci.decode('utf-8'))

def handle_case_opponent(board):
    while True:
        print("Enter opponent move (UCI or SAN), or type newgame/undo/quit/fen <FEN>/set time <ms>/set tt <MB>: ", end='')
        line = input().strip()
        if line == 'quit':
            return False
        elif line == 'newgame':
            board.reset()
            clear_cache()
            print("New game started.")
            continue
        elif line == 'undo':
            if len(board.move_stack) >= 2:
                board.pop()
                board.pop()
                print("Undid last two moves.")
            else:
                print("Cannot undo.")
            continue
        elif line.startswith('fen '):
            try:
                fen = line[4:].strip()
                board.set_fen(fen)
                print("Board set to FEN.")
            except:
                print("Invalid FEN.")
            continue
        elif line.startswith('set time '):
            try:
                time_ms = int(line[9:].strip())
                # TODO: set global time
                print(f"Time set to {time_ms}ms.")
            except:
                print("Invalid time.")
            continue
        elif line.startswith('set tt '):
            try:
                tt_mb = int(line[7:].strip())
                options.tt_size_mb = tt_mb
                print(f"TT size set to {tt_mb}MB.")
            except:
                print("Invalid TT size.")
            continue

        try:
            move = board.parse_san(line) if not line.isalnum() or len(line) > 4 else board.parse_uci(line)
            if move in board.legal_moves:
                board.push(move)
                print(f"Applied move: {line}")

                # Check terminal
                if board.is_checkmate():
                    print("Checkmate! You win.")
                    return False
                elif board.is_stalemate():
                    print("Stalemate! Draw.")
                    return False
                elif board.is_insufficient_material():
                    print("Insufficient material! Draw.")
                    return False
                elif board.halfmove_clock >= 100:
                    print("50-move rule! Draw.")
                    return False

                result = suggest_move(board)
                print_result(result)
                board.push_uci(result.best_move_uci.decode('utf-8'))

                # Check terminal after engine move
                if board.is_checkmate():
                    print("Checkmate! Engine wins.")
                    return False
                elif board.is_stalemate():
                    print("Stalemate! Draw.")
                    return False
                elif board.is_insufficient_material():
                    print("Insufficient material! Draw.")
                    return False
                elif board.halfmove_clock >= 100:
                    print("50-move rule! Draw.")
                    return False
            else:
                print("Illegal move.")
        except:
            print("Invalid move or command.")

    return True

def main():
    parser = argparse.ArgumentParser(description='Chess Wizard CLI')
    parser.add_argument('--time', type=int, default=5000, help='Default time per move in ms')
    parser.add_argument('--tt', type=int, default=32, help='TT size in MB')
    parser.add_argument('--nnue', type=str, help='NNUE file path')
    parser.add_argument('--tb', type=str, help='Syzygy TB path')
    parser.add_argument('--book', type=str, help='Opening book path')
    parser.add_argument('--seed', type=int, default=0, help='Random seed')

    args = parser.parse_args()

    options.tt_size_mb = args.tt
    options.seed = args.seed
    if args.nnue:
        options.use_nnue = True
        options.nnue_path = args.nnue.encode('utf-8')
    if args.tb:
        options.use_syzygy = True
        # TODO: set tb_paths
    if args.book:
        options.book_path = args.book.encode('utf-8')

    print("Chess Wizard ready. Who started the match? Type \"me\" if you started (White) or \"opponent\" if they started (Black).")

    while True:
        response = input().strip().lower()
        if response in ['me', 'white']:
            board = chess.Board()
            handle_case_me(board)
            handle_case_opponent(board)
            break
        elif response in ['opponent', 'black']:
            board = chess.Board()
            if not handle_case_opponent(board):
                break
        else:
            print("Invalid input. Please type 'me' or 'opponent'.")

if __name__ == '__main__':
    main()
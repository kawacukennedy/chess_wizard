# Chess Wizard

**Chess Wizard** is a high-performance chess engine written in C++20, designed for ultra-efficient and ultra-accurate win-optimized move prediction. This project is engineered from a detailed specification to maximize practical win probability and tactical accuracy while maintaining extreme efficiency (low latency, high nodes/sec).

## Table of Contents

- [Features](#features)
- [Architecture Overview](#architecture-overview)
- [Performance Targets](#performance-targets)
- [Building Chess Wizard](#building-chess-wizard)
- [Configuration](#configuration)
- [Usage](#usage)
- [Testing](#testing)
- [Examples](#examples)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

## Features

Chess Wizard incorporates cutting-edge chess engine technologies to deliver exceptional performance:

*   **High-Performance Search:** Implements an iterative deepening framework with Principal Variation Search (PVS), aspiration windows, and a highly optimized move ordering system including SEE (Static Exchange Evaluation), killer moves, and history heuristics.
*   **Advanced Pruning and Extensions:** Utilizes modern techniques such as Null Move Pruning, Late Move Reductions (LMR), Futility Pruning, Razoring, and Singular Extensions to efficiently explore the search tree while maintaining accuracy.
*   **Hybrid Evaluation:** Features a primary NNUE (Efficiently Updatable Neural Network) evaluator for fast, accurate position evaluations, with a fallback to a sophisticated classical evaluation function incorporating piece-square tables, mobility, and positional factors.
*   **Endgame Tablebases:** Integrates Syzygy tablebases for perfect play in endgame positions (K vs K, KQ vs K, etc.).
*   **Opening Book:** Supports Polyglot opening books for theoretical play in the opening phase.
*   **Monte Carlo Tie-Break:** Employs Monte Carlo rollouts to resolve close positions (within 20cp) with high uncertainty, improving decision-making in complex scenarios.
*   **UCI and CLI Interfaces:** Provides a standard UCI (Universal Chess Interface) for compatibility with chess GUIs, and a command-line interface for direct interaction and analysis.
*   **Modern C++20:** Written in modern C++20 for optimal performance, readability, and maintainability.
*   **Cross-Platform:** Designed to compile and run on Linux, macOS, and Windows.
*   **Comprehensive Testing:** Includes unit tests for core components (e.g., Zobrist hashing, NNUE parity) and integration tests for perft verification up to depth 6.

## Architecture Overview

Chess Wizard is structured around modular components for clarity and efficiency:

- **Core Engine (`engine.cpp`):** Handles search algorithms, evaluation, and move generation.
- **Board Representation (`position.cpp`, `bitboard.cpp`):** Efficient bitboard-based position management with Zobrist hashing for transposition tables.
- **Move Generation (`movegen.cpp`):** Generates legal and pseudo-legal moves with optimizations.
- **Evaluation (`evaluate.cpp`, `nnue.cpp`):** Hybrid evaluation system with NNUE support.
- **Tablebases (`syzygy.cpp`):** Syzygy endgame database integration.
- **Opening Book (`book.cpp`):** Polyglot book support.
- **Transposition Table (`tt.cpp`):** Efficient hash table for search memoization.
- **Types and Utilities (`types.h`, `move.h`, etc.):** Shared data structures and helper functions.

The engine follows a specification defined in `chess_wizard_spec.json`, ensuring all features are implemented according to design requirements.

## Performance Targets

*   **x86_64 with AVX2:** >= 6,000,000 nodes/sec
*   **x86_64 without AVX2:** >= 3,000,000 nodes/sec
*   **ARM64 (NEON):** >= 3,000,000 nodes/sec

These targets are measured on standard hardware configurations and may vary based on system specifics.

## Building Chess Wizard

This project uses CMake for building.

### Prerequisites

*   A C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+).
*   CMake (version 3.20 or later).

### Build Steps

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/kawacukennedy/chess_wizard.git
    cd chess_wizard
    ```

2.  **Configure and build with CMake:**
    ```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    ```

3.  **Run the engine:**
    The executable `chess_wizard` will be located in the `build` directory. You can run it directly to use the UCI interface.

## Configuration

Chess Wizard can be configured via command-line options or UCI setoption commands:

- **NNUE Path:** Specify the path to the NNUE evaluation file using `--nnue-path <path>` or `setoption name EvalFile value <path>`.
- **Syzygy Path:** Set the directory containing Syzygy tablebase files with `--syzygy-path <path>` or `setoption name SyzygyPath value <path>`.
- **Opening Book:** Provide a Polyglot book file via `--book-path <path>` or `setoption name BookFile value <path>`.
- **Resignation Threshold:** Adjust the win probability threshold for automatic resignation with `--resign-threshold <float>` (default: 0.05).

## Usage

### UCI Protocol

Chess Wizard supports the UCI protocol for integration with chess GUIs:

```bash
./chess_wizard
uci
setoption name EvalFile value nnue_file.bin
position startpos
go movetime 1000
```

### Command-Line Interface

For direct command-line usage:

```bash
./chess_wizard --help  # Display help
./chess_wizard --test  # Run unit tests
./chess_wizard --integration-test  # Run integration tests
```

You can also pipe UCI commands:

```bash
echo "position startpos moves e2e4 e7e5" | ./chess_wizard
echo "go" | ./chess_wizard
```

## Testing

Chess Wizard includes comprehensive tests to ensure correctness and performance:

- **Unit Tests:** Run with `./chess_wizard --test`. Tests include Zobrist key invariance and NNUE evaluation parity.
- **Integration Tests:** Run with `./chess_wizard --integration-test`. Verifies perft (move count) up to depth 6 for correctness.

All tests must pass before contributions are accepted.

## Examples

### Basic Search

```bash
./chess_wizard << EOF
uci
ucinewgame
position startpos
go depth 10
EOF
```

### Endgame Analysis with Tablebases

```bash
./chess_wizard << EOF
setoption name SyzygyPath value /path/to/syzygy
position fen 8/8/8/8/8/8/8/K7 w - - 0 1
go
EOF
```

### CLI Mode

```bash
./chess_wizard
# Then type: position startpos
# go movetime 2000
```

## Documentation

For a detailed specification of the engine's architecture, algorithms, and data structures, please refer to the `chess_wizard_spec.json` file in this repository.

## Contributing

Contributions are welcome! Please see `CONTRIBUTING.md` for details on how to contribute to this project.

## License

This project is licensed under the MIT License - see the `LICENSE` file for details.

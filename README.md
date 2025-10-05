# Chess Wizard

**Chess Wizard** is a high-performance chess engine written in C++20, designed for ultra-efficient and ultra-accurate win-optimized move prediction. This project is engineered from a detailed specification to maximize practical win probability and tactical accuracy while maintaining extreme efficiency (low latency, high nodes/sec).

## Features

*   **High-Performance Search:** Implements an iterative deepening framework with a Principal Variation Search (PVS) algorithm, aspiration windows, and a highly optimized move ordering system.
*   **Advanced Pruning:** Utilizes a suite of modern pruning techniques including Null Move Pruning, Late Move Reductions (LMR), Futility Pruning, and Razoring to efficiently traverse the search tree.
*   **Hybrid Evaluation:** Features a primary NNUE (Efficiently Updatable Neural Network) evaluator for fast, accurate evaluations, with a fallback to a sophisticated classical evaluation function.
*   **Endgame Tablebases:** Integrates with Syzygy tablebases to provide perfect play in endgame positions.
*   **Opening Book:** Uses a Polyglot opening book to play theoretical moves in the opening.
*   **UCI and C-API:** Provides a standard UCI (Universal Chess Interface) for compatibility with most chess GUIs, as well as a C-API for easy integration into other applications.
*   **Modern C++20:** Written in modern C++20 for performance, readability, and maintainability.
*   **Cross-Platform:** Designed to be compiled and run on Linux, macOS, and Windows.

## Performance Targets

*   **x86_64 with AVX2:** >= 6,000,000 nodes/sec
*   **x86_64 without AVX2:** >= 3,000,000 nodes/sec
*   **ARM64 (NEON):** >= 3,000,000 nodes/sec

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

## Usage

Chess Wizard supports the UCI protocol. You can use it with any UCI-compatible chess GUI, such as Arena, Cute Chess, or Shredder.

You can also use the engine from the command line for basic interaction or analysis.

## Documentation

For a detailed specification of the engine's architecture, algorithms, and data structures, please refer to the `chess_wizard_spec.json` file in this repository.

## Contributing

Contributions are welcome! Please see `CONTRIBUTING.md` for details on how to contribute to this project.

## License

This project is licensed under the MIT License - see the `LICENSE` file for details.

# Contributing to Chess Wizard

Thank you for your interest in contributing to Chess Wizard! We welcome contributions from everyone. By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

There are many ways to contribute to Chess Wizard, including:

*   **Reporting Bugs:** If you find a bug, please open an issue on GitHub. Include a clear description of the bug, steps to reproduce it, and any relevant logs or screenshots.
*   **Suggesting Enhancements:** If you have an idea for a new feature or an improvement to an existing one, please open an issue to discuss it.
*   **Writing Code:** If you'd like to contribute code, please follow these steps:
    1.  Fork the repository and create a new branch for your feature or bug fix.
    2.  Make your changes, adhering to the existing code style.
    3.  Write unit tests for your changes.
    4.  Ensure all tests pass.
    5.  Submit a pull request with a clear description of your changes.

## Development Setup

Please refer to the `README.md` file for instructions on how to build and run the project.

### Testing

Before submitting a pull request, ensure all tests pass:

- Run unit tests: `./chess_wizard --test`
- Run integration tests: `./chess_wizard --integration-test`

Add tests for new features or bug fixes as appropriate.

## Coding Style

Please follow the existing coding style in the project. We use a consistent style to maintain readability and ease of maintenance. Key guidelines:

- Use C++20 features where beneficial.
- Prefer `const` and references for parameters.
- Use meaningful variable names.
- Comment complex logic, but avoid over-commenting obvious code.

## Pull Request Process

1.  Ensure any install or build dependencies are removed before the end of the layer when doing a build.
2.  Update the `README.md` with details of changes to the interface, this includes new environment variables, exposed ports, useful file locations and container parameters.
3.  Increase the version numbers in any examples and the `README.md` to the new version that this Pull Request would represent. The versioning scheme we use is [SemVer](http://semver.org/).
4.  You may merge the Pull Request in once you have the sign-off of two other developers, or if you do not have permission to do that, you may request the second reviewer to merge it for you.

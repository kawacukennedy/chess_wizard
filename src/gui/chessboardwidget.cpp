#include "chessboardwidget.h"
#include <QApplication>

ChessBoardWidget::ChessBoardWidget(QWidget *parent) : QWidget(parent) {
    setFixedSize(480, 480);
    // Load piece images (placeholder)
    // pieceImages[0] = QPixmap("resources/pieces/wP.svg");
    // etc.
}

void ChessBoardWidget::setPosition(const std::string& fen) {
    currentFen = fen;
    update();
}

void ChessBoardWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            QColor color = (rank + file) % 2 == 0 ? lightColor : darkColor;
            painter.fillRect(file * squareSize, (7 - rank) * squareSize, squareSize, squareSize, color);
        }
    }
    // Draw pieces based on fen (placeholder)
}

void ChessBoardWidget::mousePressEvent(QMouseEvent *event) {
    int file = event->x() / squareSize;
    int rank = 7 - (event->y() / squareSize);
    selectedSquare = rank * 8 + file;
    update();
}

void ChessBoardWidget::mouseReleaseEvent(QMouseEvent *event) {
    int file = event->x() / squareSize;
    int rank = 7 - (event->y() / squareSize);
    int targetSquare = rank * 8 + file;
    if (selectedSquare != -1 && targetSquare != selectedSquare) {
        // Generate move string
        char move[5];
        move[0] = 'a' + (selectedSquare % 8);
        move[1] = '1' + (selectedSquare / 8);
        move[2] = 'a' + (targetSquare % 8);
        move[3] = '1' + (targetSquare / 8);
        move[4] = '\0';
        emit moveMade(std::string(move));
    }
    selectedSquare = -1;
    update();
}
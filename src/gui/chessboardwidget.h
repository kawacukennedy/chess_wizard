#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPixmap>
#include <array>

class ChessBoardWidget : public QWidget {
    Q_OBJECT

public:
    ChessBoardWidget(QWidget *parent = nullptr);
    void setPosition(const std::string& fen);

signals:
    void moveMade(const std::string& move);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    std::array<QPixmap, 12> pieceImages;
    std::string currentFen;
    int squareSize = 60;
    QColor lightColor = QColor("#F0D9B5");
    QColor darkColor = QColor("#B58863");
    int selectedSquare = -1;
    int draggedSquare = -1;
    QPoint dragOffset;
};

#endif // CHESSBOARDWIDGET_H
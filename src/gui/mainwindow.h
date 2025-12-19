#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>

class ChessBoardWidget;
class SidePanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewGame();
    void onSuggestMove();
    void onUndo();
    void onRedo();

private:
    ChessBoardWidget *chessBoard;
    SidePanel *sidePanel;
    QHBoxLayout *mainLayout;
};

#endif // MAINWINDOW_H
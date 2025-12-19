#include "mainwindow.h"
#include "chessboardwidget.h"
#include "sidepanel.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Chess Wizard");
    setFixedSize(800, 480);

    QWidget *centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    mainLayout = new QHBoxLayout(centralWidget);
    chessBoard = new ChessBoardWidget();
    sidePanel = new SidePanel();

    mainLayout->addWidget(chessBoard);
    mainLayout->addWidget(sidePanel);

    connect(chessBoard, &ChessBoardWidget::moveMade, this, [this](const std::string& move) {
        // Handle move
        sidePanel->addMove(QString::fromStdString(move));
    });

    connect(sidePanel->suggestButton, &QPushButton::clicked, this, &MainWindow::onSuggestMove);
    connect(sidePanel->undoButton, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(sidePanel->redoButton, &QPushButton::clicked, this, &MainWindow::onRedo);
    connect(sidePanel->newGameButton, &QPushButton::clicked, this, &MainWindow::onNewGame);
}

MainWindow::~MainWindow() {}

void MainWindow::onNewGame() {
    chessBoard->setPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    sidePanel->moveList->clear();
}

void MainWindow::onSuggestMove() {
    // Call engine to suggest move
    // For now, placeholder
}

void MainWindow::onUndo() {
    // Undo move
}

void MainWindow::onRedo() {
    // Redo move
}
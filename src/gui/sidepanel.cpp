#include "sidepanel.h"

SidePanel::SidePanel(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    evalBar = new QProgressBar();
    evalBar->setRange(-1000, 1000);
    evalBar->setValue(0);
    layout->addWidget(evalBar);

    moveList = new QTextEdit();
    moveList->setReadOnly(true);
    layout->addWidget(moveList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    suggestButton = new QPushButton("Suggest Move");
    undoButton = new QPushButton("Undo");
    redoButton = new QPushButton("Redo");
    newGameButton = new QPushButton("New Game");
    buttonLayout->addWidget(suggestButton);
    buttonLayout->addWidget(undoButton);
    buttonLayout->addWidget(redoButton);
    buttonLayout->addWidget(newGameButton);
    layout->addLayout(buttonLayout);
}

void SidePanel::updateEvaluation(int score) {
    evalBar->setValue(score);
}

void SidePanel::addMove(const QString& move) {
    moveList->append(move);
}
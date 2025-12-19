#ifndef SIDEPANEL_H
#define SIDEPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>

class SidePanel : public QWidget {
    Q_OBJECT

public:
    SidePanel(QWidget *parent = nullptr);

public slots:
    void updateEvaluation(int score);
    void addMove(const QString& move);

private:
    QVBoxLayout *layout;
    QProgressBar *evalBar;
    QTextEdit *moveList;
    QPushButton *suggestButton;
    QPushButton *undoButton;
    QPushButton *redoButton;
    QPushButton *newGameButton;
};

#endif // SIDEPANEL_H
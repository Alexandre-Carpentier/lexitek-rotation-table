/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <memory>
#include <string>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void lock_controls(bool lock);
    void status(std::string);

    void registersignals(Ui::MainWindow* ui);
    Ui::MainWindow* ui;
    QTimer* timer = nullptr;

private slots:
    void onRunButtonClicked();
	void onCalibrateButtonClicked();
    void onHomeButtonClicked();
	void onTargetEditChanged();
    void onTimerTimeout();
    void onDialReleased();
};

#endif // MAINWINDOW_H

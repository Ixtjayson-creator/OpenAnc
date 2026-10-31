#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include "DeviceManager.h"
#include "AudioEngine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void onStartClicked();
    void onStopClicked();
    void onCalibrateClicked();
    void onIntensityChanged(int value);
private:
    void populateDeviceLists();
    DeviceManager deviceManager_;
    AudioEngine audioEngine_;
    QListWidget *renderList_;
    QListWidget *captureList_;
    QLabel *statusLabel_;
    QPushButton *startBtn_;
    QPushButton *stopBtn_;
    QPushButton *calibBtn_;
    QSlider *intensitySlider_;
    bool engineRunning_ = false;
};


#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "DeviceManager.h"
#include "AudioEngine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void onStartClicked();
    void onStopClicked();
private:
    void populateDeviceLists();
    DeviceManager deviceManager_;
    AudioEngine audioEngine_;
    QListWidget *renderList_;
    QListWidget *captureList_;
    QLabel *statusLabel_;
    QPushButton *startBtn_;
    QPushButton *stopBtn_;
    bool engineRunning_ = false;
};


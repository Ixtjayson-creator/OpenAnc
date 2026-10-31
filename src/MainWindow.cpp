#include "MainWindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      renderList_(new QListWidget(this)),
      captureList_(new QListWidget(this)),
      statusLabel_(new QLabel(tr("Idle"), this)),
      startBtn_(new QPushButton(tr("Start ANC"), this)),
      stopBtn_(new QPushButton(tr("Stop ANC"), this)),
      calibBtn_(new QPushButton(tr("Calibrate Latency"), this)),
      intensitySlider_(new QSlider(Qt::Horizontal, this))
{
    setWindowTitle(tr("OpenANC - Bluetooth Software ANC"));
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    QHBoxLayout *listsLayout = new QHBoxLayout();
    listsLayout->addWidget(new QLabel(tr("Render Devices"), this));
    listsLayout->addWidget(renderList_);
    listsLayout->addWidget(new QLabel(tr("Capture Devices"), this));
    listsLayout->addWidget(captureList_);
    mainLayout->addLayout(listsLayout);

    QHBoxLayout *controls = new QHBoxLayout();
    controls->addWidget(startBtn_);
    controls->addWidget(stopBtn_);
    controls->addWidget(calibBtn_);
    controls->addWidget(new QLabel(tr("Intensity"), this));
    intensitySlider_->setRange(0, 100);
    intensitySlider_->setValue(100);
    controls->addWidget(intensitySlider_);
    mainLayout->addLayout(controls);
    mainLayout->addWidget(statusLabel_);

    // connections
    connect(startBtn_, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(calibBtn_, &QPushButton::clicked, this, &MainWindow::onCalibrateClicked);
    connect(intensitySlider_, &QSlider::valueChanged, this, &MainWindow::onIntensityChanged);

    populateDeviceLists();
    stopBtn_->setEnabled(false);
    calibBtn_->setEnabled(true);
}

MainWindow::~MainWindow() {}

void MainWindow::populateDeviceLists() {
    deviceManager_.Init();
    renderList_->clear();
    for (const auto &dev : deviceManager_.GetRenderDevices()) {
        QString txt = QString("%1 %2").arg(dev.isBluetooth ? "[BT]" : "[WIRED]").arg(QString::fromStdWString(dev.name));
        renderList_->addItem(txt);
    }
    captureList_->clear();
    for (const auto &dev : deviceManager_.GetCaptureDevices()) {
        QString txt = QString("%1 %2").arg(dev.isBluetooth ? "[BT]" : "[WIRED]").arg(QString::fromStdWString(dev.name));
        captureList_->addItem(txt);
    }
}

void MainWindow::onStartClicked() {
    if (engineRunning_) return;
    HRESULT hr = audioEngine_.Init();
    if (FAILED(hr)) {
        QMessageBox::critical(this, tr("Error"), tr("AudioEngine Init failed: 0x%1").arg(hr, 8, 16, QLatin1Char('0')));
        return;
    }
    // optionally calibrate first if not done
    if (audioEngine_.CalibrateLatency() != S_OK) {
        statusLabel_->setText(tr("Calibration failed – using 0 latency"));
    } else {
        statusLabel_->setText(tr("Latency calibrated"));
    }
    hr = audioEngine_.Start();
    if (FAILED(hr)) {
        QMessageBox::critical(this, tr("Error"), tr("AudioEngine Start failed: 0x%1").arg(hr, 8, 16, QLatin1Char('0')));
        return;
    }
    engineRunning_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(tr("ANC running"));
}

void MainWindow::onStopClicked() {
    if (!engineRunning_) return;
    audioEngine_.Stop();
    engineRunning_ = false;
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    statusLabel_->setText(tr("ANC stopped"));
}

void MainWindow::onCalibrateClicked() {
    HRESULT hr = audioEngine_.CalibrateLatency();
    if (SUCCEEDED(hr)) {
        statusLabel_->setText(tr("Calibration complete – latency %1 samples").arg(audioEngine_.delaySamples_));
    } else {
        statusLabel_->setText(tr("Calibration error"));
    }
}

void MainWindow::onIntensityChanged(int value) {
    float f = static_cast<float>(value) / 100.0f;
    audioEngine_.SetIntensity(f);
}


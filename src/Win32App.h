#pragma once
#include <windows.h>
#include <commctrl.h>
#include "DeviceManager.h"
#include "AudioEngine.h"

class Win32App {
public:
    static int Run(HINSTANCE hInstance, int nCmdShow);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void CreateControls(HWND hWnd);
    void PopulateDeviceLists();
    void UpdateStatus(const std::wstring& status);

    HWND hWnd_ = nullptr;
    HWND hComboRender_ = nullptr;
    HWND hComboCapture_ = nullptr;
    HWND hBtnStart_ = nullptr;
    HWND hBtnStop_ = nullptr;
    HWND hBtnCalib_ = nullptr;
    HWND hBtnMeasure_ = nullptr;
    HWND hSliderIntensity_ = nullptr;
    HWND hSliderCutoff_ = nullptr;
    HWND hChkHP_ = nullptr;
    HWND hChkComp_ = nullptr;
    HWND hLblStatus_ = nullptr;
    HWND hLblIntensityVal_ = nullptr;
    HWND hLblCutoffVal_ = nullptr;

    DeviceManager deviceManager_;
    AudioEngine audioEngine_;
    std::vector<AudioDeviceInfo> renderDevices_;
    std::vector<AudioDeviceInfo> captureDevices_;
};


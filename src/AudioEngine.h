#pragma once
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include "FxLMS.h"
#include "HighPassFilter.h"
#include "Limiter.h"
#include "AcousticCalibrator.h"

using Microsoft::WRL::ComPtr;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    HRESULT Init();
    HRESULT Start();
    void Stop();

    void SetPreferredDeviceIds(const std::wstring& renderId, const std::wstring& captureId);
    HRESULT CalibrateLatency();
    HRESULT MeasureAcousticResponse();

    void SetIntensity(float intensity);
    void SetHighPassEnabled(bool en);
    void SetHighPassCutoff(float hz);
    void EnableSpeakerCompensation(bool en);

    size_t GetLatencySamples() const { return delaySamples_; }
    bool IsRunning() const { return running_; }

private:
    void Loop();
    void ApplyDelay(const float* in, float* out, size_t count);
    void ApplySpeakerCompensation(std::vector<float>& anti);

    HRESULT LoadCalibration(const std::wstring& deviceId);
    HRESULT SaveCalibration(const std::wstring& deviceId) const;

    ComPtr<IAudioClient> renderClient_;
    ComPtr<IAudioClient> captureClient_;
    ComPtr<IAudioRenderClient> renderSink_;
    ComPtr<IAudioCaptureClient> captureSource_;
    HANDLE renderEvent_ = nullptr;
    HANDLE captureEvent_ = nullptr;

    FxLMS fx_;
    HighPassFilter hpFilter_;
    Limiter limiter_{0.9f};
    bool hpEnabled_ = true;
    bool compensationEnabled_ = true;

    std::thread thread_;
    std::atomic<bool> running_{false};
    WAVEFORMATEX* pwfx_ = nullptr;

    std::vector<float> delayLine_;
    size_t delayPos_ = 0;
    size_t delaySamples_ = 0;
    size_t delayLineSize_ = 0;

    std::vector<float> speakerResponse_;
    std::vector<float> irBuffer_;
    size_t irPos_ = 0;

    std::wstring renderId_;
    std::wstring captureId_;
    std::wstring calibrationFolder_;
};

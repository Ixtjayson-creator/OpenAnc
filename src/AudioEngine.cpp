#include "AudioEngine.h"
#include "DeviceManager.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <shlobj.h>

static std::wstring GetCalibFolder() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        std::wstring folder = std::wstring(path) + L"\\OpenANC\\calib";
        CreateDirectoryW((std::wstring(path) + L"\\OpenANC").c_str(), nullptr);
        CreateDirectoryW(folder.c_str(), nullptr);
        return folder;
    }
    return L".";
}

AudioEngine::AudioEngine() {}

AudioEngine::~AudioEngine() {
    Stop();
    if (pwfx_) { CoTaskMemFree(pwfx_); pwfx_ = nullptr; }
    if (renderEvent_) CloseHandle(renderEvent_);
    if (captureEvent_) CloseHandle(captureEvent_);
}

void AudioEngine::SetPreferredDeviceIds(const std::wstring& renderId, const std::wstring& captureId) {
    renderId_ = renderId;
    captureId_ = captureId;
}

HRESULT AudioEngine::Init() {
    Stop();

    if (pwfx_) { CoTaskMemFree(pwfx_); pwfx_ = nullptr; }
    if (renderEvent_) { CloseHandle(renderEvent_); renderEvent_ = nullptr; }
    if (captureEvent_) { CloseHandle(captureEvent_); captureEvent_ = nullptr; }
    renderClient_.Reset();
    captureClient_.Reset();
    renderSink_.Reset();
    captureSource_.Reset();

    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) return hr;

    ComPtr<IMMDevice> renderDevice;
    if (!renderId_.empty()) {
        hr = enumerator->GetDevice(renderId_.c_str(), &renderDevice);
    }
    if (!renderDevice) {
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &renderDevice);
        if (FAILED(hr)) return hr;
    }

    ComPtr<IMMDevice> captureDevice;
    if (!captureId_.empty()) {
        hr = enumerator->GetDevice(captureId_.c_str(), &captureDevice);
    }
    if (!captureDevice) {
        hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &captureDevice);
        if (FAILED(hr)) return hr;
    }

    hr = renderDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(renderClient_.GetAddressOf()));
    if (FAILED(hr)) return hr;

    hr = renderClient_->GetMixFormat(&pwfx_);
    if (FAILED(hr)) return hr;

    REFERENCE_TIME hnsBufferDuration = 2000000; // 200 ms in 100ns units
    hr = renderClient_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsBufferDuration, 0, pwfx_, nullptr);
    if (FAILED(hr)) return hr;

    hr = renderClient_->GetService(IID_PPV_ARGS(&renderSink_));
    if (FAILED(hr)) return hr;

    renderEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!renderEvent_) return HRESULT_FROM_WIN32(GetLastError());
    hr = renderClient_->SetEventHandle(renderEvent_);
    if (FAILED(hr)) return hr;

    hr = captureDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(captureClient_.GetAddressOf()));
    if (FAILED(hr)) return hr;

    hr = captureClient_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsBufferDuration, 0, pwfx_, nullptr);
    if (FAILED(hr)) return hr;

    hr = captureClient_->GetService(IID_PPV_ARGS(&captureSource_));
    if (FAILED(hr)) return hr;

    captureEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!captureEvent_) return HRESULT_FROM_WIN32(GetLastError());
    hr = captureClient_->SetEventHandle(captureEvent_);
    if (FAILED(hr)) return hr;

    delayLineSize_ = static_cast<size_t>(pwfx_->nSamplesPerSec * 2);
    delayLine_.assign(delayLineSize_, 0.0f);
    delayPos_ = 0;
    delaySamples_ = 0;

    fx_.Init(1024, 0.01f, 0.0001f);
    hpFilter_.setSampleRate(static_cast<float>(pwfx_->nSamplesPerSec));
    hpFilter_.setCutoff(200.0f);
    limiter_.setMaxLevel(0.9f);

    LoadCalibration(renderId_.empty() ? L"default" : renderId_);
    irBuffer_.assign(speakerResponse_.empty() ? 256 : speakerResponse_.size(), 0.0f);
    irPos_ = 0;

    return S_OK;
}

HRESULT AudioEngine::Start() {
    if (running_) return S_OK;
    if (!renderClient_ || !captureClient_) {
        HRESULT hr = Init();
        if (FAILED(hr)) return hr;
    }

    HRESULT hr = renderClient_->Start();
    if (FAILED(hr)) return hr;

    hr = captureClient_->Start();
    if (FAILED(hr)) {
        renderClient_->Stop();
        return hr;
    }

    running_ = true;
    thread_ = std::thread(&AudioEngine::Loop, this);
    return S_OK;
}

void AudioEngine::Stop() {
    if (!running_) return;
    running_ = false;
    if (thread_.joinable()) thread_.join();
    if (renderClient_) renderClient_->Stop();
    if (captureClient_) captureClient_->Stop();

    SaveCalibration(renderId_.empty() ? L"default" : renderId_);
}

void AudioEngine::SetIntensity(float intensity) {
    fx_.SetIntensity(intensity);
}

void AudioEngine::SetHighPassEnabled(bool en) {
    hpEnabled_ = en;
}

void AudioEngine::SetHighPassCutoff(float hz) {
    hpFilter_.setCutoff(hz);
}

void AudioEngine::EnableSpeakerCompensation(bool en) {
    compensationEnabled_ = en;
}

HRESULT AudioEngine::CalibrateLatency() {
    if (!pwfx_ || !renderSink_ || !captureSource_) return E_FAIL;

    UINT32 frames = pwfx_->nSamplesPerSec / 100; // 10ms
    BYTE* renderData = nullptr;
    HRESULT hr = renderSink_->GetBuffer(frames, &renderData);
    if (FAILED(hr)) return hr;

    memset(renderData, 0, frames * pwfx_->nBlockAlign);
    if (pwfx_->wBitsPerSample == 16) {
        int16_t* ptr = reinterpret_cast<int16_t*>(renderData);
        ptr[0] = INT16_MAX;
    } else if (pwfx_->wBitsPerSample == 32) {
        float* ptr = reinterpret_cast<float*>(renderData);
        ptr[0] = 1.0f;
    }
    hr = renderSink_->ReleaseBuffer(frames, 0);
    if (FAILED(hr)) return hr;

    UINT32 maxSamples = static_cast<UINT32>(pwfx_->nSamplesPerSec * 2);
    UINT32 captured = 0;
    bool found = false;

    while (captured < maxSamples && !found) {
        WaitForSingleObject(captureEvent_, 10);
        UINT32 packet = 0;
        hr = captureSource_->GetNextPacketSize(&packet);
        if (FAILED(hr)) break;

        while (packet > 0) {
            BYTE* capData = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;
            hr = captureSource_->GetBuffer(&capData, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            const float threshold = (pwfx_->wBitsPerSample == 16) ? 20000.0f : 0.6f;
            for (UINT32 i = 0; i < numFrames; ++i) {
                float sample = 0.0f;
                if (pwfx_->wBitsPerSample == 16) {
                    int16_t* s = reinterpret_cast<int16_t*>(capData);
                    sample = static_cast<float>(s[i]);
                } else if (pwfx_->wBitsPerSample == 32) {
                    float* s = reinterpret_cast<float*>(capData);
                    sample = s[i];
                }
                if (std::abs(sample) > threshold) {
                    delaySamples_ = captured + i;
                    found = true;
                    break;
                }
            }

            captured += numFrames;
            captureSource_->ReleaseBuffer(numFrames);
            if (found) break;
            hr = captureSource_->GetNextPacketSize(&packet);
            if (FAILED(hr)) break;
        }
    }

    if (!found) {
        delaySamples_ = 0;
    }
    if (delaySamples_ >= delayLineSize_) delaySamples_ = delayLineSize_ - 1;
    std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
    delayPos_ = 0;
    return S_OK;
}

HRESULT AudioEngine::MeasureAcousticResponse() {
    if (!pwfx_) return E_FAIL;
    std::vector<float> sweep = AcousticCalibrator::BuildSweep(static_cast<float>(pwfx_->nSamplesPerSec), 20.0f, 20000.0f, 2.0f);

    UINT32 frames = static_cast<UINT32>(sweep.size());
    BYTE* renderData = nullptr;
    HRESULT hr = renderSink_->GetBuffer(frames, &renderData);
    if (SUCCEEDED(hr)) {
        if (pwfx_->wBitsPerSample == 16) {
            int16_t* dst = reinterpret_cast<int16_t*>(renderData);
            for (size_t i = 0; i < sweep.size(); ++i) {
                dst[i] = static_cast<int16_t>(std::clamp(sweep[i] * 32767.0f, -32768.0f, 32767.0f));
            }
        } else if (pwfx_->wBitsPerSample == 32) {
            float* dst = reinterpret_cast<float*>(renderData);
            std::copy(sweep.begin(), sweep.end(), dst);
        }
        renderSink_->ReleaseBuffer(frames, 0);
    }

    const size_t irLength = 256;
    speakerResponse_.assign(irLength, 0.0f);
    speakerResponse_[0] = 1.0f; // baseline delta IR
    irBuffer_.assign(irLength, 0.0f);
    irPos_ = 0;
    return S_OK;
}

void AudioEngine::ApplyDelay(const float* in, float* out, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        delayLine_[delayPos_] = in[i];
        size_t readPos = (delayPos_ + delayLineSize_ - delaySamples_) % delayLineSize_;
        out[i] = delayLine_[readPos];
        delayPos_ = (delayPos_ + 1) % delayLineSize_;
    }
}

void AudioEngine::ApplySpeakerCompensation(std::vector<float>& anti) {
    if (!compensationEnabled_ || speakerResponse_.empty()) return;

    size_t L = speakerResponse_.size();
    for (float sample : anti) {
        irBuffer_[irPos_] = sample;
        irPos_ = (irPos_ + 1) % L;
    }

    std::vector<float> out(anti.size(), 0.0f);
    for (size_t n = 0; n < anti.size(); ++n) {
        float acc = 0.0f;
        size_t bufIdx = irPos_;
        for (size_t k = 0; k < L; ++k) {
            bufIdx = (bufIdx == 0) ? L - 1 : bufIdx - 1;
            acc += speakerResponse_[k] * irBuffer_[bufIdx];
        }
        out[n] = acc;
    }
    anti.swap(out);
}

HRESULT AudioEngine::LoadCalibration(const std::wstring& deviceId) {
    if (calibrationFolder_.empty()) calibrationFolder_ = GetCalibFolder();
    std::wstring file = calibrationFolder_ + L"\\calib_" + std::to_wstring(std::hash<std::wstring>{}(deviceId)) + L".bin";
    std::ifstream in(file, std::ios::binary);
    if (!in) return S_OK;

    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > 0 && len < 4096) {
        speakerResponse_.resize(len);
        in.read(reinterpret_cast<char*>(speakerResponse_.data()), len * sizeof(float));
    }
    return S_OK;
}

HRESULT AudioEngine::SaveCalibration(const std::wstring& deviceId) const {
    if (speakerResponse_.empty()) return S_OK;
    std::wstring folder = calibrationFolder_.empty() ? GetCalibFolder() : calibrationFolder_;
    std::wstring file = folder + L"\\calib_" + std::to_wstring(std::hash<std::wstring>{}(deviceId)) + L".bin";
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) return E_FAIL;

    uint32_t len = static_cast<uint32_t>(speakerResponse_.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(reinterpret_cast<const char*>(speakerResponse_.data()), len * sizeof(float));
    return S_OK;
}

void AudioEngine::Loop() {
    const UINT32 maxFrames = pwfx_->nSamplesPerSec / 10;
    BYTE* capData = nullptr;
    BYTE* renderData = nullptr;
    std::vector<float> captureFloat(maxFrames);
    std::vector<float> delayedFloat(maxFrames);
    std::vector<float> antiFloat;

    while (running_) {
        WaitForSingleObject(captureEvent_, 10);
        UINT32 packet = 0;
        HRESULT hr = captureSource_->GetNextPacketSize(&packet);
        if (FAILED(hr) || packet == 0) continue;

        while (packet > 0 && running_) {
            UINT32 numFrames = 0;
            DWORD flags = 0;
            hr = captureSource_->GetBuffer(&capData, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            if (numFrames > maxFrames) numFrames = maxFrames;

            if (pwfx_->wBitsPerSample == 16) {
                int16_t* src = reinterpret_cast<int16_t*>(capData);
                for (UINT32 i = 0; i < numFrames; ++i) {
                    captureFloat[i] = static_cast<float>(src[i * pwfx_->nChannels]) / 32768.0f;
                }
            } else if (pwfx_->wBitsPerSample == 32) {
                float* src = reinterpret_cast<float*>(capData);
                for (UINT32 i = 0; i < numFrames; ++i) {
                    captureFloat[i] = src[i * pwfx_->nChannels];
                }
            }

            ApplyDelay(captureFloat.data(), delayedFloat.data(), numFrames);

            if (hpEnabled_) {
                std::vector<float> tmp(delayedFloat.begin(), delayedFloat.begin() + numFrames);
                hpFilter_.process(tmp);
                std::copy(tmp.begin(), tmp.end(), delayedFloat.begin());
            }

            antiFloat = fx_.Process(delayedFloat.data(), numFrames);
            limiter_.process(antiFloat);
            ApplySpeakerCompensation(antiFloat);

            // RMS feedback mute guard
            float rms = 0.0f;
            for (float v : antiFloat) rms += v * v;
            rms = std::sqrt(rms / static_cast<float>(numFrames));
            if (rms > 0.75f) {
                std::fill(antiFloat.begin(), antiFloat.end(), 0.0f);
            }

            hr = renderSink_->GetBuffer(numFrames, &renderData);
            if (SUCCEEDED(hr)) {
                if (pwfx_->wBitsPerSample == 16) {
                    int16_t* dst = reinterpret_cast<int16_t*>(renderData);
                    for (UINT32 i = 0; i < numFrames; ++i) {
                        float v = antiFloat[i] * 32767.0f;
                        v = std::clamp(v, -32768.0f, 32767.0f);
                        for (WORD ch = 0; ch < pwfx_->nChannels; ++ch) {
                            dst[i * pwfx_->nChannels + ch] = static_cast<int16_t>(v);
                        }
                    }
                } else if (pwfx_->wBitsPerSample == 32) {
                    float* dst = reinterpret_cast<float*>(renderData);
                    for (UINT32 i = 0; i < numFrames; ++i) {
                        for (WORD ch = 0; ch < pwfx_->nChannels; ++ch) {
                            dst[i * pwfx_->nChannels + ch] = antiFloat[i];
                        }
                    }
                }
                renderSink_->ReleaseBuffer(numFrames, 0);
            }

            captureSource_->ReleaseBuffer(numFrames);
            hr = captureSource_->GetNextPacketSize(&packet);
            if (FAILED(hr)) break;
        }
    }
}

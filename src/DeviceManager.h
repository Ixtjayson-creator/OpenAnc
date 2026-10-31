#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

struct AudioDeviceInfo {
    std::wstring id;
    std::wstring name;
    bool isRender; // speaker
    bool isCapture; // mic
    bool isBluetooth;
};

class DeviceManager : public IMMNotificationClient {
public:
    DeviceManager();
    ~DeviceManager();
    HRESULT Init();
    std::vector<AudioDeviceInfo> GetRenderDevices() const;
    std::vector<AudioDeviceInfo> GetCaptureDevices() const;

    // IMMNotificationClient callbacks
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override;

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

private:
    void Enumerate();
    std::vector<AudioDeviceInfo> renderDevices_;
    std::vector<AudioDeviceInfo> captureDevices_;
    ComPtr<IMMDeviceEnumerator> enumerator_;
    LONG refCount_ = 1;
};

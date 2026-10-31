#include "DeviceManager.h"
#include <iostream>

DeviceManager::DeviceManager() {}
DeviceManager::~DeviceManager() {}

HRESULT DeviceManager::Init() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return hr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator_));
    if (FAILED(hr)) return hr;

    hr = enumerator_->RegisterEndpointNotificationCallback(this);
    if (FAILED(hr)) return hr;

    Enumerate();
    return S_OK;
}

void DeviceManager::Enumerate() {
    renderDevices_.clear();
    captureDevices_.clear();

    // Render devices
    ComPtr<IMMDeviceCollection> collection;
    HRESULT hr = enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (SUCCEEDED(hr)) {
        UINT count = 0;
        collection->GetCount(&count);
        for (UINT i = 0; i < count; ++i) {
            ComPtr<IMMDevice> device;
            if (FAILED(collection->Item(i, &device))) continue;

            LPWSTR id = nullptr;
            device->GetId(&id);

            ComPtr<IPropertyStore> props;
            if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props))) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                props->GetValue(PKEY_Device_FriendlyName, &varName);

                PROPVARIANT varBT;
                PropVariantInit(&varBT);
                props->GetValue(PKEY_DeviceInterface_FriendlyName, &varBT);

                std::wstring name = (varName.vt == VT_LPWSTR && varName.pwszVal) ? varName.pwszVal : L"Unknown Device";
                std::wstring iface = (varBT.vt == VT_LPWSTR && varBT.pwszVal) ? varBT.pwszVal : L"";

                AudioDeviceInfo info;
                info.id = id ? id : L"";
                info.name = name;
                info.isRender = true;
                info.isCapture = false;
                info.isBluetooth = (iface.find(L"Bluetooth") != std::wstring::npos) ||
                                   (name.find(L"Bluetooth") != std::wstring::npos) ||
                                   (info.id.find(L"BTHENUM") != std::wstring::npos);

                renderDevices_.push_back(info);

                PropVariantClear(&varName);
                PropVariantClear(&varBT);
            }
            if (id) CoTaskMemFree(id);
        }
    }

    // Capture devices
    collection.Reset();
    hr = enumerator_->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
    if (SUCCEEDED(hr)) {
        UINT count = 0;
        collection->GetCount(&count);
        for (UINT i = 0; i < count; ++i) {
            ComPtr<IMMDevice> device;
            if (FAILED(collection->Item(i, &device))) continue;

            LPWSTR id = nullptr;
            device->GetId(&id);

            ComPtr<IPropertyStore> props;
            if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props))) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                props->GetValue(PKEY_Device_FriendlyName, &varName);

                PROPVARIANT varBT;
                PropVariantInit(&varBT);
                props->GetValue(PKEY_DeviceInterface_FriendlyName, &varBT);

                std::wstring name = (varName.vt == VT_LPWSTR && varName.pwszVal) ? varName.pwszVal : L"Unknown Device";
                std::wstring iface = (varBT.vt == VT_LPWSTR && varBT.pwszVal) ? varBT.pwszVal : L"";

                AudioDeviceInfo info;
                info.id = id ? id : L"";
                info.name = name;
                info.isRender = false;
                info.isCapture = true;
                info.isBluetooth = (iface.find(L"Bluetooth") != std::wstring::npos) ||
                                   (name.find(L"Bluetooth") != std::wstring::npos) ||
                                   (info.id.find(L"BTHENUM") != std::wstring::npos);

                captureDevices_.push_back(info);

                PropVariantClear(&varName);
                PropVariantClear(&varBT);
            }
            if (id) CoTaskMemFree(id);
        }
    }
}

std::vector<AudioDeviceInfo> DeviceManager::GetRenderDevices() const { return renderDevices_; }
std::vector<AudioDeviceInfo> DeviceManager::GetCaptureDevices() const { return captureDevices_; }

HRESULT STDMETHODCALLTYPE DeviceManager::OnDeviceStateChanged(LPCWSTR, DWORD) { Enumerate(); return S_OK; }
HRESULT STDMETHODCALLTYPE DeviceManager::OnDeviceAdded(LPCWSTR) { Enumerate(); return S_OK; }
HRESULT STDMETHODCALLTYPE DeviceManager::OnDeviceRemoved(LPCWSTR) { Enumerate(); return S_OK; }
HRESULT STDMETHODCALLTYPE DeviceManager::OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) { Enumerate(); return S_OK; }
HRESULT STDMETHODCALLTYPE DeviceManager::OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) { Enumerate(); return S_OK; }

ULONG STDMETHODCALLTYPE DeviceManager::AddRef() { return InterlockedIncrement(&refCount_); }
ULONG STDMETHODCALLTYPE DeviceManager::Release() {
    LONG c = InterlockedDecrement(&refCount_);
    if (c == 0) delete this;
    return c;
}
HRESULT STDMETHODCALLTYPE DeviceManager::QueryInterface(REFIID riid, void **ppv) {
    if (riid == __uuidof(IMMNotificationClient) || riid == IID_IUnknown) {
        *ppv = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

#include "sysaudio.h"

SysAudio::SysAudio() :
    _pDeviceEnumerator(nullptr),
    _pPolicyConfig(nullptr)
{

}

SysAudio::~SysAudio()
{
    CoUninitialize();
}

void SysAudio::init()
{
    CoInitialize(nullptr);

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&_pDeviceEnumerator);
    if (hr != S_OK)
    {
        qDebug() << "hr != S_OK: CoCreateInstance(__uuidof(MMDeviceEnumerator): " << Q_FUNC_INFO;
    }

    // for Win 10
    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig2, (LPVOID *)&_pPolicyConfig);
    if(hr != S_OK)
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig1, (LPVOID *)&_pPolicyConfig);

    // for Win Vista, 7, 8, 8.1
    if(hr != S_OK)
    {
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig0, (LPVOID *)&_pPolicyConfig);
        if(hr != S_OK)
        {
            qDebug() << "hr != S_OK: CoCreateInstance(__uuidof(CPolicyConfigClient): " << Q_FUNC_INFO;
        }
    }
}

CComPtr<IMMDevice> SysAudio::getDevice(const QString &deviceId)
{
    CComPtr<IMMDevice> Device;
    HRESULT hr;
    if(deviceId.isEmpty())
    {
        hr = _pDeviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &Device);
        if(hr != S_OK)
        {
            qDebug() << "hr != S_OK: _pDeviceEnumerator->GetDefaultAudioEndpoint: " << Q_FUNC_INFO;
            return nullptr;
        }
    }
    else
    {
        hr = _pDeviceEnumerator->GetDevice(deviceId.toStdWString().c_str(), &Device);
        if(hr != S_OK)
        {
            qDebug() << "hr != S_OK: _pDeviceEnumerator->GetDevice: " << Q_FUNC_INFO;
            return nullptr;
        }
    }

    return Device;
}

CComPtr<IAudioEndpointVolume> SysAudio::getDeviceVolume(const QString &deviceId)
{
    CComPtr<IAudioEndpointVolume> AudioEndpointVolume;
    CComPtr<IMMDevice> Device = getDevice(deviceId);
    HRESULT hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (PVOID *)&AudioEndpointVolume);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: Device->Activate(__uuidof(IAudioEndpointVolume): " << Q_FUNC_INFO;
        return nullptr;
    }

    return AudioEndpointVolume;
}

void SysAudio::setDefaultDevice(const wchar_t *deviceId)
{
    if(!_pPolicyConfig)
    {
        qDebug() << "!_pPolicyConfig: " << Q_FUNC_INFO;
        return;
    }

    _pPolicyConfig->SetDefaultEndpoint(deviceId, eConsole);
    _pPolicyConfig->SetDefaultEndpoint(deviceId, eMultimedia);
    _pPolicyConfig->SetDefaultEndpoint(deviceId, eCommunications);
}

bool SysAudio::setEndpointVisibility(const wchar_t *deviceId, bool isEnabled)
{
    if(!_pPolicyConfig)
    {
        qDebug() << "!_pPolicyConfig: " << Q_FUNC_INFO;
        return false;
    }

    HRESULT hr = _pPolicyConfig->SetEndpointVisibility(deviceId, (int)isEnabled);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: _pPolicyConfig->SetEndpointVisibility: " << Q_FUNC_INFO;
        return false;
    }

    return true;
}

bool SysAudio::getPropertyValue(const wchar_t *deviceId, const PROPERTYKEY &propertyKey, QVariant &outValue)
{
    if(!_pPolicyConfig)
    {
        qDebug() << "!_pPolicyConfig: " << Q_FUNC_INFO;
        return false;
    }

    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);
    HRESULT hr = _pPolicyConfig->GetPropertyValue(deviceId, 0, propertyKey, &propVariant);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: _pPolicyConfig->GetPropertyValue: " << Q_FUNC_INFO;
        PropVariantClear(&propVariant);
        return false;
    }

    switch(propVariant.vt)
    {
        case VT_BOOL:
            outValue = QVariant::fromValue(propVariant.boolVal);
            break;
        case VT_LPWSTR:
            outValue = QVariant::fromValue(QString::fromWCharArray(propVariant.pwszVal));
            break;
        default:
            qDebug() << "Unsupported type for propVariant.vt: " << Q_FUNC_INFO;
            return false;
    }

    PropVariantClear(&propVariant);

    return true;
}

bool SysAudio::setPropertyValue(const wchar_t *deviceId, const PROPERTYKEY &propertyKey, const QVariant &value)
{
    if(!_pPolicyConfig)
    {
        qDebug() << "!_pPolicyConfig: " << Q_FUNC_INFO;
        return false;
    }

    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);

    switch(value.type())
    {
        case QVariant::Bool:
            InitPropVariantFromBoolean(value.toBool(), &propVariant);
            break;
        case QVariant::String:
            InitPropVariantFromString(value.toString().toStdWString().c_str(), &propVariant);
            break;
        default:
            qDebug() << "Unsupported type for value.type: " << Q_FUNC_INFO;
            return false;
    }


    HRESULT hr = _pPolicyConfig->SetPropertyValue(deviceId, 0, propertyKey, &propVariant);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: _pPolicyConfig->SetPropertyValue: " << Q_FUNC_INFO;
        PropVariantClear(&propVariant);
        return false;
    }

    PropVariantClear(&propVariant);

    return true;
}

QHash<QString, QString> SysAudio::getDevices(EDataFlow dataFlow, DWORD dwStateMask)
{
    CComPtr<IMMDeviceCollection> DeviceCollection;
    HRESULT hr = _pDeviceEnumerator->EnumAudioEndpoints(dataFlow, dwStateMask, &DeviceCollection);
    if (hr != S_OK)
    {
        qDebug() << "hr != S_OK: _pDeviceEnumerator->EnumAudioEndpoints: " << Q_FUNC_INFO;
        return QHash<QString, QString>();
    }

    UINT Count = 0;
    DeviceCollection->GetCount(&Count);

    QHash<QString, QString> Devices;

    for (UINT i=0; i < Count; i++)
    {
        CComPtr<IMMDevice> Device;
        CComPtr<IPropertyStore> PropertyStore;

        DeviceCollection->Item(i, &Device);
        Device->OpenPropertyStore(STGM_READ, &PropertyStore);

        PROPVARIANT propDeviceName;
        PropVariantInit(&propDeviceName);
        PropertyStore->GetValue(PKEY_Device_FriendlyName, &propDeviceName);

        LPWSTR deviceId = nullptr;
        Device->GetId(&deviceId);

        const QString deviceIdStr = QString::fromWCharArray(deviceId);
        const QString deviceNameStr = QString::fromWCharArray(propDeviceName.pwszVal);

        Devices.insert(deviceNameStr, deviceIdStr);

        CoTaskMemFree(deviceId);
        deviceId = nullptr;

        PropVariantClear(&propDeviceName);
    }

    return Devices;
}

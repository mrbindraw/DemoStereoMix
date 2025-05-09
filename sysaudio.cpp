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

CComPtr<IMMDevice> SysAudio::getDefaultDevice(EDataFlow dataFlow)
{
    CComPtr<IMMDevice> Device;
    HRESULT hr = _pDeviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &Device);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: _pDeviceEnumerator->GetDefaultAudioEndpoint: " << Q_FUNC_INFO;
        return nullptr;
    }

    return Device;
}

QString SysAudio::getDeviceName(const CComPtr<IMMDevice> &device) const
{
    if(!device)
    {
        Q_ASSERT_X(device, Q_FUNC_INFO, "device is nullptr!");
        return QString();
    }

    CComPtr<IPropertyStore> PropertyStore;
    device->OpenPropertyStore(STGM_READ, &PropertyStore);

    PROPVARIANT propDeviceName;
    PropVariantInit(&propDeviceName);
    PropertyStore->GetValue(PKEY_Device_FriendlyName, &propDeviceName);

    const QString deviceName = QString::fromWCharArray(propDeviceName.pwszVal);
    PropVariantClear(&propDeviceName);

    return deviceName;
}

QString SysAudio::getDeviceId(const CComPtr<IMMDevice> &device) const
{
    if(!device)
    {
        Q_ASSERT_X(device, Q_FUNC_INFO, "device is nullptr!");
        return QString();
    }

    LPWSTR deviceId = nullptr;
    device->GetId(&deviceId);

    const QString deviceIdStr = QString::fromWCharArray(deviceId);

    CoTaskMemFree(deviceId);
    deviceId = nullptr;

    return deviceIdStr;
}

bool SysAudio::isDeviceEnabled(const CComPtr<IMMDevice> &device) const
{
    if(!device)
    {
        Q_ASSERT_X(device, Q_FUNC_INFO, "device is nullptr!");
        return false;
    }

    DWORD stateDevice = 0;
    device->GetState(&stateDevice);
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd371410(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd370823(v=vs.85).aspx
    return stateDevice >= 2 ? false : true;
}

bool SysAudio::isDevicePowerSaveEnabled(const CComPtr<IMMDevice> &device) const
{
    if(!device)
    {
        Q_ASSERT_X(device, Q_FUNC_INFO, "device is nullptr!");
        return false;
    }

    PROPVARIANT PowerMgrState;
    PropVariantInit(&PowerMgrState);

    CComPtr<IPropertyStore> PropertyStore;
    device->OpenPropertyStore(STGM_READ, &PropertyStore);
    PropertyStore->GetValue(PKEY_MonitorPauseOnBattery, &PowerMgrState);
    const bool isPowerSaveEnabled = PowerMgrState.boolVal;
    PropVariantClear(&PowerMgrState);

    return isPowerSaveEnabled;
}

bool SysAudio::isListenDevice(const QString &deviceId) const
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return false;
    }

    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(deviceId.toStdWString().c_str(), PKEY_MonitorEnabled, outValue))
    {
        qDebug() << "!SysAudio::getInstance().getPropertyValue: " << Q_FUNC_INFO;
        return false;
    }

    return outValue.value<bool>();
}

CComPtr<IMMDevice> SysAudio::getDevice(const QString &deviceId)
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return nullptr;
    }

    CComPtr<IMMDevice> Device;
    HRESULT hr = _pDeviceEnumerator->GetDevice(deviceId.toStdWString().c_str(), &Device);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: _pDeviceEnumerator->GetDevice: " << Q_FUNC_INFO;
        return nullptr;
    }

    return Device;
}

CComPtr<IMMDevice> SysAudio::getDevice(EDataFlow dataFlow, const QString &deviceName)
{
    if(deviceName.isEmpty())
    {
        Q_ASSERT_X(!deviceName.isEmpty(), Q_FUNC_INFO, "deviceName is empty!");
        return nullptr;
    }

    CComPtr<IMMDevice> device = getDefaultDevice(dataFlow);
    if(device)
    {
        const QString devName = getDeviceName(device);
        if(devName.contains(deviceName))
        {
            return device;
        }
    }

    const auto &devices = SysAudio::getInstance().getDevices(dataFlow, DEVICE_STATEMASK_ALL);
    for(const auto &devName : devices.keys())
    {
        if(devName.contains(deviceName))
        {
            return getDevice(devices[devName]);
        }
    }

    return nullptr;
}

CComPtr<IAudioEndpointVolume> SysAudio::getDeviceVolume(const QString &deviceId)
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return nullptr;
    }

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
        case VT_EMPTY:
            case VT_NULL:
                outValue = QVariant();
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
        DeviceCollection->Item(i, &Device);

        const QString deviceIdStr = getDeviceId(Device);
        const QString deviceNameStr = getDeviceName(Device);

        Devices.insert(deviceNameStr, deviceIdStr);
    }

    return Devices;
}

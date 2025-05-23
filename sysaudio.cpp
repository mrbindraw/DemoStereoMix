#include "sysaudio.h"

SysAudio::SysAudio() :
    deviceEnumerator(nullptr),
    policyConfig(nullptr)
{

}

SysAudio::~SysAudio()
{
    CoUninitialize();
}

void SysAudio::init()
{
    CoInitialize(nullptr);

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    if (hr != S_OK)
    {
        qDebug() << "hr != S_OK: CoCreateInstance(__uuidof(MMDeviceEnumerator): " << Q_FUNC_INFO;
    }

    // for Win 10
    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_INPROC, IID_IPolicyConfig2, (LPVOID *)&policyConfig);
    if(hr != S_OK)
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_INPROC, IID_IPolicyConfig1, (LPVOID *)&policyConfig);

    // for Win Vista, 7, 8, 8.1
    if(hr != S_OK)
    {
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_INPROC, IID_IPolicyConfig0, (LPVOID *)&policyConfig);
        if(hr != S_OK)
        {
            qDebug() << "hr != S_OK: CoCreateInstance(__uuidof(CPolicyConfigClient): " << Q_FUNC_INFO;
        }
    }
}

CComPtr<IMMDevice> SysAudio::getDefaultDevice(EDataFlow dataFlow)
{
    CComPtr<IMMDevice> device;
    HRESULT hr = deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &device);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: deviceEnumerator->GetDefaultAudioEndpoint: " << Q_FUNC_INFO;
        return nullptr;
    }

    return device;
}

QString SysAudio::getDeviceName(const CComPtr<IMMDevice> &device) const
{
    if(!device)
    {
        Q_ASSERT_X(device, Q_FUNC_INFO, "device is nullptr!");
        return QString();
    }

    CComPtr<IPropertyStore> propertyStore;
    device->OpenPropertyStore(STGM_READ, &propertyStore);

    PROPVARIANT propDeviceName;
    PropVariantInit(&propDeviceName);
    propertyStore->GetValue(PKEY_Device_FriendlyName, &propDeviceName);

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

    PROPVARIANT powerMgrState;
    PropVariantInit(&powerMgrState);

    CComPtr<IPropertyStore> propertyStore;
    device->OpenPropertyStore(STGM_READ, &propertyStore);
    propertyStore->GetValue(PKEY_MonitorPauseOnBattery, &powerMgrState);
    const bool isPowerSaveEnabled = powerMgrState.boolVal;
    PropVariantClear(&powerMgrState);

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
    if(!getPropertyValue(deviceId, PKEY_MonitorEnabled, outValue))
    {
        qDebug() << "!getPropertyValue: " << Q_FUNC_INFO;
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

    CComPtr<IMMDevice> device;
    HRESULT hr = deviceEnumerator->GetDevice(deviceId.toStdWString().c_str(), &device);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: deviceEnumerator->GetDevice: " << Q_FUNC_INFO;
        return nullptr;
    }

    return device;
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

    const auto &devices = getDevices(dataFlow, DEVICE_STATEMASK_ALL);
    const auto &names = devices.keys();
    for(const auto &name : names)
    {
        if(name.contains(deviceName))
        {
            return getDevice(devices[name]);
        }
    }

    return nullptr;
}

CComPtr<IAudioEndpointVolume> SysAudio::getAudioEndpointVolume(const QString &deviceId)
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return nullptr;
    }

    CComPtr<IAudioEndpointVolume> audioEndpointVolume;
    CComPtr<IMMDevice> device = getDevice(deviceId);
    HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (PVOID *)&audioEndpointVolume);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: device->Activate(__uuidof(IAudioEndpointVolume): " << Q_FUNC_INFO;
        return nullptr;
    }

    return audioEndpointVolume;
}

bool SysAudio::setDeviceVolume(const QString &deviceId, int volume)
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return false;
    }

    CComPtr<IAudioEndpointVolume> audioEndpointVolume = getAudioEndpointVolume(deviceId);
    if(!audioEndpointVolume)
    {
        qDebug() << "!audioEndpointVolume: " << Q_FUNC_INFO;
        return false;
    }

    float volumeScalar = getScalarFromValue(volume);
    HRESULT hr = audioEndpointVolume->SetMasterVolumeLevelScalar(volumeScalar, nullptr);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: audioEndpointVolume->SetMasterVolumeLevelScalar: " << Q_FUNC_INFO;
        return false;
    }

    return true;
}

int SysAudio::getDeviceVolume(const QString &deviceId)
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return 0;
    }

    CComPtr<IAudioEndpointVolume> audioEndpointVolume = getAudioEndpointVolume(deviceId);
    if(!audioEndpointVolume)
    {
        qDebug() << "!audioEndpointVolume: " << Q_FUNC_INFO;
        return 0;
    }

    float volumeScalar = 0.0f;
    HRESULT hr = audioEndpointVolume->GetMasterVolumeLevelScalar(&volumeScalar);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: audioEndpointVolume->GetMasterVolumeLevelScalar: " << Q_FUNC_INFO;
        return 0;
    }

    return getValueFromScalar(volumeScalar);
}

void SysAudio::setDefaultDevice(const QString &deviceId)
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return;
    }

    if(!policyConfig)
    {
        qDebug() << "!policyConfig: " << Q_FUNC_INFO;
        return;
    }

    policyConfig->SetDefaultEndpoint(deviceId.toStdWString().c_str(), eConsole);
    policyConfig->SetDefaultEndpoint(deviceId.toStdWString().c_str(), eMultimedia);
    policyConfig->SetDefaultEndpoint(deviceId.toStdWString().c_str(), eCommunications);
}

bool SysAudio::setEndpointVisibility(const QString &deviceId, bool isEnabled) const
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return false;
    }

    if(!policyConfig)
    {
        qDebug() << "policyConfig: " << Q_FUNC_INFO;
        return false;
    }

    HRESULT hr = policyConfig->SetEndpointVisibility(deviceId.toStdWString().c_str(), (int)isEnabled);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: policyConfig->SetEndpointVisibility: " << Q_FUNC_INFO;
        return false;
    }

    return true;
}

bool SysAudio::getPropertyValue(const QString &deviceId, const PROPERTYKEY &propertyKey, QVariant &outValue) const
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return false;
    }

    if(!policyConfig)
    {
        qDebug() << "!policyConfig: " << Q_FUNC_INFO;
        return false;
    }

    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);
    HRESULT hr = policyConfig->GetPropertyValue(deviceId.toStdWString().c_str(), 0, propertyKey, &propVariant);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: policyConfig->GetPropertyValue: " << Q_FUNC_INFO;
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

bool SysAudio::setPropertyValue(const QString &deviceId, const PROPERTYKEY &propertyKey, const QVariant &value) const
{
    if(deviceId.isEmpty())
    {
        Q_ASSERT_X(!deviceId.isEmpty(), Q_FUNC_INFO, "deviceId is empty!");
        return false;
    }

    if(!policyConfig)
    {
        qDebug() << "!policyConfig: " << Q_FUNC_INFO;
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


    HRESULT hr = policyConfig->SetPropertyValue(deviceId.toStdWString().c_str(), 0, propertyKey, &propVariant);
    if(hr != S_OK)
    {
        qDebug() << "hr != S_OK: policyConfig->SetPropertyValue: " << Q_FUNC_INFO;
        PropVariantClear(&propVariant);
        return false;
    }

    PropVariantClear(&propVariant);

    return true;
}

QHash<QString, QString> SysAudio::getDevices(EDataFlow dataFlow, DWORD dwStateMask)
{
    CComPtr<IMMDeviceCollection> deviceCollection;
    HRESULT hr = deviceEnumerator->EnumAudioEndpoints(dataFlow, dwStateMask, &deviceCollection);
    if (hr != S_OK)
    {
        qDebug() << "hr != S_OK: deviceEnumerator->EnumAudioEndpoints: " << Q_FUNC_INFO;
        return QHash<QString, QString>();
    }

    UINT count = 0;
    deviceCollection->GetCount(&count);

    QHash<QString, QString> devices;

    for (UINT i=0; i < count; i++)
    {
        CComPtr<IMMDevice> device;
        deviceCollection->Item(i, &device);

        const QString deviceIdStr = getDeviceId(device);
        const QString deviceNameStr = getDeviceName(device);

        devices.insert(deviceNameStr, deviceIdStr);
    }

    return devices;
}

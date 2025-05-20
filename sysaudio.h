#ifndef SYSAUDIO_H
#define SYSAUDIO_H

#include <initguid.h>
#include <Windows.h>
#include <cguid.h>
#include <atlbase.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "PolicyConfig.h"
#include <Objbase.h>
#include <Shlwapi.h>
#include <Propvarutil.h>

// from mmsys.pdb (mmsys.cpl)
DEFINE_PROPERTYKEY(PKEY_MonitorOutput, 0x24dbb0fc, 0x9311, 0x4b3d, 0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4, 0);          // Playback device key
DEFINE_PROPERTYKEY(PKEY_MonitorEnabled, 0x24dbb0fc, 0x9311, 0x4b3d, 0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4, 1);         // Listen checker state
DEFINE_PROPERTYKEY(PKEY_MonitorPauseOnBattery, 0x24dbb0fc, 0x9311, 0x4b3d, 0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4, 2);  // PowerMgr mode

#include <QString>
#include <QDebug>
#include <QVariant>

class SysAudio
{
private:
    SysAudio();

    CComPtr<IMMDeviceEnumerator> deviceEnumerator;
    CComPtr<IPolicyConfig>  policyConfig;

public:
    static SysAudio &getInstance()
    {
        static SysAudio instance;
        return instance;
    }

    SysAudio(SysAudio const &) = delete;
    SysAudio &operator = (SysAudio const &) = delete;

    ~SysAudio();

    void init();

    CComPtr<IMMDevice> getDefaultDevice(EDataFlow dataFlow);
    QString getDeviceName(const CComPtr<IMMDevice> &device) const;
    QString getDeviceId(const CComPtr<IMMDevice> &device) const;
    bool isDeviceEnabled(const CComPtr<IMMDevice> &device) const;
    bool isDevicePowerSaveEnabled(const CComPtr<IMMDevice> &device) const;
    bool isListenDevice(const QString &deviceId) const;
    CComPtr<IMMDevice> getDevice(const QString &deviceId);
    CComPtr<IMMDevice> getDevice(EDataFlow dataFlow, const QString &deviceName);
    CComPtr<IAudioEndpointVolume> getAudioEndpointVolume(const QString &deviceId);
    bool setDeviceVolume(const QString &deviceId, int volume);
    int getDeviceVolume(const QString &deviceId);
    void setDefaultDevice(const QString &deviceId);
    bool setEndpointVisibility(const QString &deviceId, bool isEnabled) const;
    bool getPropertyValue(const QString &deviceId, const PROPERTYKEY &propertyKey, QVariant &outValue) const;
    bool setPropertyValue(const QString &deviceId, const PROPERTYKEY &propertyKey, const QVariant &value) const;
    QHash<QString, QString> getDevices(EDataFlow dataFlow, DWORD dwStateMask);

    // utils
    inline static float getScalarFromValue(int value)
    {
        return qAbs(value) >= 100.0f ? 1.0f : value / 100.0f;
    }

    inline static int getValueFromScalar(float value)
    {
        return qRound(qAbs(value) > 0.0f ? value * 100.0f : 0.0f);
    }
};

#endif // SYSAUDIO_H

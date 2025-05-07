#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    _pDeviceEnumerator(nullptr),
    _pPolicyConfig(nullptr),
    _isEnabledDevice(false),
    _isListenSM(false),
    _isPowerSaveEnabled(true),
    _isAppLoading(true)
{
    ui->setupUi(this);

    CoInitialize(nullptr);
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&_pDeviceEnumerator);
    if (hr != S_OK)
        return;


    // for Win 10
    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig2, (LPVOID *)&_pPolicyConfig);
    if(hr != S_OK)
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig1, (LPVOID *)&_pPolicyConfig);

    // for Win Vista, 7, 8, 8.1
    if(hr != S_OK)
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig0, (LPVOID *)&_pPolicyConfig);

    if(hr != S_OK)
        return;


    this->createPlaybackDevicesList();

    this->getStereoMixInfo();

    //this->setDefaultRecordDevice(_wstrSMDevId.c_str()); // set StereoMix

    /*
    CComPtr<IMMDevice> Device;
    hr = _pDeviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &Device);
    if(hr != S_OK) // when all record devices off on load app
        return;

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume;
    hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (PVOID *)&_pAudioEndpointVolume);
    if(hr != S_OK)
        return;
    */
}

void Widget::showEvent(QShowEvent *)
{
    this->refreshStereoMixVolume();

    this->getCurrentPlaybackDevice();

    ui->cbEnableSM->setChecked(_isEnabledDevice);
    ui->cbListen->setChecked(_isListenSM);
    _isPowerSaveEnabled ? ui->rbDisable->setChecked(true) : ui->rbContinue->setChecked(true);

    _isAppLoading = false;
}

void Widget::getCurrentPlaybackDevice()
{
    PROPVARIANT propDevId;
    PropVariantInit(&propDevId);

    if(!_pPolicyConfig)
        return;

    HRESULT hr = _pPolicyConfig->GetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorOutput, &propDevId);
    if(hr != S_OK)
    {
        PropVariantClear(&propDevId);
        return;
    }

    if(!propDevId.pwszVal)
    {
        PropVariantClear(&propDevId);
        return;
    }

    std::wstring id;
    id.assign(propDevId.pwszVal);

    for(int i=0; i < _listAudioDevices.size(); i++)
    {
        if(_listAudioDevices.value(i).id.compare(id)==0)
        {
            ui->cBox_AudioDevices->setCurrentText(_listAudioDevices.value(i).name);
            break;
        }
    }

    PropVariantClear(&propDevId);
}

void Widget::createPlaybackDevicesList()
{
    CComPtr<IMMDeviceCollection> DeviceCollection;
    HRESULT hr = _pDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &DeviceCollection);
    if (hr != S_OK)
        return;

    UINT Count = 0;
    DeviceCollection->GetCount(&Count);

    LPWSTR deviceId = nullptr;
    QString name = "";

    ui->cBox_AudioDevices->insertItem(0, "Default Playback Device");

    CComPtr<IMMDevice> Device;
    CComPtr<IPropertyStore> PropertyStore;

    for (UINT i=0; i < Count; i++)
    {
        DeviceCollection->Item(i, &Device);
        Device->OpenPropertyStore(STGM_READ, &PropertyStore);

        PROPVARIANT namePlaybackDevice;
        PropVariantInit(&namePlaybackDevice);
        PropertyStore->GetValue(PKEY_Device_FriendlyName, &namePlaybackDevice);

        name = QString::fromWCharArray(namePlaybackDevice.pwszVal);
        Device->GetId(&deviceId);

        _stAudioDevice.name = name;
        _stAudioDevice.id.assign(deviceId);
        ui->cBox_AudioDevices->addItem(_stAudioDevice.name);
        _listAudioDevices.append(_stAudioDevice);

        CoTaskMemFree(deviceId);
        deviceId = nullptr;

        PropVariantClear(&namePlaybackDevice);
    }
}

void Widget::setDefaultRecordDevice(const wchar_t *id)
{
    if(!_pPolicyConfig)
        return;

    _pPolicyConfig->SetDefaultEndpoint(id, eConsole);
    _pPolicyConfig->SetDefaultEndpoint(id, eMultimedia);
    _pPolicyConfig->SetDefaultEndpoint(id, eCommunications);
}

void Widget::getStereoMixInfo()
{
    CComPtr<IMMDeviceCollection> DeviceCollection;
    HRESULT hr = _pDeviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATEMASK_ALL, &DeviceCollection);
    if (hr != S_OK)
        return;

    UINT Count = 0;
    DeviceCollection->GetCount(&Count);
    DWORD stateDevice = 0;
    LPWSTR deviceId = nullptr;
    QString name = "";

    CComPtr<IMMDevice> Device;
    CComPtr<IPropertyStore> PropertyStore;

    for (UINT i=0; i < Count; i++)
    {
        DeviceCollection->Item(i, &Device);
        Device->OpenPropertyStore(STGM_READ, &PropertyStore);

        PROPVARIANT nameDevice;
        PropVariantInit(&nameDevice);
        PropertyStore->GetValue(PKEY_Device_FriendlyName, &nameDevice);

        name = QString::fromWCharArray(nameDevice.pwszVal);
        if(name.contains("Stereo Mix") || name.contains("Стерео микшер"))
        {
            Device->GetState(&stateDevice);
            // https://msdn.microsoft.com/en-us/library/windows/desktop/dd371410(v=vs.85).aspx
            // https://msdn.microsoft.com/en-us/library/windows/desktop/dd370823(v=vs.85).aspx
            _isEnabledDevice = stateDevice >= 2 ? false : true;

            PROPVARIANT PowerMgrState;
            PropVariantInit(&PowerMgrState);
            PropertyStore->GetValue(PKEY_MonitorPauseOnBattery, &PowerMgrState);
            _isPowerSaveEnabled = PowerMgrState.boolVal;
            PropVariantClear(&PowerMgrState);

            Device->GetId(&deviceId);
            _wstrSMDevId.assign(deviceId);

            CoTaskMemFree(deviceId);
            deviceId = nullptr;
        }

        PropVariantClear(&nameDevice);
    }

    if(!_pPolicyConfig)
        return;

    PROPVARIANT valKey;
    PropVariantInit(&valKey);
    hr = _pPolicyConfig->GetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorEnabled, &valKey);
    if(hr != S_OK)
        return;

    _isListenSM = valKey.boolVal;
}

void Widget::refreshStereoMixVolume()
{
    this->setDefaultRecordDevice(_wstrSMDevId.c_str()); // set StereoMix

    CComPtr<IMMDevice> Device;
    HRESULT hr = _pDeviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &Device);
    if(hr != S_OK)
        return;

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume;
    hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (PVOID *)&AudioEndpointVolume);
    if(hr != S_OK)
        return;

    if(AudioEndpointVolume)
    {
        float scalarVolume = 0.0f;
        AudioEndpointVolume->GetMasterVolumeLevelScalar(&scalarVolume);
        float volume = getValueFromScalar(scalarVolume);
        ui->horizontalSlider->setValue(volume);
        ui->lbl_Value->setText(QString::number(volume));
    }
}

// utils
float Widget::getScalarFromValue(unsigned int value)
{
    return value >= 100.0f ? 1.0f : value / 100.0f;
}

unsigned int Widget::getValueFromScalar(float value)
{
    return (unsigned int)(value >= 1.0f ? 100.0f : value * 100.0f);
}

// change volume StereoMix
void Widget::on_horizontalSlider_valueChanged(int value)
{
    CComPtr<IMMDevice> Device;
    HRESULT hr = _pDeviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &Device);
    if(hr != S_OK)
        return;

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume;
    hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (PVOID *)&AudioEndpointVolume);
    if(hr != S_OK)
        return;

    if(AudioEndpointVolume)
    {
        float val = this->getScalarFromValue(value);
        AudioEndpointVolume->SetMasterVolumeLevelScalar(val, nullptr);
        ui->lbl_Value->setText(QString::number(value));
    }
}

// change Listen checker state
void Widget::on_cbListen_toggled(bool checked)
{
    if(_isAppLoading)
        return;

    qDebug() << Q_FUNC_INFO << checked;

    if(!_pPolicyConfig)
        return;

    int val = 0;
    checked ? val = -1 : val = 0;

    PROPVARIANT valKey;
    PropVariantInit(&valKey);

    // TRUE -1 - on; FALSE 0 - off
    InitPropVariantFromBoolean(checked, &valKey);
    HRESULT hr = _pPolicyConfig->SetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorEnabled, &valKey);
    if(hr != S_OK)
        return;

    PropVariantClear(&valKey);
}

// Enable/Disable StereoMix device
void Widget::on_cbEnableSM_toggled(bool checked)
{
    if(_isAppLoading)
        return;

    qDebug() << Q_FUNC_INFO << checked;

    if(!_pPolicyConfig)
        return;

    HRESULT hr = _pPolicyConfig->SetEndpointVisibility(_wstrSMDevId.c_str(), (int)checked); // 0/1 - disable/enable StereoMix device
    if(hr != S_OK)
        return;

    if(checked) // when load app and StereoMix off
        this->refreshStereoMixVolume();
}

// choose Playback audio device
void Widget::on_cBox_AudioDevices_activated(int index)
{
    qDebug() << index;

    if(!_pPolicyConfig)
        return;

    HRESULT hr;
    PROPVARIANT propDevId;
    PropVariantInit(&propDevId);

    if(index > 0)
    {
        InitPropVariantFromString(_listAudioDevices.value(index-1).id.c_str(), &propDevId);
        hr = _pPolicyConfig->SetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorOutput, &propDevId);
        if(hr != S_OK)
            return;
    }
    else
    {
        hr = _pPolicyConfig->SetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorOutput, &propDevId);
        if(hr != S_OK)
            return;
    }

    PropVariantClear(&propDevId);
}

// Power Management control mode
void Widget::on_rbDisable_toggled(bool checked)
{
    if(_isAppLoading)
        return;

    if(!checked) // don't call twice SetPropertyValue when change state Power Management
        return;

    qDebug() << Q_FUNC_INFO << checked;

    UNREFERENCED_PARAMETER(checked);

    if(!_pPolicyConfig)
        return;

    PROPVARIANT valKey;
    PropVariantInit(&valKey);

    InitPropVariantFromBoolean(TRUE, &valKey);
    HRESULT hr = _pPolicyConfig->SetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorPauseOnBattery, &valKey);
    if(hr != S_OK)
        return;

    PropVariantClear(&valKey);
}

void Widget::on_rbContinue_toggled(bool checked)
{
    if(_isAppLoading)
        return;

    if(!checked) // don't call twice SetPropertyValue when change state Power Management
        return;

    qDebug() << Q_FUNC_INFO << checked;

    if(!_pPolicyConfig)
        return;

    PROPVARIANT valKey;
    PropVariantInit(&valKey);

    InitPropVariantFromBoolean(FALSE, &valKey);
    HRESULT hr = _pPolicyConfig->SetPropertyValue(_wstrSMDevId.c_str(), 0, PKEY_MonitorPauseOnBattery, &valKey);
    if(hr != S_OK)
        return;

    PropVariantClear(&valKey);
}

Widget::~Widget()
{
    CoUninitialize();

    delete ui;
}


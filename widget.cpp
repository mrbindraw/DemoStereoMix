#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    _isEnabledDevice(false),
    _isListenSM(false),
    _isPowerSaveEnabled(true),
    _isAppLoading(true)
{
    ui->setupUi(this);

    SysAudio::getInstance().init();

    ui->cBox_AudioDevices->insertItem(0, "Default Playback Device");

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    ui->cBox_AudioDevices->addItems(devices.keys());

    this->getStereoMixInfo();
}

void Widget::showEvent(QShowEvent *)
{
    this->refreshStereoMixVolume();

    this->getCurrentPlaybackDevice();

    ui->cbEnableSM->setChecked(_isEnabledDevice);
    ui->cbListen->setChecked(_isListenSM);
    _isPowerSaveEnabled ? ui->rbDisable->setChecked(true) : ui->rbContinue->setChecked(true);

    _isAppLoading = false;

    on_cbEnableSM_toggled(_isEnabledDevice);
}

void Widget::getCurrentPlaybackDevice()
{
    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(_wstrSMDevId.c_str(), PKEY_MonitorOutput, outValue))
    {
        return;
    }

    const QString deviceIdValue = outValue.toString();
    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    for (auto it = devices.cbegin(); it != devices.cend(); ++it)
    {
        if(it.value() == deviceIdValue)
        {
            ui->cBox_AudioDevices->setCurrentText(it.key());
            break;
        }
    }
}

void Widget::getStereoMixInfo()
{
    CComPtr<IMMDevice> device = SysAudio::getInstance().getDevice(EDataFlow::eCapture, "Stereo Mix");
    if(!device)
    {
        device = SysAudio::getInstance().getDevice(EDataFlow::eCapture, "Стерео микшер");
    }

    if(!device)
    {
        qDebug() << "ERROR!: Stereo Mix device not found!" << Q_FUNC_INFO;
        return;
    }

    LPWSTR deviceId = nullptr;
    device->GetId(&deviceId);
    _wstrSMDevId.assign(deviceId);
    CoTaskMemFree(deviceId);
    deviceId = nullptr;

    DWORD stateDevice = 0;
    device->GetState(&stateDevice);
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd371410(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd370823(v=vs.85).aspx
    _isEnabledDevice = stateDevice >= 2 ? false : true;

    PROPVARIANT PowerMgrState;
    PropVariantInit(&PowerMgrState);

    CComPtr<IPropertyStore> PropertyStore;
    device->OpenPropertyStore(STGM_READ, &PropertyStore);
    PropertyStore->GetValue(PKEY_MonitorPauseOnBattery, &PowerMgrState);
    _isPowerSaveEnabled = PowerMgrState.boolVal;
    PropVariantClear(&PowerMgrState);

    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(_wstrSMDevId.c_str(), PKEY_MonitorEnabled, outValue))
    {
        return;
    }

    _isListenSM = outValue.value<bool>();
}

void Widget::refreshStereoMixVolume()
{
    if(!_isEnabledDevice)
    {
        ui->horizontalSlider->setValue(0);
        ui->lbl_Value->setText(QString::number(0));
        return;
    }

    SysAudio::getInstance().setDefaultDevice(_wstrSMDevId.c_str()); // set StereoMix

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume = SysAudio::getInstance().getDeviceVolume();
    if(!AudioEndpointVolume)
    {
        qDebug() << "!AudioEndpointVolume: " << Q_FUNC_INFO;
        return;
    }

    float scalarVolume = 0.0f;
    AudioEndpointVolume->GetMasterVolumeLevelScalar(&scalarVolume);
    float volume = SysAudio::getValueFromScalar(scalarVolume);
    ui->horizontalSlider->setValue(volume);
    ui->lbl_Value->setText(QString::number(volume));
}

// Enable/Disable StereoMix device
void Widget::on_cbEnableSM_toggled(bool checked)
{
    _isEnabledDevice = checked;

    if(_isAppLoading)
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    ui->cbListen->setEnabled(checked);
    ui->cBox_AudioDevices->setEnabled(checked);
    ui->rbContinue->setEnabled(checked);
    ui->rbDisable->setEnabled(checked);

    if(!SysAudio::getInstance().setEndpointVisibility(_wstrSMDevId.c_str(), (int)checked)) // 0/1 - disable/enable StereoMix device
    {
        return;
    }

    this->refreshStereoMixVolume();
}

// Change volume StereoMix
void Widget::on_horizontalSlider_valueChanged(int value)
{
    if(!_isEnabledDevice)
    {
        return;
    }

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume = SysAudio::getInstance().getDeviceVolume();
    if(!AudioEndpointVolume)
    {
        qDebug() << "!AudioEndpointVolume: " << Q_FUNC_INFO;
        return;
    }

    float val = SysAudio::getScalarFromValue(value);
    AudioEndpointVolume->SetMasterVolumeLevelScalar(val, nullptr);
    ui->lbl_Value->setText(QString::number(value));
}

// Change Listen checker state
void Widget::on_cbListen_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(_wstrSMDevId.c_str(), PKEY_MonitorEnabled, QVariant::fromValue(checked));
}

// Choose Playback audio device
void Widget::on_cBox_AudioDevices_activated(int index)
{
    qDebug() << index;

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    const auto &deviceId = devices[ui->cBox_AudioDevices->currentText()];
    const QVariant varValue = QVariant::fromValue(deviceId);
    SysAudio::getInstance().setPropertyValue(_wstrSMDevId.c_str(), PKEY_MonitorOutput, varValue);
}

// Power Management control mode
void Widget::on_rbContinue_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    if(!checked) // don't call twice SetPropertyValue when change state Power Management
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(_wstrSMDevId.c_str(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(false));
}

void Widget::on_rbDisable_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    if(!checked) // don't call twice SetPropertyValue when change state Power Management
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(_wstrSMDevId.c_str(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(true));
}

Widget::~Widget()
{
    delete ui;
}


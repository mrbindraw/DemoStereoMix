#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    _isAppLoading(true)
{
    ui->setupUi(this);

    connect(ui->cbEnableSM, &QCheckBox::toggled, this, &Widget::handleDeviceEnableOnToggled);
    connect(ui->horizontalSlider, &QSlider::valueChanged, this, &Widget::handleDeviceVolumeOnValueChanged);
    connect(ui->cbListen, &QCheckBox::toggled, this, &Widget::handleDeviceListenOnToggled);
    connect(ui->cBox_AudioDevices, &QComboBox::activated, this, &Widget::handleDevicePlaybackOnActivated);
    connect(ui->rbContinue, &QRadioButton::toggled, this, &Widget::handlePowerMgrContinueOnToggled);
    connect(ui->rbDisable, &QRadioButton::toggled, this, &Widget::handlePowerMgrDisableOnToggled);

    SysAudio::getInstance().init();

    ui->cBox_AudioDevices->insertItem(0, "Default Playback Device");

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    ui->cBox_AudioDevices->addItems(devices.keys());
}

void Widget::showEvent(QShowEvent *)
{
    const bool isDeviceEnabled = SysAudio::getInstance().isDeviceEnabled(getStereoMixDevice());

    ui->cbEnableSM->setChecked(isDeviceEnabled);

    _isAppLoading = false;

    handleDeviceEnableOnToggled(isDeviceEnabled);
}

QString Widget::getPlaybackDeviceName() const
{
    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(getStereoMixDeviceId(), PKEY_MonitorOutput, outValue))
    {
        qDebug() << "!SysAudio::getInstance().getPropertyValue: " << Q_FUNC_INFO;
        return QString();
    }

    const QString deviceIdValue = outValue.toString();
    if(deviceIdValue.isEmpty())
    {
        return QString();
    }

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    for (auto it = devices.cbegin(); it != devices.cend(); ++it)
    {
        if(it.value() == deviceIdValue)
        {
            return it.key();
        }
    }

    return QString();
}

CComPtr<IMMDevice> Widget::getStereoMixDevice() const
{
    CComPtr<IMMDevice> device = SysAudio::getInstance().getDevice(EDataFlow::eCapture, "Stereo Mix");
    if(!device)
    {
        device = SysAudio::getInstance().getDevice(EDataFlow::eCapture, "Стерео микшер");
    }

    if(!device)
    {
        qDebug() << "ERROR!: Stereo Mix device not found!" << Q_FUNC_INFO;
        return nullptr;
    }

    return device;
}

QString Widget::getStereoMixDeviceId() const
{
    return SysAudio::getInstance().getDeviceId(getStereoMixDevice());
}

void Widget::refreshStereoMixVolume()
{
    // This is fix for crash on exit app. Launch app when no recording devices in the system or all devices disabled in mmsys.cpl.
    int volume = SysAudio::getInstance().getDeviceVolume(getStereoMixDeviceId());
    if(!ui->cbEnableSM->isChecked())
    {
        ui->horizontalSlider->setValue(0);
        ui->lbl_Value->setText(QString::number(0));
        return;
    }

    //int volume = SysAudio::getInstance().getDeviceVolume(getStereoMixDeviceId());
    ui->horizontalSlider->setValue(volume);
    ui->lbl_Value->setText(QString::number(volume));
}

// Enable/Disable StereoMix device
void Widget::handleDeviceEnableOnToggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    SysAudio::getInstance().setDefaultDevice(getStereoMixDeviceId()); // set StereoMix

    const bool isListenDevice = SysAudio::getInstance().isListenDevice(getStereoMixDeviceId());
    ui->cbListen->setChecked(isListenDevice);
    SysAudio::getInstance().isDevicePowerSaveEnabled(getStereoMixDevice()) ? ui->rbDisable->setChecked(true) : ui->rbContinue->setChecked(true);

    qDebug() << Q_FUNC_INFO << checked;

    ui->horizontalSlider->setEnabled(checked);
    ui->cbListen->setEnabled(checked);
    ui->cBox_AudioDevices->setEnabled(checked);
    ui->rbContinue->setEnabled(checked);
    ui->rbDisable->setEnabled(checked);

    if(!SysAudio::getInstance().setEndpointVisibility(getStereoMixDeviceId(), (int)checked)) // 0/1 - disable/enable StereoMix device
    {
        qDebug() << "!SysAudio::getInstance().setEndpointVisibility: " << Q_FUNC_INFO;
        return;
    }

    const QString playbackDeviceName = getPlaybackDeviceName();
    playbackDeviceName.isEmpty() ? ui->cBox_AudioDevices->setCurrentIndex(0) : ui->cBox_AudioDevices->setCurrentText(playbackDeviceName);

    this->refreshStereoMixVolume();
}

// Change volume StereoMix
void Widget::handleDeviceVolumeOnValueChanged(int value)
{
    if(!ui->cbEnableSM->isChecked())
    {
        return;
    }

    SysAudio::getInstance().setDeviceVolume(getStereoMixDeviceId(), value);
    ui->lbl_Value->setText(QString::number(value));
}

// Change Listen checker state
void Widget::handleDeviceListenOnToggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorEnabled, QVariant::fromValue(checked));
}

// Choose Playback audio device
void Widget::handleDevicePlaybackOnActivated(int index)
{
    qDebug() << Q_FUNC_INFO << index;

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    const auto &deviceId = devices[ui->cBox_AudioDevices->currentText()];
    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorOutput, QVariant::fromValue(deviceId));
}

// Power Management control mode
void Widget::handlePowerMgrContinueOnToggled(bool checked)
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

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(false));
}

void Widget::handlePowerMgrDisableOnToggled(bool checked)
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

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(true));
}

Widget::~Widget()
{
    delete ui;
}


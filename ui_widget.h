/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 5.5.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QCheckBox *cbListen;
    QCheckBox *cbEnableSM;
    QGroupBox *gbPowerMgr;
    QRadioButton *rbContinue;
    QRadioButton *rbDisable;
    QLabel *lbl_PlaybackDevice;
    QSlider *horizontalSlider;
    QLabel *lbl_Value;
    QComboBox *cBox_AudioDevices;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName(QStringLiteral("Widget"));
        Widget->resize(331, 219);
        cbListen = new QCheckBox(Widget);
        cbListen->setObjectName(QStringLiteral("cbListen"));
        cbListen->setGeometry(QRect(10, 40, 151, 17));
        cbEnableSM = new QCheckBox(Widget);
        cbEnableSM->setObjectName(QStringLiteral("cbEnableSM"));
        cbEnableSM->setGeometry(QRect(10, 10, 141, 17));
        cbEnableSM->setChecked(false);
        gbPowerMgr = new QGroupBox(Widget);
        gbPowerMgr->setObjectName(QStringLiteral("gbPowerMgr"));
        gbPowerMgr->setGeometry(QRect(10, 130, 301, 71));
        rbContinue = new QRadioButton(gbPowerMgr);
        rbContinue->setObjectName(QStringLiteral("rbContinue"));
        rbContinue->setGeometry(QRect(10, 20, 271, 17));
        rbDisable = new QRadioButton(gbPowerMgr);
        rbDisable->setObjectName(QStringLiteral("rbDisable"));
        rbDisable->setGeometry(QRect(10, 40, 221, 17));
        lbl_PlaybackDevice = new QLabel(Widget);
        lbl_PlaybackDevice->setObjectName(QStringLiteral("lbl_PlaybackDevice"));
        lbl_PlaybackDevice->setGeometry(QRect(10, 70, 151, 16));
        horizontalSlider = new QSlider(Widget);
        horizontalSlider->setObjectName(QStringLiteral("horizontalSlider"));
        horizontalSlider->setGeometry(QRect(190, 10, 121, 22));
        horizontalSlider->setMaximum(100);
        horizontalSlider->setSingleStep(1);
        horizontalSlider->setValue(0);
        horizontalSlider->setOrientation(Qt::Horizontal);
        lbl_Value = new QLabel(Widget);
        lbl_Value->setObjectName(QStringLiteral("lbl_Value"));
        lbl_Value->setGeometry(QRect(190, 40, 121, 20));
        cBox_AudioDevices = new QComboBox(Widget);
        cBox_AudioDevices->setObjectName(QStringLiteral("cBox_AudioDevices"));
        cBox_AudioDevices->setGeometry(QRect(10, 90, 301, 22));

        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QApplication::translate("Widget", "DemoStereoMix", 0));
        cbListen->setText(QApplication::translate("Widget", "Listen from default device", 0));
        cbEnableSM->setText(QApplication::translate("Widget", "Enable StereoMix", 0));
        gbPowerMgr->setTitle(QApplication::translate("Widget", "Power Management", 0));
        rbContinue->setText(QApplication::translate("Widget", "Continue running when on battery power", 0));
        rbDisable->setText(QApplication::translate("Widget", "Disable automatically to save power", 0));
        lbl_PlaybackDevice->setText(QApplication::translate("Widget", "Playback through this device:", 0));
        lbl_Value->setText(QApplication::translate("Widget", "Volume", 0));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H

#include "haikuaudioplugin.h"

#include "haikuaudiodeviceinfo.h"
//#include "haikuaudioinput.h"
#include "haikuaudiooutput.h"
#include <QDebug>

static const char *INPUT_ID = "MediaKit Input";
static const char *OUTPUT_ID = "MediaKit Output";

QT_BEGIN_NAMESPACE

HaikuAudioPlugin::HaikuAudioPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
{
}

QByteArray HaikuAudioPlugin::defaultDevice(QAudio::Mode mode) const
{
    return (mode == QAudio::AudioOutput) ? OUTPUT_ID : INPUT_ID;
}

QList<QByteArray> HaikuAudioPlugin::availableDevices(QAudio::Mode mode) const
{
    if (mode == QAudio::AudioOutput)
        return QList<QByteArray>() << OUTPUT_ID;
    else
        return QList<QByteArray>();
}

QAbstractAudioInput *HaikuAudioPlugin::createInput(const QByteArray &device)
{
    Q_ASSERT(device == INPUT_ID);
    Q_UNUSED(device);
//    return new QAbstractAudioInput();
	return NULL;
}

QAbstractAudioOutput *HaikuAudioPlugin::createOutput(const QByteArray &device)
{
    Q_ASSERT(device == OUTPUT_ID);
    Q_UNUSED(device);
    return new HaikuAudioOutput();
}

QAbstractAudioDeviceInfo *HaikuAudioPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    Q_ASSERT(device == OUTPUT_ID || device == INPUT_ID);
    return new HaikuAudioDeviceInfo(device, mode);
}

QT_END_NAMESPACE

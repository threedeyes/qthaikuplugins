#ifndef HAIKUAUDIODEVICEINFO_H
#define HAIKUAUDIODEVICEINFO_H

#include <qaudiosystem.h>

#include <ByteOrder.h>

QT_BEGIN_NAMESPACE

class HaikuAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT

public:
    HaikuAudioDeviceInfo(const QString &deviceName, QAudio::Mode mode);
    ~HaikuAudioDeviceInfo();

    QAudioFormat preferredFormat() const override;
    bool isFormatSupported(const QAudioFormat &format) const override;
    QString deviceName() const override;
    QStringList supportedCodecs() override;
    QList<int> supportedSampleRates() override;
    QList<int> supportedChannelCounts() override;
    QList<int> supportedSampleSizes() override;
    QList<QAudioFormat::Endian> supportedByteOrders() override;
    QList<QAudioFormat::SampleType> supportedSampleTypes() override;

private:
    const QString m_name;
    const QAudio::Mode m_mode;
};

QT_END_NAMESPACE

#endif

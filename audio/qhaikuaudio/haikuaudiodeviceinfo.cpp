#include "haikuaudiodeviceinfo.h"

QT_BEGIN_NAMESPACE

HaikuAudioDeviceInfo::HaikuAudioDeviceInfo(const QString &deviceName, QAudio::Mode mode)
    : m_name(deviceName),
      m_mode(mode)
{
}

HaikuAudioDeviceInfo::~HaikuAudioDeviceInfo()
{
}

QAudioFormat HaikuAudioDeviceInfo::preferredFormat() const
{
    QAudioFormat format;
    if (m_mode == QAudio::AudioOutput) {
        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setByteOrder(((B_HOST_IS_BENDIAN) ? QAudioFormat::BigEndian : QAudioFormat::LittleEndian));
        format.setSampleType(QAudioFormat::SignedInt);
        format.setSampleSize(16);
        format.setCodec(QLatin1String("audio/pcm"));
    } else {
        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setByteOrder(((B_HOST_IS_BENDIAN) ? QAudioFormat::BigEndian : QAudioFormat::LittleEndian));
        format.setSampleType(QAudioFormat::SignedInt);
        format.setSampleSize(16);
        format.setCodec(QLatin1String("audio/pcm"));
    }
    return format;
}

bool HaikuAudioDeviceInfo::isFormatSupported(const QAudioFormat &format) const
{
    if (!format.codec().startsWith(QLatin1String("audio/pcm")))
        return false;

    return true;
}

QString HaikuAudioDeviceInfo::deviceName() const
{
    return m_name;
}

QStringList HaikuAudioDeviceInfo::supportedCodecs()
{
    return QStringList() << QLatin1String("audio/pcm");
}

QList<int> HaikuAudioDeviceInfo::supportedSampleRates()
{
 	if (m_mode == QAudio::AudioOutput)
    	return QList<int>() << 44100 << 48000 << 96000;
   	return QList<int>() << 48000;
}

QList<int> HaikuAudioDeviceInfo::supportedChannelCounts()
{
    return QList<int>() << 1 << 2;
}

QList<int> HaikuAudioDeviceInfo::supportedSampleSizes()
{
    return QList<int>() << 16 << 32;
}

QList<QAudioFormat::Endian> HaikuAudioDeviceInfo::supportedByteOrders()
{
   	return QList<QAudioFormat::Endian>() << ((B_HOST_IS_BENDIAN) ? QAudioFormat::BigEndian : QAudioFormat::LittleEndian);
}

QList<QAudioFormat::SampleType> HaikuAudioDeviceInfo::supportedSampleTypes()
{
	if (m_mode == QAudio::AudioOutput)
    	return QList<QAudioFormat::SampleType>() << QAudioFormat::SignedInt << QAudioFormat::Float;
    return QList<QAudioFormat::SampleType>() << QAudioFormat::SignedInt;
}

QT_END_NAMESPACE

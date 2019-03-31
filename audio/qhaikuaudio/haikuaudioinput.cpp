#include "haikuaudioinput.h"

#include <private/qaudiohelpers_p.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

HaikuAudioInput::HaikuAudioInput()
    : m_audioSource(0)
    , m_error(QAudio::NoError)
    , m_state(QAudio::StoppedState)
    , m_bytesRead(0)
    , m_elapsedTimeOffset(0)
    , m_totalTimeValue(0)
    , m_volume(qreal(1.0f))
    , m_bytesAvailable(0)
    , m_bufferSize(0)
    , m_periodSize(0)
    , m_intervalTime(1000)
    , m_pullMode(true)
{
	qDebug() << "HaikuAudioInput";
}

HaikuAudioInput::~HaikuAudioInput()
{
    close();
}

void HaikuAudioInput::start(QIODevice *device)
{
	qDebug() << "HaikuAudioInput::start(QIODevice *device)";
    if (m_state != QAudio::StoppedState)
        close();

    if (!m_pullMode && m_audioSource)
        delete m_audioSource;

    m_pullMode = true;
    m_audioSource = device;

    if (open()) {
        setError(QAudio::NoError);
        setState(QAudio::ActiveState);
    } else {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
    }
}

QIODevice *HaikuAudioInput::start()
{
	qDebug() << "HaikuAudioInput::start()";
    if (m_state != QAudio::StoppedState)
        close();

    if (!m_pullMode && m_audioSource)
        delete m_audioSource;

    m_pullMode = false;
    m_audioSource = new InputPrivate(this);
    m_audioSource->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    if (open()) {
        setError(QAudio::NoError);
        setState(QAudio::IdleState);
    } else {
        delete m_audioSource;
        m_audioSource = 0;

        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
    }

    return m_audioSource;
}

void HaikuAudioInput::stop()
{
    if (m_state == QAudio::StoppedState)
        return;

    setError(QAudio::NoError);
    setState(QAudio::StoppedState);
    close();
}

void HaikuAudioInput::reset()
{
    stop();
    m_bytesAvailable = 0;
}

void HaikuAudioInput::suspend()
{
    setState(QAudio::SuspendedState);
}

void HaikuAudioInput::resume()
{
    if (m_pullMode) {
        setState(QAudio::ActiveState);
    } else {
        setState(QAudio::IdleState);
    }
}

int HaikuAudioInput::bytesReady() const
{
    return qMax(m_bytesAvailable, 0);
}

int HaikuAudioInput::periodSize() const
{
    return m_periodSize;
}

void HaikuAudioInput::setBufferSize(int bufferSize)
{
    m_bufferSize = bufferSize;
}

int HaikuAudioInput::bufferSize() const
{
    return m_bufferSize;
}

void HaikuAudioInput::setNotifyInterval(int milliSeconds)
{
    m_intervalTime = qMax(0, milliSeconds);
}

int HaikuAudioInput::notifyInterval() const
{
    return m_intervalTime;
}

qint64 HaikuAudioInput::processedUSecs() const
{
    return qint64(1000000) * m_format.framesForBytes(m_bytesRead) / m_format.sampleRate();
}

qint64 HaikuAudioInput::elapsedUSecs() const
{
    if (m_state == QAudio::StoppedState)
        return 0;

    return m_clockStamp.elapsed() * qint64(1000);
}

QAudio::Error HaikuAudioInput::error() const
{
    return m_error;
}

QAudio::State HaikuAudioInput::state() const
{
    return m_state;
}

void HaikuAudioInput::setFormat(const QAudioFormat &format)
{
	qDebug() << "HaikuAudioInput::setFormat" << format;
    if (m_state == QAudio::StoppedState)
        m_format = format;
}

QAudioFormat HaikuAudioInput::format() const
{
    return m_format;
}

void HaikuAudioInput::setVolume(qreal volume)
{
    m_volume = qBound(qreal(0.0), volume, qreal(1.0));
}

qreal HaikuAudioInput::volume() const
{
    return m_volume;
}

bool HaikuAudioInput::open()
{
    if (!m_format.isValid() || m_format.sampleRate() <= 0) {
        if (!m_format.isValid())
            qWarning("HaikuAudioInput: open error, invalid format.");
        else
            qWarning("HaikuAudioInput: open error, invalid sample rate (%d).", m_format.sampleRate());

        return false;
    }

    m_clockStamp.restart();
    m_timeStamp.restart();
    m_elapsedTimeOffset = 0;
    m_totalTimeValue = 0;
    m_bytesRead = 0;

    return true;
}

void HaikuAudioInput::close()
{
    if (!m_pullMode && m_audioSource) {
        delete m_audioSource;
        m_audioSource = 0;
    }
}

qint64 HaikuAudioInput::read(char *data, qint64 len)
{
    int errorCode = 0;

    m_bytesAvailable = 0;

    return 0;
}

void HaikuAudioInput::setError(QAudio::Error error)
{
    if (m_error == error)
        return;

    m_error = error;
    emit errorChanged(m_error);
}

void HaikuAudioInput::setState(QAudio::State state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);
}

InputPrivate::InputPrivate(HaikuAudioInput *audio)
    : m_audioDevice(audio)
{
}

qint64 InputPrivate::readData(char *data, qint64 len)
{
    return m_audioDevice->read(data, len);
}

qint64 InputPrivate::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return 0;
}

void InputPrivate::trigger()
{
    emit readyRead();
}

QT_END_NAMESPACE

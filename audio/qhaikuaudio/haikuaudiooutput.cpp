#include "qaudiosystem.h"
#include "haikuaudiooutput.h"
#include <private/qaudiohelpers_p.h>

#include <QDebug>
#include <QGuiApplication>
#include <QFileInfo>

QT_BEGIN_NAMESPACE

const int PeriodTimeMs = 10;
const int BufferSizeMs = 40;

const int LowLatencyPeriodTimeMs = 5;
const int LowLatencyBufferSizeMs = 20;

const int TimeOutMs = 100;

#define LOW_LATENCY_CATEGORY_NAME "game"

static void playerProc(void *cookie, void *buffer, size_t len, const media_raw_audio_format &format)
{
	Q_UNUSED(format)
	HaikuAudioOutput *obj = (HaikuAudioOutput*)cookie;
	if (obj->m_state == QAudio::StoppedState
		|| obj->m_state == QAudio::SuspendedState
		|| obj->m_state == QAudio::InterruptedState) {
			memset(buffer, 0, len);
			return;
	}
	
	if (obj->m_pushSource) {
		obj->m_ringbuffer->Read((unsigned char*)buffer, len);
		return;
	}

	size_t bytesRead = obj->m_source->read((char*)buffer, len);
	if (bytesRead <= 0) {
		emit obj->closeDevice();
		if (bytesRead != 0)
			obj->setError(QAudio::IOError);
		obj->setState(QAudio::StoppedState);
		memset(buffer, 0, len);
	} else {
		if (bytesRead < len)
			memset((char*)buffer + bytesRead, 0, len - bytesRead);
		obj->m_bytesWritten += bytesRead;
	}

	if (obj->m_state != QAudio::ActiveState)
		return;

	if (obj->m_notifyInterval > 0 && (obj->m_intervalTimeStamp.elapsed() + obj->m_intervalOffset) > obj->m_notifyInterval) {
		emit obj->notify();
		obj->m_intervalOffset = obj->m_intervalTimeStamp.elapsed() + obj->m_intervalOffset - obj->m_notifyInterval;
		obj->m_intervalTimeStamp.restart();
	}
}

HaikuAudioOutput::HaikuAudioOutput()
	: m_source(NULL)
	, m_error(QAudio::NoError)
	, m_state(QAudio::StoppedState)
	, m_intervalOffset(0)
	, m_bytesWritten(0)
	, m_periodSize(0)
	, m_notifyInterval(1000)
	, m_bufferSize(0)
	, m_pushSource(false)
	, m_player(0)
	, m_ringbuffer(0)
{
	connect(this, SIGNAL(closeDevice()), this, SLOT(close()));
}

HaikuAudioOutput::~HaikuAudioOutput()
{
	stop();
}

void HaikuAudioOutput::start(QIODevice *source)
{
	if (m_state != QAudio::StoppedState)
		stop();

	if (m_source && m_pushSource) {
		delete m_source;
		m_source = NULL;
	}
	
	if (m_ringbuffer) {
		delete m_ringbuffer;
		m_ringbuffer = NULL;
	}

	close();

	m_source = source;
	m_pushSource = false;
	m_error = QAudio::NoError;

	if (open()) {
		setState(QAudio::ActiveState);
		m_player->Start();
		m_player->SetHasData(true);
	} else {
		setError(QAudio::OpenError);
		setState(QAudio::StoppedState);
	}
}

QIODevice *HaikuAudioOutput::start()
{
	if (m_state != QAudio::StoppedState)
		stop();

	if (m_source && m_pushSource) {
		delete m_source;
		m_source = NULL;
	}
	
	if (m_ringbuffer) {
		delete m_ringbuffer;
		m_ringbuffer = NULL;
	}

	close();

	m_error = QAudio::NoError;
    m_source = new HaikuOutputDevicePrivate(this);
    m_source->open(QIODevice::WriteOnly | QIODevice::Unbuffered);
    m_pushSource = true;

	if (open()) {
		setState(QAudio::IdleState);
		m_player->Start();
		m_player->SetHasData(true);
	} else {
		setError(QAudio::OpenError);
		setState(QAudio::StoppedState);
	}

    return m_source;
}

void HaikuAudioOutput::stop()
{
	if (m_state == QAudio::StoppedState)
		return;

	setError(QAudio::NoError);
	setState(QAudio::StoppedState);
	close();
}

void HaikuAudioOutput::reset()
{
	stop();
}

void HaikuAudioOutput::suspend()
{
	if (state() != QAudio::InterruptedState)
		suspendInternal(QAudio::SuspendedState);
}

void HaikuAudioOutput::resume()
{
	if (state() != QAudio::InterruptedState)
		resumeInternal();
}

int HaikuAudioOutput::bytesFree() const
{
	if (m_state != QAudio::ActiveState && m_state != QAudio::IdleState)
		return 0;

	return m_bufferSize;
}

int HaikuAudioOutput::periodSize() const
{
	return m_periodSize;
}

void HaikuAudioOutput::setNotifyInterval(int ms)
{
	m_notifyInterval = ms;
}

int HaikuAudioOutput::notifyInterval() const
{
	return m_notifyInterval;
}

qint64 HaikuAudioOutput::processedUSecs() const
{
	return qint64(1000000) * m_format.framesForBytes(m_bytesWritten) / m_format.sampleRate();
}

qint64 HaikuAudioOutput::elapsedUSecs() const
{
	if (m_state == QAudio::StoppedState)
		return 0;

	return m_clockStamp.elapsed() * qint64(1000);
}

QAudio::Error HaikuAudioOutput::error() const
{
	return m_error;
}

QAudio::State HaikuAudioOutput::state() const
{
	return m_state;
}

void HaikuAudioOutput::setFormat(const QAudioFormat &format)
{
	if (m_state == QAudio::StoppedState)
		m_format = format;
}

QAudioFormat HaikuAudioOutput::format() const
{
	return m_format;
}

void HaikuAudioOutput::setVolume(qreal volume)
{
	if(m_player)
		m_player->SetVolume(qBound(qreal(0.0), volume, qreal(1.0)));
}

qreal HaikuAudioOutput::volume() const
{
	if(m_player)
		return m_player->Volume();
	else
		return 1.0;
}

void HaikuAudioOutput::setCategory(const QString &category)
{
	if (m_category != category)
		m_category = category;
}

QString HaikuAudioOutput::category() const
{
	return m_category;
}


bool HaikuAudioOutput::open()
{
	if (!m_format.isValid() || m_format.sampleRate() <= 0) {
		if (!m_format.isValid())
			qWarning("HaikuAudioOutput: open error, invalid format.");
		else
			qWarning("HaikuAudioOutput: open error, invalid sample rate (%d).", m_format.sampleRate());
		return false;
	}

	qint64 bytesPerSecond = m_format.sampleRate() * m_format.channelCount() * m_format.sampleSize() / 8;
	m_periodTime = (m_category == LOW_LATENCY_CATEGORY_NAME) ? LowLatencyPeriodTimeMs : PeriodTimeMs;	
	m_periodSize = qMin(int(2048), int(bytesPerSecond * m_periodTime) / 1000);

	if (m_category == LOW_LATENCY_CATEGORY_NAME) {
		m_bufferSize = (bytesPerSecond * LowLatencyBufferSizeMs) / qint64(1000);
	} else {
		m_bufferSize = (bytesPerSecond * BufferSizeMs) / qint64(1000);
	}

	uint32 format = media_raw_audio_format::B_AUDIO_SHORT;
	uint32 byte_order = B_MEDIA_LITTLE_ENDIAN;

	switch (m_format.sampleType()) {
		case QAudioFormat::SignedInt:			
			if (m_format.sampleSize() == 8)
				format = media_raw_audio_format::B_AUDIO_CHAR;
			else if (m_format.sampleSize() == 16)
				format = media_raw_audio_format::B_AUDIO_SHORT;
			else if (m_format.sampleSize() == 32)
				format = media_raw_audio_format::B_AUDIO_INT;
			else
				return false;
			break;
		case QAudioFormat::Float:
			if (m_format.sampleSize() == 32)
				format = media_raw_audio_format::B_AUDIO_FLOAT;
			else
				return false;
			break;
		default:
			return false;
	}

	if(m_format.byteOrder() == QAudioFormat::LittleEndian)
		byte_order = B_MEDIA_LITTLE_ENDIAN;
	else
		byte_order = B_MEDIA_BIG_ENDIAN;

	media_raw_audio_format mediaKitFormat = {
		(float)m_format.sampleRate(),
		(uint32)m_format.channelCount(),
		format,
		byte_order,
		(uint32)m_bufferSize / (m_format.sampleSize() / 8)
	};

	if (m_pushSource)
		m_ringbuffer = new RingBuffer(m_bufferSize * 16);
	else
		m_pushSource = NULL;

	QString appname = QCoreApplication::applicationName();

	m_player = new BSoundPlayer(&mediaKitFormat, appname.toUtf8(), playerProc, NULL, (void*)this);

	if(m_player->InitCheck() != B_OK) {
		delete m_player;
		m_player = NULL;
		return false;
	}

	m_bytesWritten = 0;
	m_clockStamp.restart();
	m_intervalTimeStamp.restart();
	m_intervalOffset = 0;

	return true;
}

void HaikuAudioOutput::close()
{
	if (m_player) {
		m_player->SetHasData(false);
		m_player->Stop();
		delete m_player;
		m_player = NULL;
	}
	if (m_pushSource && m_source) {
        delete m_source;
        m_source = NULL;
    }
    if (m_ringbuffer) {
    	delete m_ringbuffer;
    	m_ringbuffer = NULL;
    }
}

void HaikuAudioOutput::setError(QAudio::Error error)
{
	if (m_error != error) {
		m_error = error;
		emit errorChanged(error);
	}
}

void HaikuAudioOutput::setState(QAudio::State state)
{
	if (m_state != state) {
		m_state = state;
		emit stateChanged(state);
	}
}

qint64 HaikuAudioOutput::write(const char *data, qint64 len)
{
    if (!m_ringbuffer)
        return 0;

    if (len == 0)
        return 0;

	int written = 0;		
	int availableBytes = 0;

	bigtime_t begTime = system_time();
	while (availableBytes = m_ringbuffer->GetWriteAvailable() < len) {
		if (system_time() - begTime > TimeOutMs * 1000) {
   			written = m_ringbuffer->Write( (unsigned char*)data, len);
			m_bytesWritten += written;
			return written;
		}
		snooze(10);
	}

   	written = m_ringbuffer->Write( (unsigned char*)data, len);
	m_bytesWritten += written;
	return written;
}

void HaikuAudioOutput::suspendInternal(QAudio::State suspendState)
{
	if (m_player) {
		m_player->SetHasData(false);
		m_player->Stop();
	}
	setState(suspendState);
}

void HaikuAudioOutput::resumeInternal()
{
    if (m_pushSource) {
        setState(QAudio::IdleState);
    } else {
		if (m_player) {
			m_player->SetHasData(true);
			m_player->Start();
			setState(QAudio::ActiveState);
		}
	}
}

HaikuOutputDevicePrivate::HaikuOutputDevicePrivate(HaikuAudioOutput* audio)
{
    m_audioDevice = qobject_cast<HaikuAudioOutput*>(audio);
}

HaikuOutputDevicePrivate::~HaikuOutputDevicePrivate() {}

qint64 HaikuOutputDevicePrivate::readData( char* data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 HaikuOutputDevicePrivate::writeData(const char *data, qint64 len)
{
    int retry = 0;
    qint64 written = 0;

    if (m_audioDevice->state() == QAudio::ActiveState
     || m_audioDevice->state() == QAudio::IdleState) {
        while (written < len) {
            const int writeSize = m_audioDevice->write(data + written, len - written);
            if (writeSize <= 0) {
                retry++;
                if (retry > 10)
                    return written;
                else
                    continue;
            }
            retry = 0;
            written += writeSize;
        }
    }

    return written;
}

QT_END_NAMESPACE

#include "qaudiosystem.h"
#include "haikuaudiooutput.h"
#include <private/qaudiohelpers_p.h>

#include <QDebug>
#include <QGuiApplication>
#include <QFileInfo>

QT_BEGIN_NAMESPACE

const int PeriodTimeMs = 20;
const int BufferSizeMs = 80;

const int LowLatencyPeriodTimeMs = 10;
const int LowLatencyBufferSizeMs = 40;

#define LOW_LATENCY_CATEGORY_NAME "game"

static void playerProc(void *cookie, void *buffer, size_t len, const media_raw_audio_format &format)
{
	HaikuAudioOutput *obj = (HaikuAudioOutput*)cookie;
	if (obj->m_state == QAudio::StoppedState
		|| obj->m_state == QAudio::SuspendedState
		|| obj->m_state == QAudio::InterruptedState) {
			memset(buffer, 0, len);
			return;
	}

	int bytesRead = obj->m_source->read((char*)buffer, len);
	if (bytesRead <= 0) {
		obj->close();
		if (bytesRead != 0)
			obj->setError(QAudio::IOError);
		obj->setState(QAudio::StoppedState);
		memset(buffer, 0, len);
	} else {
		if (bytesRead < len)
			memset(buffer + bytesRead, 0, len - bytesRead);
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
	: m_error(QAudio::NoError)
	, m_state(QAudio::StoppedState)
	, m_bytesWritten(0)
	, m_player(NULL)
	, m_bufferSize(0)
	, m_periodSize(0)
	, m_intervalOffset(0)
	, m_notifyInterval(1000)    
{
}

HaikuAudioOutput::~HaikuAudioOutput()
{
	stop();
}

void HaikuAudioOutput::start(QIODevice *source)
{
	if (m_state != QAudio::StoppedState)
		stop();

	m_source = source;
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

	m_error = QAudio::NoError;

	if (open())
		setState(QAudio::IdleState);
	else {
		setError(QAudio::OpenError);
		setState(QAudio::StoppedState);
	}

    return NULL;
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
	//m_periodSize = (bytesPerSecond * m_periodTime) / 1000;
	
	if (m_category == LOW_LATENCY_CATEGORY_NAME) {
		m_bufferSize = (bytesPerSecond * LowLatencyBufferSizeMs) / qint64(1000);
	} else {
		m_bufferSize = (bytesPerSecond * BufferSizeMs) / qint64(1000);
	}

	uint32 format = media_raw_audio_format::B_AUDIO_SHORT;
	uint32 byte_order = B_MEDIA_LITTLE_ENDIAN;

	if(m_format.sampleSize() != 16 || m_format.sampleType() != QAudioFormat::SignedInt)
		return false;

	if(m_format.byteOrder() == QAudioFormat::LittleEndian)
		byte_order = B_MEDIA_LITTLE_ENDIAN;
	else
		byte_order = B_MEDIA_BIG_ENDIAN;

	media_raw_audio_format mediaKitFormat = {
		(float)m_format.sampleRate(),
		(uint32)m_format.channelCount(),
		format,
		byte_order,
		(uint32)m_bufferSize /  2
	};

	QString appname = QFileInfo(QCoreApplication::applicationFilePath()).fileName();

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
		m_player->Stop();
		delete m_player;
		m_player = NULL;
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

void HaikuAudioOutput::suspendInternal(QAudio::State suspendState)
{
	if (m_player) {
		m_player->Stop();
		setState(suspendState);
	}
}

void HaikuAudioOutput::resumeInternal()
{
	if (m_player) {
		m_player->Start();
		setState(QAudio::ActiveState);
	}
}

QT_END_NAMESPACE

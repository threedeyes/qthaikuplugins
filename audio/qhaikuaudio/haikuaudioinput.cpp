#include "haikuaudioinput.h"

#include <private/qaudiohelpers_p.h>

#include <QDebug>
#include <QCoreApplication>

QT_BEGIN_NAMESPACE

const int TimeOutMs = 250;

void RecordData(void* cookie, bigtime_t timestamp, void* data, size_t size, const media_format &format)
{
	HaikuAudioInput *audioInput = (HaikuAudioInput*)cookie;
	if (!audioInput->m_isRecording) {		
		return;
	}
	
	if (audioInput->m_swrContext != NULL) {
		uint8_t *output;
		int in_samples = (size / (format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK * format.u.raw_audio.channel_count)) / 2;
	    int out_samples = av_rescale_rnd(swr_get_delay(audioInput->m_swrContext, format.u.raw_audio.frame_rate) +
			in_samples, audioInput->m_format.sampleRate(), format.u.raw_audio.frame_rate, AV_ROUND_UP);

	    if (av_samples_alloc(&output, NULL, audioInput->m_format.channelCount(), out_samples, audioInput->m_outAVFormat, 0) > 0) {
	    	out_samples = swr_convert(audioInput->m_swrContext, &output, out_samples, (const uint8_t**)&data, in_samples);
	    	int dst_bufsize = av_samples_get_buffer_size(NULL, audioInput->m_format.channelCount(), out_samples, audioInput->m_outAVFormat, 0);
			audioInput->m_ringBuffer->Write(output, dst_bufsize);
	    	av_freep(&output);
		}
	}
}

void NotifyRecordData(void * cookie, BMediaRecorder::notification code, ...)
{
	HaikuAudioInput *audioInput = (HaikuAudioInput*)cookie;
	if (code == BMediaRecorder::B_WILL_STOP) {
		if (audioInput->m_isRecording) {
			audioInput->close();
		}
	}
}

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
    , m_isRecording(false)
    , m_mediaRecorder(0)
    , m_ringBuffer(0)
    , m_swrContext(0)
    , m_pullMode(true)
{
}

HaikuAudioInput::~HaikuAudioInput()
{
    close();
}

void HaikuAudioInput::start(QIODevice *device)
{
    if (m_state != QAudio::StoppedState)
        close();

    if (!m_pullMode && m_audioSource)
        delete m_audioSource;

    m_pullMode = true;
    m_audioSource = device;

    if (open()) {
        setError(QAudio::NoError);
        setState(QAudio::ActiveState);
        m_isRecording = true;
        m_mediaRecorder->Start();

    } else {
    	m_isRecording = false;
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
    }
}

QIODevice *HaikuAudioInput::start()
{
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
        m_isRecording = true;
        m_mediaRecorder->Start();
    } else {
        delete m_audioSource;
        m_audioSource = 0;
		m_isRecording = false;
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
    }

    return m_audioSource;
}

void HaikuAudioInput::stop()
{
    if (m_state == QAudio::StoppedState)
        return;

	if(m_isRecording) {
		m_isRecording = false;
		m_mediaRecorder->Stop();
	}
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

void HaikuAudioInput::userFeed()
{
    if (m_state == QAudio::StoppedState || m_state == QAudio::SuspendedState)
        return;
    deviceReady();
}

bool HaikuAudioInput::deviceReady()
{
	if (m_pullMode)
		read(0,0);
	else {
		if (m_audioSource != 0) {
			InputPrivate *a = qobject_cast<InputPrivate*>(m_audioSource);
			a->trigger();
        }
    }

	if (m_ringBuffer != NULL)
		m_bytesAvailable = m_ringBuffer->GetReadAvailable();
	else
		m_bytesAvailable = 0;

    if (m_state != QAudio::ActiveState)
        return true;

    if (m_intervalTime && (m_timeStamp.elapsed() + m_elapsedTimeOffset) > m_intervalTime) {
        emit notify();
        m_elapsedTimeOffset = m_timeStamp.elapsed() + m_elapsedTimeOffset - m_intervalTime;
        m_timeStamp.restart();
    }

    return true;
}

int HaikuAudioInput::bytesReady() const
{
	if (m_ringBuffer != NULL) {
		return qMax(m_ringBuffer->GetReadAvailable(), 0);
	}
    return 0;
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
    if (!m_format.isValid() || m_format.sampleRate() <= 0)
        return false;

	if (m_ringBuffer != NULL) {
		delete m_ringBuffer;
		m_ringBuffer = NULL;
	}

	status_t error;

	m_mediaRoster = BMediaRoster::Roster(&error);
	if (!m_mediaRoster)
		return false;

	error = m_mediaRoster->GetAudioInput(&m_audioInputNode);
	if (error < B_OK)
		return false;

	error = m_mediaRoster->GetAudioMixer(&m_audioMixerNode);
	if (error < B_OK)
		return false;
	
	QString appname = QCoreApplication::applicationName();
	
	m_mediaRecorder = new BMediaRecorder(appname.toUtf8(), B_MEDIA_RAW_AUDIO);
	if (m_mediaRecorder->InitCheck() < B_OK) {
		return false;
	}	
	media_format output_format;
	output_format.type = B_MEDIA_RAW_AUDIO;
	output_format.u.raw_audio = media_raw_audio_format::wildcard;
    output_format.u.raw_audio.channel_count = m_format.channelCount();
	m_mediaRecorder->SetAcceptedFormat(output_format);
	
	const int maxInputCount = 64;
	dormant_node_info dni[maxInputCount];

	int32 real_count = maxInputCount;

	error = m_mediaRoster->GetDormantNodes(dni, &real_count, 0, &output_format, 0, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
	if (real_count > maxInputCount)
		real_count = maxInputCount;
	char selected_name[B_MEDIA_NAME_LENGTH] = "Default input";

	for (int i = 0; i < real_count; i++) {
		media_node_id ni[12];
		int32 ni_count = 12;
		error = m_mediaRoster->GetInstancesFor(dni[i].addon, dni[i].flavor_id, ni, &ni_count);
		if (error == B_OK) {
			for (int j = 0; j < ni_count; j++) {
				if (ni[j] == m_audioInputNode.node) {
					strcpy(selected_name, dni[i].name);
					break;
				}
			}
		}
	}

	media_output audioOutput;
	if (!m_mediaRecorder->IsConnected()) {
		int32 count = 0;
		error = m_mediaRoster->GetFreeOutputsFor(m_audioInputNode, &audioOutput, 1, &count, B_MEDIA_RAW_AUDIO);
		if (error < B_OK) {
			return false;
		}
		if (count < 1) {
			return false;
		}
		m_recordFormat.u.raw_audio = audioOutput.format.u.raw_audio;
	} else {
		m_recordFormat.u.raw_audio = m_mediaRecorder->AcceptedFormat().u.raw_audio;
	}
	m_recordFormat.type = B_MEDIA_RAW_AUDIO;

	error = m_mediaRecorder->SetHooks(RecordData, NotifyRecordData, this);
	if (error < B_OK)
		return false;
	
	if (!m_mediaRecorder->IsConnected()) {
		error = m_mediaRecorder->Connect(m_audioInputNode, &audioOutput, &m_recordFormat);
		if (error < B_OK) {
			m_mediaRecorder->SetHooks(NULL, NULL, NULL);
			return false;
		}
	}

	m_ringBuffer = new RingBuffer(m_recordFormat.u.raw_audio.buffer_size * 64);
	if (m_ringBuffer->InitCheck() != B_OK)
		return false;

	m_swrContext = swr_alloc();

	av_opt_set_channel_layout(m_swrContext, "in_channel_layout",
		(m_recordFormat.u.raw_audio.channel_count == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO, 0);

	av_opt_set_channel_layout(m_swrContext, "out_channel_layout",
		(m_format.channelCount() == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO,  0);

	av_opt_set_int(m_swrContext, "in_sample_rate", (int)m_recordFormat.u.raw_audio.frame_rate, 0);
	av_opt_set_int(m_swrContext, "out_sample_rate", m_format.sampleRate(), 0);

	switch (m_recordFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt",  AV_SAMPLE_FMT_U8, 0);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt",  AV_SAMPLE_FMT_S16, 0);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt",  AV_SAMPLE_FMT_S32, 0);
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt",  AV_SAMPLE_FMT_FLT, 0);
			break;
		default:
			return false;
	}

	m_outAVFormat = AV_SAMPLE_FMT_NONE;
	switch (m_format.sampleType()) {
		case QAudioFormat::SignedInt:
			if (m_format.sampleSize() == 8)
				m_outAVFormat = AV_SAMPLE_FMT_U8;
			else if (m_format.sampleSize() == 16)
				m_outAVFormat = AV_SAMPLE_FMT_S16;
			else if (m_format.sampleSize() == 32)
				m_outAVFormat = AV_SAMPLE_FMT_S32;
			else
				return false;
			break;
		case QAudioFormat::Float:
			if (m_format.sampleSize() == 32)
				m_outAVFormat = AV_SAMPLE_FMT_FLT;				
			return false;
			break;
		default:
			return false;
	}
	av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", m_outAVFormat,  0);
	
    if ((swr_init(m_swrContext)) < 0)
        return false;

    m_clockStamp.restart();
    m_timeStamp.restart();
    m_elapsedTimeOffset = 0;
    m_totalTimeValue = 0;
    m_bytesRead = 0;

    return true;
}

void HaikuAudioInput::close()
{	
	if (m_mediaRecorder != NULL) {
		if (m_mediaRecorder->InitCheck() == B_OK) {
			if (m_mediaRecorder->IsConnected())
				m_mediaRecorder->Disconnect();
		}
		delete m_mediaRecorder;
		m_mediaRecorder = NULL;
	}

	if (m_ringBuffer != NULL) {
		delete m_ringBuffer;
		m_ringBuffer = NULL;
	}

    if (!m_pullMode && m_audioSource) {
        delete m_audioSource;
        m_audioSource = 0;
    }    
    
    if (m_swrContext != NULL) {
    	swr_free(&m_swrContext);
    	m_swrContext = NULL;
    }
}

qint64 HaikuAudioInput::read(char *data, qint64 len)
{
	if (!m_isRecording)
		return 0;

	int readed = 0;

	bigtime_t begTime = system_time();
	while (m_ringBuffer->GetReadAvailable() < len) {
		if (system_time() - begTime > TimeOutMs * 1000) {
			readed = m_ringBuffer->Read((unsigned char*)data, m_ringBuffer->GetReadAvailable());
			m_bytesRead += readed;
			return readed;
		}
		snooze(10);
	}

	readed = m_ringBuffer->Read((unsigned char*)data, len);
	m_bytesRead += readed;

	if (m_intervalTime && (m_timeStamp.elapsed() + m_elapsedTimeOffset) > m_intervalTime) {
		emit notify();
		m_elapsedTimeOffset = m_timeStamp.elapsed() + m_elapsedTimeOffset - m_intervalTime;
		m_timeStamp.restart();
	}

    return readed;
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

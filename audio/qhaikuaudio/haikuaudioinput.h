#ifndef HAIKUAUDIOINPUT_H
#define HAIKUAUDIOINPUT_H

#include <qaudiosystem.h>

#include <QSocketNotifier>
#include <QIODevice>
#include <QTime>
#include <QTimer>

#include <OS.h>

#include <MediaKit.h>
#include <SupportKit.h>

#include <MediaFile.h>
#include <MediaNode.h>
#include <MediaRecorder.h>
#include <MediaTrack.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <NodeInfo.h>
#include <MediaAddOn.h>

extern "C" {
	#include <libavutil/opt.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/samplefmt.h>
	#include <libswresample/swresample.h>
}

#include "haikuaudioringbuffer.h"

QT_BEGIN_NAMESPACE

class HaikuAudioInput : public QAbstractAudioInput
{
    Q_OBJECT

public:
    HaikuAudioInput();
    ~HaikuAudioInput();

    void start(QIODevice*) override;
    QIODevice* start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesReady() const override;
    int periodSize() const override;
    void setBufferSize(int ) override;
    int bufferSize() const  override;
    void setNotifyInterval(int ) override;
    int notifyInterval() const override;
    qint64 processedUSecs() const override;
    qint64 elapsedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat&) override;
    QAudioFormat format() const override;
    void setVolume(qreal) override;
    qreal volume() const override;

    friend class InputPrivate;

    bool open();
    void close();
    qint64 read(char *data, qint64 len);
    void setError(QAudio::Error error);
    void setState(QAudio::State state);

    QTime m_timeStamp;
    QTime m_clockStamp;
    QAudioFormat m_format;

    QIODevice *m_audioSource;

    QAudio::Error m_error;
    QAudio::State m_state;

    qint64 m_bytesRead;
    qint64 m_elapsedTimeOffset;
    qint64 m_totalTimeValue;

    qreal m_volume;

    int m_bytesAvailable;
    int m_bufferSize;
    int m_periodSize;
    int m_intervalTime;

    bool m_pullMode;
    
    bool m_isRecording;
    
    RingBuffer *m_ringBuffer;
    SwrContext *m_swrContext;
    
    AVSampleFormat m_outAVFormat;

private:
	BMediaRoster * m_mediaRoster;
	BMediaRecorder * m_mediaRecorder;
	media_format m_recordFormat;
	media_node m_audioInputNode;
	media_node m_audioMixerNode;

private slots:
    void userFeed();
    bool deviceReady();
};

class InputPrivate : public QIODevice
{
    Q_OBJECT
public:
    InputPrivate(HaikuAudioInput *audio);

    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

    void trigger();

private:
    HaikuAudioInput *m_audioDevice;
};

QT_END_NAMESPACE

#endif

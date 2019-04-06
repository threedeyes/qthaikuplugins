
#ifndef HAIKUAUDIOOUTPUT_H
#define HAIKUAUDIOOUTPUT_H

#include "qaudiosystem.h"
#include "haikuaudioringbuffer.h"

#include <QTime>
#include <QTimer>
#include <QIODevice>

#include <MediaKit.h>
#include <SupportKit.h>

QT_BEGIN_NAMESPACE

class HaikuOutputDevicePrivate;

class HaikuAudioOutput : public QAbstractAudioOutput
{
	Q_OBJECT

public:
	HaikuAudioOutput();
	~HaikuAudioOutput();

	void start(QIODevice *source) override;
	QIODevice *start() override;
	void stop() override;
	void reset() override;
	void suspend() override;
	void resume() override;
	int bytesFree() const override;
	int periodSize() const override;
	void setBufferSize(int) override {}
	int bufferSize() const override { return 0; }
	void setNotifyInterval(int ms) override;
	int notifyInterval() const override;
	qint64 processedUSecs() const override;
	qint64 elapsedUSecs() const override;
	QAudio::Error error() const override;
	QAudio::State state() const override;
	void setFormat(const QAudioFormat &format) override;
	QAudioFormat format() const override;
	void setVolume(qreal volume) override;
	qreal volume() const override;
	void setCategory(const QString &category) override;
	QString category() const override;

	QIODevice *m_source;
	QAudio::Error m_error;
	QAudio::State m_state;
	QAudioFormat m_format;
	QString m_category;
    
	QTime m_clockStamp;
	QTime m_intervalTimeStamp;

	qint64 m_intervalOffset;
	qint64 m_bytesWritten;

	int m_periodTime;
	int m_periodSize;
	int m_notifyInterval;
	int m_bufferSize;

	bool open();
	void setError(QAudio::Error error);
	void setState(QAudio::State state);
	void suspendInternal(QAudio::State suspendState);
	void resumeInternal();

	friend class HaikuOutputDevicePrivate;

	bool m_pushSource;

	BSoundPlayer *m_player;
	RingBuffer *m_ringbuffer;

public Q_SLOTS:
	void close();

Q_SIGNALS:
	void closeDevice();
};

class HaikuOutputDevicePrivate : public QIODevice
{
    friend class HaikuAudioOutput;
    Q_OBJECT
public:
    HaikuOutputDevicePrivate(HaikuAudioOutput* audio);
    ~HaikuOutputDevicePrivate();

    qint64 readData( char* data, qint64 len);
    qint64 writeData(const char* data, qint64 len);

private:
    HaikuAudioOutput *audioDevice;
};

QT_END_NAMESPACE

#endif

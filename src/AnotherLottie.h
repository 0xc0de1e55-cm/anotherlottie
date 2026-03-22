#pragma once

#include <QFuture>
#include <QQuickItem>
#include <QSGTexture>
#include <QTimer>
#include <QUrl>

class AnotherLottie : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)
    Q_PROPERTY(int direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(int endFrame READ endFrame NOTIFY endFrameChanged)
    Q_PROPERTY(quint64 frameRate READ frameRate NOTIFY frameRateChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int startFrame READ startFrame NOTIFY startFrameChanged)
    Q_PROPERTY(int status READ status NOTIFY statusChanged)

Q_SIGNALS:
    void autoPlayChanged();
    void directionChanged();
    void endFrameChanged();
    void frameRateChanged();
    void loopsChanged();
    void sourceChanged();
    void startFrameChanged();
    void statusChanged();

    void finished();

public:
    enum Status {
        Null = 0,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    enum Direction {
        Forward = 0,
        Reverse,
    };
    Q_ENUM(Direction)

    enum Loops {
        Infinite = -1,
    };
    Q_ENUM(Loops);

    explicit AnotherLottie(QQuickItem* parent = nullptr);
    ~AnotherLottie() override;

    bool autoPlay() const;
    void setAutoPlay(bool play);

    int direction() const;
    void setDirection(int dir);

    int endFrame() const;

    quint64 frameRate() const;

    int loops() const;
    void setLoops(int loops);

    QUrl source() const;
    void setSource(const QUrl& source);

    int startFrame() const;

    int status() const;

    Q_INVOKABLE double getDuration(bool inFrames) const;
    Q_INVOKABLE void gotoAndPlay(int frame);
    Q_INVOKABLE void gotoAndStop(int frame);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void play();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePause();

protected:
    void componentComplete() override final;
    QSGNode* updatePaintNode(QSGNode* oldNode,  UpdatePaintNodeData* updatePaintNodeData) override final;

private:
    void setStatus(Status status);
    void setFrameRate(quint64 frate);

    void updateContents();
    void prepare();
    void reset(bool emitSignals = true);
    int currentIndex() const;
    void lock();
    void unlock();
    QByteArray loadAnimationData();
    void clearAnimationData();

    void handleScenegraphInitialized();
    void handleWidthChanged();
    void handleHeightChanged();
    void handleTimerTimeouted();

private:
    bool mAutoPlay{false};
    Direction mDirection{Direction::Forward};
    quint64 mFrameRate{0};
    QUrl mSource;
    Status mStatus{Status::Null};

    std::uint64_t mTaskId{0};
    QList<QSGTexture*> mTextures;
    int mCurrentFrameIndex{0};
    QTimer mTimer;
    int mLoops{1};
    int mCurrentLoop{0};
    quint64 mDurationMsecs{0};

    QByteArray mCurrentRawData;

    bool mPended{false};
    bool mLocked{false};
};

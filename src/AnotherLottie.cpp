#include "AnotherLottie.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QtConcurrent>

#include <rlottie.h>

namespace {
struct LoadResult {
    quint64 id;                                                /// Task id
    AnotherLottie::Status status{AnotherLottie::Status::Null}; /// Status of loading result
    QSize frameSize;                                           /// Animation origin frame size
    quint64 duration;                                          /// Animation duration in msecs
    quint64 frameRate;                                         /// Frame rate in msecs
    QList<QSGTexture*> textures;                               /// List of rendered textures
};

}

AnotherLottie::AnotherLottie(QQuickItem* parent)
    : QQuickItem{parent}
{
    setWidth(0.0);
    setHeight(0.0);
    setFlag(QQuickItem::ItemHasContents, false);

    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow* win) {
        if (win != nullptr) {
            connect(win, &QQuickWindow::sceneGraphInitialized, this, &AnotherLottie::handleScenegraphInitialized);
        }
    });
    connect(this, &QQuickItem::widthChanged, this, &AnotherLottie::handleWidthChanged);
    connect(this, &QQuickItem::heightChanged, this, &AnotherLottie::handleHeightChanged);
    connect(&mTimer, &QTimer::timeout, this, &AnotherLottie::handleTimerTimeouted);
}

AnotherLottie::~AnotherLottie()
{
    reset(false);
}

bool AnotherLottie::autoPlay() const
{
    return mAutoPlay;
}

void AnotherLottie::setAutoPlay(bool play)
{
    if (play != mAutoPlay) {
        mAutoPlay = play;
        Q_EMIT autoPlayChanged();
    }
}

int AnotherLottie::direction() const
{
    return static_cast<int>(mDirection);
}

void AnotherLottie::setDirection(int dir)
{
    // Normalize direction value
    if (dir < 0) dir = 0;
    if (dir > 1) dir = 1;

    // Update direction
    const Direction direction = static_cast<Direction>(dir);
    if (direction != mDirection) {
        mDirection = direction;
        Q_EMIT directionChanged();
    }
}

int AnotherLottie::endFrame() const
{
    if (Status::Ready == mStatus) {
        return Direction::Forward == mDirection ? mTextures.size() - 1 : 0;
    }
    return -1;
}

quint64 AnotherLottie::frameRate() const
{
    if (Status::Ready == mStatus) {
        return mFrameRate;
    }
    return 0;
}

int AnotherLottie::loops() const
{
    return mLoops;
}

void AnotherLottie::setLoops(int loops)
{
    // Set as infinite
    if (loops < 0) {
        loops = -1;
    }

    // Update max loops count
    if (loops != mLoops) {
        mLoops = loops;
        Q_EMIT loopsChanged();

        // Stop if loops exceeded
        if (mCurrentLoop > mLoops - 1) {
            stop();
            mCurrentLoop = mLoops - 1;
        }
    }
}

QUrl AnotherLottie::source() const
{
    return mSource;
}

void AnotherLottie::setSource(const QUrl& source)
{
    // Update source url
    if (source != mSource) {
        // NOTE: We supports only local files for now
        if (source.isValid() && !source.isLocalFile()) {
            qWarning() << "AnotherLottie: Source can be only a local file url";
            return;
        }

        // Update property
        mSource = source;
        clearAnimationData();
        Q_EMIT sourceChanged();

        // Update contents
        updateContents();
    }
}

int AnotherLottie::startFrame() const
{
    if (Status::Ready == mStatus) {
        return Direction::Forward == mDirection ? 0 : mTextures.size() - 1;
    }
    return -1;
}

int AnotherLottie::status() const
{
    return static_cast<int>(mStatus);
}

double AnotherLottie::getDuration(bool inFrames) const
{
    if (Status::Ready == mStatus) {
        return inFrames ? mTextures.size() : mDurationMsecs / 1000;
    }
    return 0.0;
}

void AnotherLottie::gotoAndPlay(int frame)
{
    if (Status::Ready == mStatus && frame >= 0 && frame < mTextures.size()) {
        mTimer.stop();
        mCurrentFrameIndex = frame;
        play();
    }
}

void AnotherLottie::gotoAndStop(int frame)
{
    if (Status::Ready == mStatus && frame >= 0 && frame < mTextures.size()) {
        mTimer.stop();
        mCurrentFrameIndex = frame;
        update();
    }
}

void AnotherLottie::pause()
{
    if (Status::Ready == mStatus) {
        mTimer.stop();
    }
}

void AnotherLottie::play()
{
    if (Status::Ready == mStatus && mLoops != 0) {
        mTimer.stop();
        mTimer.start();
    }
}

void AnotherLottie::start()
{
    if (Status::Ready == mStatus && mLoops != 0) {
        mCurrentLoop = 0;
        mCurrentFrameIndex = 0;
        play();
    }
}

void AnotherLottie::stop()
{
    mTimer.stop();
    mCurrentFrameIndex = 0;
}

void AnotherLottie::togglePause()
{
    if (Status::Ready == mStatus && mLoops != 0) {
        if (mTimer.isActive()) {
            pause();
        } else {
            play();
        }
    }
}

void AnotherLottie::componentComplete()
{
    QQuickItem::componentComplete();
    updateContents();
}

QSGNode* AnotherLottie::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    // Ensure if index is valid
    Q_ASSERT_X(mCurrentFrameIndex >= 0 && mCurrentFrameIndex < mTextures.size(), "AnotherLottie::updatePaintNode", "Invalid frame index");

    // Create image node if old node is not initialized
    if (oldNode == nullptr) {
        oldNode = window()->createImageNode();
        Q_ASSERT_X(oldNode, "AnotherLottie::updatePaintNode", "Failed to create scenegraph image node");
    }

    // Cast old node to image node and set texture to a node
    // NOTE: node does not owns texture
    QSGImageNode* imageNode = static_cast<QSGImageNode*>(oldNode);
    imageNode->setTexture(mTextures.at(currentIndex()));
    imageNode->setOwnsTexture(false);
    imageNode->setRect(QRectF{0.0, 0.0, width(), height()});

    return oldNode;
}

void AnotherLottie::setStatus(Status status)
{
    if (status != mStatus) {
        mStatus = status;
        Q_EMIT statusChanged();
    }
}

void AnotherLottie::setFrameRate(quint64 frate)
{
    if (frate != mFrameRate) {
        mFrameRate = frate;
        Q_EMIT frameRateChanged();

        mTimer.stop();
        mTimer.setInterval(static_cast<std::int32_t>(1000 / frate));
    }
}

void AnotherLottie::updateContents()
{
    // Do nothing if component isn't comleted yet or source isn't valid
    if (!isComponentComplete()) {
        return;
    }
    if (!mSource.isValid()) {
        setStatus(Status::Error);
        return;
    }

    reset();
    prepare();
}

void AnotherLottie::prepare()
{
    // Update status to loading
    setStatus(Status::Loading);

    // Check if scenegraph initialized
    if (!(window()->isSceneGraphInitialized())) {
        mPended = true;
        return;
    } else {
        mPended = false;
    }

    // Prepare variables for future
    QSize currentSize{static_cast<int>(width()), static_cast<int>(height())};

    // Load raw data
    QByteArray rawdata = loadAnimationData();
    if (rawdata.isEmpty()) {
        qWarning() << "AnotherLottie: Invalid or emty source";
        return;
    }

    ++mTaskId;
    QtConcurrent::run([data = std::move(rawdata),
                       size = std::move(currentSize),
                       window = QPointer<QQuickWindow>(this->window()),
                       task = mTaskId,
                       key = mSource.toString().toStdString()](QPromise<LoadResult>& promise) {
        LoadResult result;
        result.id = task;

        // Ensure window is valid
        if (window == nullptr) {
            promise.addResult(std::move(result));
            return;
        }

        // Try to create rlottie animation from buffered data
        std::unique_ptr<rlottie::Animation> animation = rlottie::Animation::loadFromData(data.toStdString(), key);
        if (animation == nullptr) {
            result.status = Status::Error;
            promise.addResult(std::move(result));
            return;
        }

        // Get total duration in msecs and framerate
        result.duration = static_cast<quint64>(animation->duration() * 1000);
        result.frameRate = static_cast<quint64>(animation->frameRate());

        // Get origin size
        std::size_t w;
        std::size_t h;
        animation->size(w, h);

        // Compute frame size
        const QSize originSize = QSize{static_cast<int>(w), static_cast<int>(h)};
        result.frameSize = (size.width() > 0 && size.height() > 0) ? size : originSize;

        // Get frames && render em into scene graph textures
        QList<QSGTexture*> textures;
        QImage frame{result.frameSize, QImage::Format_ARGB32_Premultiplied};
        for (std::size_t index = 0; index < animation->totalFrame(); ++index) {
            // Surface for rendering
            rlottie::Surface surface{reinterpret_cast<std::uint32_t*>(frame.bits()),
                                     static_cast<std::size_t>(frame.width()),
                                     static_cast<std::size_t>(frame.height()),
                                     static_cast<size_t>(frame.bytesPerLine())};
            // Render synchronous current frame
            animation->renderSync(index, std::move(surface));
            // Create texture from image
            QSGTexture* texture = window->createTextureFromImage(frame);
            textures << texture;
        }
        result.textures = std::move(textures);

        // Ready
        result.status = Status::Ready;

        // Save result
        promise.addResult(std::move(result));
    }).then(this, [that = QPointer<AnotherLottie>{this}](LoadResult result) {
        if (that != nullptr && result.id == that->mTaskId) {
            if (Status::Error == result.status) {
                that->setStatus(result.status);
            } else {
                that->setStatus(result.status);
                that->lock();
                const QSizeF sz{result.frameSize};
                const QSizeF tsz{that->width(), that->height()};
                if (sz != tsz) {
                    that->setWidth(sz.width());
                    that->setHeight(sz.height());
                }
                that->mDurationMsecs = result.duration;
                that->setFrameRate(result.frameRate);
                that->mTextures = std::move(result.textures);
                that->mCurrentFrameIndex = 0;
                that->unlock();
                that->setFlag(QQuickItem::ItemHasContents, true);
                if (that->mAutoPlay) {
                    that->play();
                }

                Q_EMIT that->endFrameChanged();
                Q_EMIT that->startFrameChanged();
            }
        } else {
            qDeleteAll(result.textures);
        }
    });
}

void AnotherLottie::reset(bool emitSignals)
{
    mTimer.stop();
    qDeleteAll(mTextures);
    mTextures.clear();
    mCurrentLoop = 0;
    mCurrentFrameIndex = 0;
    setStatus(Status::Null);
    setFlag(QQuickItem::ItemHasContents, false);

    if (emitSignals) {
        Q_EMIT endFrameChanged();
        Q_EMIT startFrameChanged();
    }
}

int AnotherLottie::currentIndex() const
{
    // Get frame index based on current direction
    return Direction::Forward == mDirection ? mCurrentFrameIndex : (mTextures.size() - 1 - mCurrentFrameIndex);
}

void AnotherLottie::lock()
{
    mLocked = true;
}

void AnotherLottie::unlock()
{
    mLocked = false;
}

QByteArray AnotherLottie::loadAnimationData()
{
    if (mCurrentRawData.isEmpty()) {
        const QString path = mSource.toLocalFile();
        QFile file{path};
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "AnotherLottie: Failed to open source file";
            return QByteArray{};
        }
        mCurrentRawData = file.readAll();
    }

    return mCurrentRawData;
}

void AnotherLottie::clearAnimationData()
{
    mCurrentRawData.clear();
}

void AnotherLottie::handleScenegraphInitialized()
{
    // Start preparing if pended for prepare
    if (mPended) {
        mPended = false;
        prepare();
    }
}

void AnotherLottie::handleWidthChanged()
{
    if (!mLocked) {
        updateContents();
    }
}

void AnotherLottie::handleHeightChanged()
{
    if (!mLocked) {
        updateContents();
    }
}

void AnotherLottie::handleTimerTimeouted()
{
    ++mCurrentFrameIndex;

    // Normalize index if exceeded
    if (mCurrentFrameIndex >= mTextures.size()) {
        mCurrentFrameIndex = 0;

        // Stop if loops exceeded
        if (mLoops > 0) {
            ++mCurrentLoop;
            if (mCurrentLoop >= mLoops) {
                stop();
                Q_EMIT finished();
                return;
            }
        }
    }

    update();
}

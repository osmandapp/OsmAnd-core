#import <Foundation/Foundation.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#include <OsmAndCore.h>
#include <OsmAndCore/ICoreResourcesProvider.h>
#include <OsmAndCore/Logging.h>
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/MapRendererSetupOptions.h>
#include <OsmAndCore/Map/VectorLinesCollection.h>
#include <OsmAndCore/Map/VectorLineBuilder.h>
#include <QCoreApplication>
#include <QFile>
#include <QThread>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

class BenchmarkResources : public OsmAnd::ICoreResourcesProvider
{
    QString _directory;
public:
    explicit BenchmarkResources(const QString& directory) : _directory(directory) {}

    QByteArray getResource(const QString& name, bool* ok = nullptr) const override
    {
        QString relative = name;
        if (name.startsWith("map/stubs/"))
            relative = "rendering_styles/stubs/[ddf=1.0]/" + name.section('/', -1);
        else if (name.startsWith("map/fonts/"))
            relative = "rendering_styles/fonts/" + name.section('/', -1);
        else if (name == "misc/icu4c/icu-data-l.dat")
            relative = "misc/icu4c/icudt52l.dat";
        QFile file(_directory + '/' + relative);
        const bool opened = file.open(QIODevice::ReadOnly);
        if (ok)
            *ok = opened;
        return opened ? file.readAll() : QByteArray();
    }

    QByteArray getResource(const QString& name, float, bool* ok = nullptr) const override
    {
        return getResource(name, ok);
    }

    bool containsResource(const QString& name) const override
    {
        bool ok;
        getResource(name, &ok);
        return ok;
    }

    bool containsResource(const QString& name, float) const override
    {
        return containsResource(name);
    }
};

using Clock = std::chrono::steady_clock;

static double elapsed(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

static double percentile(std::vector<double> samples, double fraction)
{
    std::sort(samples.begin(), samples.end());
    return samples[std::min(samples.size() - 1, size_t(fraction * samples.size()))];
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 6)
    {
        std::cerr << "Usage: benchmark points lines repeatedFrames angle resourcesDirectory\n";
        return 2;
    }
    const int pointCount = std::atoi(argv[1]);
    const int lineCount = std::atoi(argv[2]);
    const int repeat = std::atoi(argv[3]);
    const int angle = std::atoi(argv[4]);
    if (pointCount < 2 || lineCount < 1 || repeat < 1 || repeat > 120 || 120 % repeat != 0
        || angle <= 0 || angle > 90)
        return 2;
    @autoreleasepool
    {
        if (!OsmAnd::InitializeCore(std::make_shared<BenchmarkResources>(QString::fromUtf8(argv[5]))))
            return 2;
        OsmAnd::Logger::get()->setSeverityLevelThreshold(OsmAnd::LogSeverityLevel::Warning);
        EAGLContext* context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        if (![EAGLContext setCurrentContext:context])
            return 2;
        GLuint framebuffer, color, depth;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glGenRenderbuffers(1, &color);
        glBindRenderbuffer(GL_RENDERBUFFER, color);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            return 3;
        auto renderer = OsmAnd::createMapRenderer(OsmAnd::MapRendererClass::AtlasMapRenderer_OpenGLES2plus);
        OsmAnd::MapRendererSetupOptions options;
        options.gpuWorkerThreadEnabled = false;
        options.displayDensityFactor = 1.0f;
        renderer->setup(options);
        renderer->setWindowSize(OsmAnd::PointI(512, 512));
        renderer->setViewport(OsmAnd::AreaI(0, 0, 512, 512));
        renderer->setTarget(OsmAnd::PointI(1 << 30, 1 << 30));
        renderer->setZoom(16.0f);
        renderer->setElevationAngle(angle);
        if (!renderer->initializeRendering())
            return 3;
        auto collection = std::make_shared<OsmAnd::VectorLinesCollection>();
        auto updates = std::make_shared<std::atomic<int>>(0);
        for (int lineIndex = 0; lineIndex < lineCount; ++lineIndex)
        {
            QVector<OsmAnd::PointI> points;
            points.reserve(pointCount);
            for (int index = 0; index < pointCount; ++index)
            {
                const double fraction = double(index) / (pointCount - 1);
                points.push_back(OsmAnd::PointI((1 << 30) + int((fraction - 0.5) * 80000),
                    (1 << 30) + int(8000 * std::sin(index * 0.07) + lineIndex * 1000)));
            }
            OsmAnd::VectorLineBuilder builder;
            auto line = builder.setLineId(lineIndex + 1).setPoints(points).setLineWidth(64)
                .buildAndAddToCollection(collection);
            line->updatedObservable.attach(reinterpret_cast<OsmAnd::IObservable::Tag>(updates.get()),
                [updates](const OsmAnd::VectorLine*) { ++*updates; });
        }
        renderer->addSymbolsProvider(collection);
        int renderFailures = 0;
        auto draw = [&]()
        {
            if (renderer->isFrameInvalidated() && renderer->prepareFrame())
            {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                if (!renderer->renderFrame())
                    ++renderFailures;
                glFinish();
            }
        };
        auto frame = [&]() { renderer->update(); draw(); };
        bool ready = false;
        for (int index = 0; index < 600; ++index)
        {
            frame();
            if (renderer->isIdle())
            {
                ready = true;
                break;
            }
            QThread::msleep(10);
        }
        if (!ready || renderer->getSymbolsCount() < unsigned(lineCount))
        {
            std::cerr << "WARMUP FAILED: " << renderer->getNotIdleReason().toStdString() << '\n';
            return 4;
        }
        *updates = 0;
        renderFailures = 0;
        std::vector<double> samples, updateTimes;
        int late = 0;
        for (int index = 0; index < 120; ++index)
        {
            renderer->setZoom(16.0f + 0.2f * float(index / repeat + 1) / float(120 / repeat));
            const auto start = Clock::now();
            renderer->update();
            updateTimes.push_back(elapsed(start));
            draw();
            const double milliseconds = elapsed(start);
            samples.push_back(milliseconds);
            if (milliseconds > 16.667)
                ++late;
            else
                QThread::usleep((16.667 - milliseconds) * 1000);
        }
        const int motionUpdates = *updates;
        int idleAt = -1;
        double settleMax = 0.0;
        for (int index = 0; index < 600; ++index)
        {
            const auto start = Clock::now();
            frame();
            settleMax = std::max(settleMax, elapsed(start));
            if (renderer->isIdle())
            {
                idleAt = index;
                break;
            }
            QThread::msleep(10);
        }
        const int settledUpdates = *updates;
        int idleFrames = 0;
        for (int index = 0; index < 60; ++index)
        {
            frame();
            if (renderer->isIdle())
                ++idleFrames;
            QThread::msleep(10);
        }
        std::cout << "RESULT points=" << pointCount << " lines=" << lineCount
            << " repeat=" << repeat << " angle=" << angle
            << " frameMean=" << std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size()
            << " frameP95=" << percentile(samples, .95) << " frameMax=" << percentile(samples, 1)
            << " updateP95=" << percentile(updateTimes, .95) << " late=" << late
            << " renderFailures=" << renderFailures << " motionRebuilds=" << motionUpdates
            << " settleRebuilds=" << settledUpdates - motionUpdates << " settleMax=" << settleMax
            << " idleAt=" << idleAt << " idleFrames=" << idleFrames
            << " idleRebuilds=" << *updates - settledUpdates << std::endl;
        const bool passed = idleAt >= 0 && idleFrames == 60 && *updates == settledUpdates && renderFailures == 0;
        renderer->releaseRendering();
        renderer.reset();
        glDeleteRenderbuffers(1, &depth);
        glDeleteRenderbuffers(1, &color);
        glDeleteFramebuffers(1, &framebuffer);
        [EAGLContext setCurrentContext:nil];
        [context release];
        return passed ? 0 : 5;
    }
}

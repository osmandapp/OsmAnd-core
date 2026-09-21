#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/OnSurfaceVectorMapSymbol.h>
#include <OsmAndCore/Map/VectorLine.h>
#include <OsmAndCore/Map/VectorLineBuilder.h>

#include <chrono>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

using namespace OsmAnd;
using UpdateResult = IUpdatableMapSymbolsGroup::UpdateResult;

static void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

static MapState mapState(float visualZoom, float magnifier = 1.0f)
{
    MapState state;
    state.zoomLevel = state.surfaceZoomLevel = ZoomLevel16;
    state.visualZoom = state.surfaceVisualZoom = visualZoom;
    state.visualZoomShift = magnifier - 1.0f;
    state.metersPerPixel = 2.0 / visualZoom;
    state.hasElevationDataProvider = false;
    state.hasElevationDataResources = false;
    state.flatEarth = true;
    state.target31 = PointI(1 << 30, 1 << 30);
    state.visibleBBoxShifted = AreaI(-1000000, -1000000, 1000000, 1000000);
    state.visibleBBox31 = AreaI((1 << 30) - 1000000, (1 << 30) - 1000000,
        (1 << 30) + 1000000, (1 << 30) + 1000000);
    return state;
}

struct LineFixture
{
    std::shared_ptr<VectorLine> line;
    std::shared_ptr<VectorLine::SymbolsGroup> group;
    std::shared_ptr<OnSurfaceVectorMapSymbol> symbol;

    explicit LineFixture(const MapState& state)
    {
        VectorLineBuilder builder;
        line = builder.setLineId(1)
            .setLineWidth(64.0)
            .setApproximationEnabled(false)
            .setPoints(QVector<PointI>{PointI((1 << 30) - 10000, 1 << 30),
                PointI(1 << 30, (1 << 30) + 10000), PointI((1 << 30) + 10000, 1 << 30)})
            .build();
        group = line->createSymbolsGroup(state);
        require(group && !group->symbols.empty(), "native symbol group created");
        symbol = std::dynamic_pointer_cast<OnSurfaceVectorMapSymbol>(group->symbols.front());
        require(symbol && !symbol->isHidden, "native line geometry generated");
        group->update(state);
        require(!group->updatesPresent(), "initial changes applied");
    }

    std::shared_ptr<VectorMapSymbol::VerticesAndIndices> geometry() const
    {
        auto mesh = symbol->getVerticesAndIndices();
        require(mesh && mesh->vertices && mesh->verticesCount > 1, "nonempty vertex mesh");
        return mesh;
    }
};

static void requireSameGeometry(const LineFixture& first, const LineFixture& second)
{
    const auto firstMesh = first.geometry();
    const auto secondMesh = second.geometry();
    require(firstMesh->verticesCount == secondMesh->verticesCount, "same final vertex count");
    require(*firstMesh->position31 == *secondMesh->position31, "same mesh origin");
    for (unsigned index = 0; index < firstMesh->verticesCount; ++index)
    {
        for (int axis : {0, 2})
        {
            const float firstValue = firstMesh->vertices[index].positionXYZD[axis];
            const float secondValue = secondMesh->vertices[index].positionXYZD[axis];
            require(std::isfinite(firstValue) && std::isfinite(secondValue)
                && std::abs(firstValue - secondValue) <= 0.001f, "same final line geometry");
        }
    }
}

static void requireSettled(LineFixture& fixture, const MapState& target)
{
    require(fixture.group->updatesPresent(), "residual reported to renderer");
    require(!fixture.line->hasUnappliedChanges(), "zoom residual is not a property change");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    UpdateResult result;
    do
    {
        result = fixture.group->update(target);
        if (result != UpdateResult::None)
            break;
        require(fixture.group->updatesPresent(), "waiting zoom remains pending");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    require(result == UpdateResult::Primitive, "settled zoom rebuilds primitive");
    require(!fixture.group->updatesPresent(), "settled update clears pending work");
    const auto mesh = fixture.geometry();
    for (int index = 0; index < 100; ++index)
    {
        require(fixture.group->update(target) == UpdateResult::None, "stable scale does not rebuild");
        require(!fixture.group->updatesPresent(), "stable scale has no pending work");
        require(fixture.geometry() == mesh, "stable scale retains mesh");
    }
}

static void directionRegression()
{
    for (float magnifier : {0.5f, 1.0f, 2.0f})
    {
        const auto target = mapState(0.983757f, magnifier);
        LineFixture fromAbove(mapState(1.166718f, magnifier));
        LineFixture fromBelow(mapState(0.7853405f, magnifier));
        LineFixture direct(target);
        require(fromAbove.group->update(target) == UpdateResult::None, "higher-scale residual deferred");
        require(fromBelow.group->update(target) == UpdateResult::None, "lower-scale residual deferred");
        requireSettled(fromAbove, target);
        requireSettled(fromBelow, target);
        requireSameGeometry(fromAbove, direct);
        requireSameGeometry(fromBelow, direct);
    }
}

static void thresholdAndSurfaceChanges()
{
    LineFixture fixture(mapState(0.75f));
    auto target = mapState(1.0f);
    const auto initial = fixture.geometry();
    require(fixture.group->update(target) == UpdateResult::None, "exact threshold is deferred");
    require(fixture.geometry() == initial, "deferred update retains mesh");
    requireSettled(fixture, target);
    const auto settled = fixture.geometry();
    for (int index = 1; index <= 20; ++index)
    {
        target = mapState(1.0f + index * 0.01f);
        require(fixture.group->update(target) == UpdateResult::None, "continuous small changes stay batched");
        require(fixture.geometry() == settled, "batched changes retain mesh");
    }
    requireSettled(fixture, target);
    target.surfaceVisualZoom += 0.02f;
    require(fixture.group->update(target) == UpdateResult::None, "surface-only change deferred");
    requireSettled(fixture, target);
    target.visualZoomShift += 0.1f;
    require(fixture.group->update(target) == UpdateResult::None, "magnifier-only change deferred");
    requireSettled(fixture, target);
    target.visualZoom += 0.3f;
    require(fixture.group->update(target) == UpdateResult::Primitive, "above-threshold change rebuilds immediately");
    require(!fixture.group->updatesPresent(), "immediate rebuild clears pending work");
}

static void repeatedIntermediateStates()
{
    LineFixture fixture(mapState(1.0f));
    const auto mesh = fixture.geometry();
    MapState target;
    for (int step = 1; step <= 20; ++step)
    {
        target = mapState(1.0f + step * 0.01f);
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            require(fixture.group->update(target) == UpdateResult::None, "short repeats do not settle zoom");
            require(fixture.geometry() == mesh, "motion retains geometry below threshold");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    requireSettled(fixture, target);
    LineFixture direct(target);
    requireSameGeometry(fixture, direct);
}

static void floatingPointJitter()
{
    LineFixture fixture(mapState(1.0f));
    auto target = mapState(1.1f);
    require(fixture.group->update(target) == UpdateResult::None, "jitter starts with residual");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    int rebuilds = 0;
    int index = 0;
    do
    {
        target = mapState(1.1f + (index++ % 2 ? 0.00001f : -0.00001f));
        const auto result = fixture.group->update(target);
        if (result == UpdateResult::Primitive)
        {
            ++rebuilds;
            LineFixture direct(target);
            requireSameGeometry(fixture, direct);
        }
        else
            require(result == UpdateResult::None, "jitter cannot change properties");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    require(rebuilds == 1, "jitter permits exactly one final rebuild");
    require(!fixture.group->updatesPresent(), "jitter does not keep renderer active");
}

static void invalidZoomRecovery()
{
    for (int component = 0; component < 5; ++component)
    {
        for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                0.0f, -1.0f, 32.0f})
        {
            if ((component < 3 && invalid == 32.0f) || (component == 2 && invalid == 0.0f)
                || (component >= 3 && (!std::isfinite(invalid) || invalid == 0.0f)))
                continue;
            LineFixture fixture(mapState(1.0f));
            const auto target = mapState(1.1f);
            require(fixture.group->update(target) == UpdateResult::None, "prepare residual");
            require(fixture.group->updatesPresent(), "pending work before invalid input");
            const auto mesh = fixture.geometry();
            auto bad = target;
            if (component == 0) bad.visualZoom = invalid;
            if (component == 1) bad.surfaceVisualZoom = invalid;
            if (component == 2) bad.visualZoomShift = invalid;
            if (component == 3) bad.zoomLevel = static_cast<ZoomLevel>(static_cast<int>(invalid));
            if (component == 4) bad.surfaceZoomLevel = static_cast<ZoomLevel>(static_cast<int>(invalid));
            for (int index = 0; index < 100; ++index)
            {
                require(fixture.group->update(bad) == UpdateResult::None, "invalid input does not rebuild");
                require(!fixture.group->updatesPresent(), "invalid input cannot latch pending work");
                require(fixture.geometry() == mesh, "invalid input preserves valid geometry");
            }
            require(fixture.group->update(target) == UpdateResult::None, "valid input resumes tracking");
            requireSettled(fixture, target);
            LineFixture direct(target);
            requireSameGeometry(fixture, direct);
        }
    }
}

int main()
{
    directionRegression();
    thresholdAndSurfaceChanges();
    repeatedIntermediateStates();
    floatingPointJitter();
    invalidZoomRecovery();
    std::cout << "PASS: native VectorLine zoom regression tests\n";
    return EXIT_SUCCESS;
}

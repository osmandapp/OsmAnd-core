package net.osmand.core.android;

import android.content.Context;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;

import net.osmand.core.jni.AreaI;
import net.osmand.core.jni.FColorRGB;
import net.osmand.core.jni.IMapElevationDataProvider;
import net.osmand.core.jni.IMapKeyedSymbolsProvider;
import net.osmand.core.jni.IMapLayerProvider;
import net.osmand.core.jni.IMapRenderer;
import net.osmand.core.jni.IMapTiledSymbolsProvider;
import net.osmand.core.jni.MapLayerConfiguration;
import net.osmand.core.jni.MapRendererConfiguration;
import net.osmand.core.jni.MapRendererDebugSettings;
import net.osmand.core.jni.MapRendererSetupOptions;
import net.osmand.core.jni.MapRendererState;
import net.osmand.core.jni.MapStubStyle;
import net.osmand.core.jni.PointI;
import net.osmand.core.jni.ZoomLevel;
import net.osmand.core.jni.MapSymbolInformationList;
import net.osmand.core.jni.MapAnimator;
import net.osmand.core.jni.MapRendererFramePreparedObservable;
import net.osmand.core.jni.MapRendererTargetChangedObservable;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

/**
 * Base abstract class for all map renderers, based on native OsmAndCore::IMapRenderer.
 * <p/>
 * Created by Alexey Pelykh on 25.11.2014.
 */
public abstract class MapRendererView extends FrameLayout {
    private static final String TAG = "OsmAndCore:Android/MapRendererView";

    /**
     * Main GLSurfaceView
     */
    private final GLSurfaceView _glSurfaceView;

    /**
     * Reference to OsmAndCore::IMapRenderer instance
     */
    protected final IMapRenderer _mapRenderer;

    /**
     * Reference to OsmAndCore::MapAnimator instance
     */
    protected final MapAnimator _animator;

    /**
     * Only instance of GPU-worker thread epilogue. Since reference to it is maintained,
     * transferring ownership to native code via SWIG is not needed.
     */
    private final GpuWorkerThreadEpilogue _gpuWorkerThreadEpilogue = new GpuWorkerThreadEpilogue();

    /**
     * Only instance of GPU-worker thread prologue. Since reference to it is maintained,
     * transferring ownership to native code via SWIG is not needed.
     */
    private final GpuWorkerThreadPrologue _gpuWorkerThreadPrologue = new GpuWorkerThreadPrologue();

    /**
     * Only instance of render request callback. Since reference to it is maintained,
     * transferring ownership to native code via SWIG is not needed.
     */
    private final RenderRequestCallback _renderRequestCallback = new RenderRequestCallback();

    private IMapRendererSetupOptionsConfigurator _mapRendererSetupOptionsConfigurator;

    /**
     * Reference to valid EGL display
     */
    private EGLDisplay _display;

    /**
     * Main EGL context
     */
    private EGLContext _mainContext;

    /**
     * GPU-worker EGL context
     */
    private EGLContext _gpuWorkerContext;

    /**
     * GPU-worker EGL surface. Not used in reality, but required to initialize context
     */
    private EGLSurface _gpuWorkerFakeSurface;

    /**
     * Device density factor (160 DPI == 1.0f)
     */
    private float _densityFactor;


    /**
     * Width and height of current window
     */
    private int _windowWidth;
    private int _windowHeight;

    /**
     * Viewport X/Y scale. Can be used to change screen center.
     */
    private float _viewportXScale;
    private float _viewportYScale;

    public MapRendererView(Context context) {
        this(context, null);
    }

    public MapRendererView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MapRendererView(Context context, AttributeSet attrs, int defaultStyle) {
        super(context, attrs, defaultStyle);
        NativeCore.checkIfLoaded();

        // Get display density factor
        _densityFactor = getResources().getDisplayMetrics().density;

        // Create instance of OsmAndCore::IMapRenderer
        _mapRenderer = createMapRendererInstance();

        // Create GLSurfaceView and add it to hierarchy
        _glSurfaceView = new GLSurfaceView(context);
        addView(_glSurfaceView, new LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT));

        // Configure GLSurfaceView
        _glSurfaceView.setPreserveEGLContextOnPause(true);
        _glSurfaceView.setEGLContextClientVersion(2);
        _glSurfaceView.setEGLConfigChooser(true);
        _glSurfaceView.setEGLContextFactory(new EGLContextFactory());
        _glSurfaceView.setRenderer(new RendererProxy());
        _glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        // Create animator for that map
        _animator = new MapAnimator(false);
        _animator.setMapRenderer(_mapRenderer);
    }

    public void setMapRendererSetupOptionsConfigurator(
            IMapRendererSetupOptionsConfigurator configurator) {
        _mapRendererSetupOptionsConfigurator = configurator;
    }

    public IMapRendererSetupOptionsConfigurator getMapRendererSetupOptionsConfigurator() {
        return _mapRendererSetupOptionsConfigurator;
    }

    /**
     * Method to create instance of OsmAndCore::IMapRenderer
     *
     * @return Reference to OsmAndCore::IMapRenderer instance
     */
    protected abstract IMapRenderer createMapRendererInstance();

    private void scheduleReleaseRendering() {
        _glSurfaceView.queueEvent(new Runnable() {
            @Override
            public void run() {
                if (!_mapRenderer.isRenderingInitialized()) {
                    return;
                }

                EGL10 egl = (EGL10) EGLContext.getEGL();
                if (egl == null ||
                        egl.eglGetCurrentContext() == null ||
                        egl.eglGetCurrentContext() == EGL10.EGL_NO_CONTEXT) {
                    Log.v(TAG, "Forcibly releasing rendering by schedule");
                    _mapRenderer.releaseRendering(true);
                } else {
                    Log.v(TAG, "Releasing rendering by schedule");
                    _mapRenderer.releaseRendering();
                }
            }
        });
    }

    @Override
    protected void onDetachedFromWindow() {
        Log.v(TAG, "onDetachedFromWindow()");
        NativeCore.checkIfLoaded();

        // Surface and context are going to be destroyed, thus try to release rendering
        // before that will happen
        Log.v(TAG, "Scheduling rendering release due to onDetachedFromWindow()");
        scheduleReleaseRendering();

        super.onDetachedFromWindow();
    }

    public final void handleOnCreate(Bundle savedInstanceState) {
        Log.v(TAG, "handleOnCreate()");
        NativeCore.checkIfLoaded();

        //TODO: do something here
    }

    public final void handleOnSaveInstanceState(Bundle outState) {
        Log.v(TAG, "handleOnSaveInstanceState()");
        NativeCore.checkIfLoaded();

        //TODO: do something here
    }

    public final void handleOnDestroy() {
        Log.v(TAG, "handleOnDestroy()");
        NativeCore.checkIfLoaded();

        // Don't delete map renderer here, since context destruction will happen later.
        // Map renderer will be automatically deleted by GC anyways. But queue
        // action to release rendering
        Log.v(TAG, "Scheduling rendering release due to handleOnDestroy()");
        scheduleReleaseRendering();
    }

    public final void handleOnLowMemory() {
        Log.v(TAG, "handleOnLowMemory()");
        NativeCore.checkIfLoaded();

        //TODO: notify IMapRenderer to unload as much resources as possible
    }

    public final void handleOnPause() {
        Log.v(TAG, "handleOnPause()");
        NativeCore.checkIfLoaded();

        // Inform GLSurfaceView that activity was paused
        _glSurfaceView.onPause();
    }

    public final void handleOnResume() {
        Log.v(TAG, "handleOnResume()");
        NativeCore.checkIfLoaded();

        // Inform GLSurfaceView that activity was resumed
        _glSurfaceView.onResume();
    }

    public final void requestRender() {
        //Log.v(TAG, "requestRender()");
        NativeCore.checkIfLoaded();

        // Request GLSurfaceView render a frame
        _glSurfaceView.requestRender();
    }

    public final MapAnimator getAnimator() {
        NativeCore.checkIfLoaded();

        return _animator;
    }

    public final MapRendererFramePreparedObservable getFramePreparedObservable() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getFramePreparedObservable();
    }

    public final MapRendererTargetChangedObservable getTargetChangedObservable() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getTargetChangedObservable();
    }

    public final MapRendererConfiguration getConfiguration() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getConfiguration();
    }

    public final void setConfiguration(
            MapRendererConfiguration configuration,
            boolean forcedUpdate) {
        NativeCore.checkIfLoaded();

        _mapRenderer.setConfiguration(configuration, forcedUpdate);
    }

    public final void setConfiguration(MapRendererConfiguration configuration) {
        NativeCore.checkIfLoaded();

        _mapRenderer.setConfiguration(configuration);
    }

    public final boolean isIdle() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isIdle();
    }

    public final String getNotIdleReason() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getNotIdleReason();
    }

    public final void reloadEverything() {
        NativeCore.checkIfLoaded();

        _mapRenderer.reloadEverything();
    }

    public final MapRendererState getState() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState();
    }

    public final int getWindowWidth() {
        NativeCore.checkIfLoaded();

        return _windowWidth;
    }

    public final int getWindowHeight() {
        NativeCore.checkIfLoaded();

        return _windowHeight;
    }

    public final float getViewportXScale() {
        NativeCore.checkIfLoaded();

        return _viewportXScale;
    }

    public final float getViewportYScale() {
        NativeCore.checkIfLoaded();

        return _viewportYScale;
    }

    public final void setViewportXScale(float xScale) {
        NativeCore.checkIfLoaded();

        _viewportXScale = xScale;
        setViewportXYScale(xScale, _viewportYScale);
    }

    public final void setViewportYScale(float yScale) {
        NativeCore.checkIfLoaded();

        _viewportYScale = yScale;
        setViewportXYScale(_viewportXScale, yScale);
    }

    public final void setViewportXYScale(float xScale, float yScale) {
        NativeCore.checkIfLoaded();

        _viewportXScale = xScale;
        _viewportYScale = yScale;

        updateViewport();
    }

    public MapSymbolInformationList getSymbolsAt(PointI screenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsAt(screenPoint);
    }

    public MapSymbolInformationList getSymbolsIn(AreaI screenPoint, boolean strict) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsIn(screenPoint, strict);
    }

    public MapSymbolInformationList getSymbolsIn(AreaI screenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsIn(screenPoint);
    }

    public final long getSymbolsCount() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsCount();
    }

    public final boolean isSymbolsUpdateSuspended() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isSymbolsUpdateSuspended();
    }

    public final boolean suspendSymbolsUpdate() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.suspendSymbolsUpdate();
    }

    public final boolean resumeSymbolsUpdate() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.resumeSymbolsUpdate();
    }

    public final int getSymbolsUpdateInterval() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsUpdateInterval();
    }

    public final void setSymbolsUpdateInterval(int interval) {
        NativeCore.checkIfLoaded();

        _mapRenderer.setSymbolsUpdateInterval(interval);
    }

    public final boolean setMapLayerProvider(
            int layerIndex,
            IMapLayerProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapLayerProvider(layerIndex, provider);
    }

    public final boolean resetMapLayerProvider(int layerIndex) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.resetMapLayerProvider(layerIndex);
    }

    public final boolean setMapLayerConfiguration(
            int layerIndex,
            MapLayerConfiguration mapLayerConfiguration) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapLayerConfiguration(layerIndex, mapLayerConfiguration);
    }

    public final boolean setElevationDataProvider(IMapElevationDataProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setElevationDataProvider(provider);
    }

    public final boolean resetElevationDataProvider() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.resetElevationDataProvider();
    }

    public final boolean addSymbolsProvider(IMapTiledSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.addSymbolsProvider(provider);
    }

    public final boolean addSymbolsProvider(IMapKeyedSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.addSymbolsProvider(provider);
    }

    public final boolean hasSymbolsProvider(IMapTiledSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.hasSymbolsProvider(provider);
    }

    public final boolean hasSymbolsProvider(IMapKeyedSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.hasSymbolsProvider(provider);
    }

    public final boolean removeSymbolsProvider(IMapTiledSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.removeSymbolsProvider(provider);
    }

    public final boolean removeSymbolsProvider(IMapKeyedSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.removeSymbolsProvider(provider);
    }

    public final boolean removeAllSymbolsProviders() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.removeAllSymbolsProviders();
    }

    public final boolean setFieldOfView(float fieldOfView) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFieldOfView(fieldOfView);
    }

    public final boolean setSkyColor(FColorRGB color) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSkyColor(color);
    }

    public final boolean setAzimuth(float azimuth) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setAzimuth(azimuth);
    }

    public final float getElevationAngle() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getElevationAngle();
    }

    public final boolean setElevationAngle(float elevationAngle) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setElevationAngle(elevationAngle);
    }

    public final PointI getTarget() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getTarget31();
    }

    public final boolean setTarget(PointI target31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setTarget(target31);
    }

    public final boolean setTarget(PointI target31, boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setTarget(target31, forcedUpdate, disableUpdate);
    }

    public final float getZoom() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getZoomLevel().ordinal() + (_mapRenderer.getState().getVisualZoom() >= 1.0f 
            ? _mapRenderer.getState().getVisualZoom() - 1.0f 
            : (_mapRenderer.getState().getVisualZoom() - 1.0f) * 2.0f);
    }

    public final boolean setZoom(float zoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setZoom(zoom);
    }

    public final boolean setZoom(ZoomLevel zoomLevel, float visualZoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setZoom(zoomLevel, visualZoom);
    }

    public final ZoomLevel getZoomLevel() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getZoomLevel();
    }

    public final boolean setZoomLevel(ZoomLevel zoomLevel) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setZoomLevel(zoomLevel);
    }

    public final boolean setVisualZoom(float visualZoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setVisualZoom(visualZoom);
    }

    public final boolean setVisualZoomShift(float visualZoomShift) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setVisualZoomShift(visualZoomShift);
    }

    public final boolean setStubsStyle(MapStubStyle style) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setStubsStyle(style);
    }

    public final boolean setBackgroundColor(FColorRGB color) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setBackgroundColor(color);
    }

    public final MapRendererDebugSettings getDebugSettings() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getDebugSettings();
    }

    public final void setDebugSettings(MapRendererDebugSettings debugSettings) {
        NativeCore.checkIfLoaded();

        _mapRenderer.setDebugSettings(debugSettings);
    }

    public final ZoomLevel getMinZoomLevel() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMinZoomLevel();
    }

    public final ZoomLevel getMaxZoomLevel() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMaxZoomLevel();
    }

    public final ZoomLevel getMinimalZoomLevelsRangeLowerBound() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMinimalZoomLevelsRangeLowerBound();
    }

    public final ZoomLevel getMinimalZoomLevelsRangeUpperBound() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMinimalZoomLevelsRangeUpperBound();
    }

    public final ZoomLevel getMaximalZoomLevelsRangeLowerBound() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMaximalZoomLevelsRangeLowerBound();
    }

    public final ZoomLevel getMaximalZoomLevelsRangeUpperBound() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMaximalZoomLevelsRangeUpperBound();
    }

    public final boolean getLocationFromScreenPoint(PointI screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getLocationFromScreenPoint(screenPoint, location31);
    }

    public final boolean getScreenPointFromLocation(PointI location31, PointI outScreenPoint, boolean checkOffScreen) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.obtainScreenPointFromPosition(location31, outScreenPoint, checkOffScreen);
    }

    public final AreaI getVisibleBBox31() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getVisibleBBox31();
    }

    public final boolean isPositionVisible(PointI position31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isPositionVisible(position31);
    }

    public final double getTileSizeInMeters() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getTileSizeInMeters();
    }

    public final double getPixelsToMetersScaleFactor() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getPixelsToMetersScaleFactor();
    }

    public final int getMaxMissingDataZoomShift() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMaxMissingDataZoomShift();
    }

    public final int getMaxMissingDataUnderZoomShift() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMaxMissingDataUnderZoomShift();
    }

    public final int getHeixelsPerTileSide() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getHeixelsPerTileSide();
    }

    public final int getElevationDataTileSize() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getElevationDataTileSize();
    }

    public final void dumpResourcesInfo() {
        NativeCore.checkIfLoaded();

        _mapRenderer.dumpResourcesInfo();
    }

    private final void updateViewport() {
        boolean isXScaleDown = _viewportXScale < 1.0;
        boolean isYScaleDown = _viewportYScale < 1.0;
        float correctedX = isXScaleDown ? -_windowWidth * _viewportXScale : 0;
        float correctedY = isYScaleDown ? -_windowHeight * _viewportYScale : 0;
        _mapRenderer.setViewport(new AreaI(new PointI((int) correctedX, (int) correctedY), 
            new PointI((int) (_windowWidth * (isXScaleDown ? 1.0 :_viewportXScale)),
                       (int) (_windowHeight * (isYScaleDown ? 1.0 :_viewportYScale)))));
    }

    /**
     * EGL context factory
     * <p/>
     * Implements creation of main and GPU-worker contexts along with needed resources
     */
    private final class EGLContextFactory implements GLSurfaceView.EGLContextFactory {
        /**
         * EGL attributes used to initialize EGL context:
         * - EGL context must support at least OpenGLES 2.0
         */
        private final int[] contextAttributes = {
                EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL10.EGL_NONE};

        /**
         * EGL attributes used to initialize GPU-worker EGL surface with 1x1 pixels size
         */
        private final int[] gpuWorkerSurfaceAttributes = {
                EGL10.EGL_WIDTH, 1,
                EGL10.EGL_HEIGHT, 1,
                EGL10.EGL_NONE};

        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            Log.v(TAG, "EGLContextFactory.createContext()...");

            // In case context is create while rendering is initialized, release it first
            if (_mapRenderer != null && _mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Rendering is still initialized during context creation, " +
                        "force releasing it!");

                // Since there's no more context, where previous resources were created,
                // they are lost. Forcibly release rendering
                _mapRenderer.releaseRendering(true);
            }

            // Create main EGL context
            if (_mainContext != null) {
                Log.w(TAG, "Previous main EGL context was not destroyed properly!");
                _mainContext = null;
            }
            try {
                _mainContext = egl.eglCreateContext(
                        display,
                        eglConfig,
                        EGL10.EGL_NO_CONTEXT,
                        contextAttributes);
            } catch (Exception e) {
                Log.e(TAG, "Failed to create main EGL context", e);
                return null;
            }
            if (_mainContext == null || _mainContext == EGL10.EGL_NO_CONTEXT) {
                Log.e(TAG, "Failed to create main EGL context: " +
                        GLSurfaceView.getEglErrorString(egl.eglGetError()));

                _mainContext = null;

                return null;
            }

            // Create GPU-worker EGL context
            if (_gpuWorkerContext != null) {
                Log.w(TAG, "Previous GPU-worker EGL context was not destroyed properly!");
                _gpuWorkerContext = null;
            }
            try {
                _gpuWorkerContext = egl.eglCreateContext(
                        display,
                        eglConfig,
                        _mainContext,
                        contextAttributes);
            } catch (Exception e) {
                Log.e(TAG, "Failed to create GPU-worker EGL context", e);
            }
            if (_gpuWorkerContext == null || _gpuWorkerContext == EGL10.EGL_NO_CONTEXT) {
                Log.e(TAG, "Failed to create GPU-worker EGL context: " +
                        GLSurfaceView.getEglErrorString(egl.eglGetError()));
                _gpuWorkerContext = null;
            }

            // Create GPU-worker EGL surface
            if (_gpuWorkerContext != null) {
                if (_gpuWorkerFakeSurface != null) {
                    Log.w(TAG, "Previous GPU-worker EGL surface was not destroyed properly!");
                    _gpuWorkerFakeSurface = null;
                }
                try {
                    _gpuWorkerFakeSurface = egl.eglCreatePbufferSurface(
                            display,
                            eglConfig,
                            gpuWorkerSurfaceAttributes);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to create GPU-worker EGL surface", e);
                }
                if (_gpuWorkerFakeSurface == null || _gpuWorkerFakeSurface == EGL10.EGL_NO_SURFACE) {
                    Log.e(TAG, "Failed to create GPU-worker EGL surface: " +
                            GLSurfaceView.getEglErrorString(egl.eglGetError()));

                    egl.eglDestroyContext(display, _gpuWorkerContext);
                    _gpuWorkerContext = null;
                    _gpuWorkerFakeSurface = null;
                }
            }

            // Save reference to EGL display
            _display = display;

            // Change renderer setup options
            MapRendererSetupOptions setupOptions = new MapRendererSetupOptions();
            if (_gpuWorkerContext != null && _gpuWorkerFakeSurface != null) {
                setupOptions.setGpuWorkerThreadEnabled(true);
                setupOptions.setGpuWorkerThreadPrologue(_gpuWorkerThreadPrologue.getBinding());
                setupOptions.setGpuWorkerThreadEpilogue(_gpuWorkerThreadEpilogue.getBinding());
            } else {
                setupOptions.setGpuWorkerThreadEnabled(false);
                setupOptions.setGpuWorkerThreadPrologue(null);
                setupOptions.setGpuWorkerThreadEpilogue(null);
            }
            setupOptions.setFrameUpdateRequestCallback(_renderRequestCallback.getBinding());
            setupOptions.setDisplayDensityFactor(_densityFactor);
            if (_mapRendererSetupOptionsConfigurator != null)
                _mapRendererSetupOptionsConfigurator.configureMapRendererSetupOptions(setupOptions);
            _mapRenderer.setup(setupOptions);

            return _mainContext;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            Log.v(TAG, "EGLContextFactory.destroyContext()...");

            // In case context is destroyed while rendering is initialized, release it first
            if (_mapRenderer != null && _mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Rendering is still initialized during context destruction, " +
                        "force releasing it!");

                // Since there's no more context, where previous resources were created,
                // they are lost. Forcibly release rendering
                _mapRenderer.releaseRendering(true);
            }

            // Destroy GPU-worker EGL surface (if present)
            if (_gpuWorkerFakeSurface != null) {
                egl.eglDestroySurface(display, _gpuWorkerFakeSurface);
                _gpuWorkerFakeSurface = null;
            }

            // Destroy GPU-worker EGL context (if present)
            if (_gpuWorkerContext != null) {
                egl.eglDestroyContext(display, _gpuWorkerContext);
                _gpuWorkerContext = null;
            }

            // Destroy main context
            egl.eglDestroyContext(display, context);
            _mainContext = null;

            // Remove reference to EGL display
            _display = null;
        }
    }

    /**
     * Renderer events handler
     * <p/>
     * Proxies calls to OsmAndCore::IMapRenderer
     */
    private final class RendererProxy implements GLSurfaceView.Renderer {
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            Log.v(TAG, "RendererProxy.onSurfaceCreated()...");

            // In case a new surface was created, and rendering was initialized it means that
            // surface was changed, so release rendering to allow it to initialize on next
            // call to onSurfaceChanged
            if (_mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Releasing rendering due to surface recreation");

                // Context still exists here and is active, so just release resources
                _mapRenderer.releaseRendering();
            }
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            Log.v(TAG, "RendererProxy.onSurfaceChanged()...");

            // Set new "window" size and viewport that covers entire "window"
            _windowWidth = width;
            _windowHeight = height;
            _mapRenderer.setWindowSize(new PointI(width, height));

            updateViewport();

            // In case rendering is not initialized, initialize it
            // (happens when surface is created for the first time, or recreated)
            if (!_mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Initializing rendering due to surface size change");

                if (!_mapRenderer.initializeRendering(true))
                    Log.e(TAG, "Failed to initialize rendering");
            }
        }

        public void onDrawFrame(GL10 gl) {
            // In case rendering was not initialized yet, don't do anything
            if (!_mapRenderer.isRenderingInitialized()) {
                Log.w(TAG, "Rendering not yet initialized");
                return;
            }

            // Allow renderer to update
            _mapRenderer.update();

            // In case a new frame was prepared, render it
            if (_mapRenderer.prepareFrame())
                _mapRenderer.renderFrame();

            // Flush all the commands to GPU
            gl.glFlush();
        }
    }

    /**
     * Callback handler to request frame render
     */
    private class RenderRequestCallback
            extends MapRendererSetupOptions.IFrameUpdateRequestCallback {
        @Override
        public void method(IMapRenderer mapRenderer) {
            _glSurfaceView.requestRender();
        }
    }


    /**
     * GPU-worker thread prologue
     */
    private class GpuWorkerThreadPrologue
            extends MapRendererSetupOptions.IGpuWorkerThreadPrologue {
        @Override
        public void method(IMapRenderer mapRenderer) {
            if (_display == null) {
                Log.e(TAG, "EGL display is missing");
                return;
            }

            if (_gpuWorkerContext == null) {
                Log.e(TAG, "GPU-worker context is missing");
                return;
            }

            if (_gpuWorkerFakeSurface == null) {
                Log.e(TAG, "GPU-worker surface is missing");
                return;
            }

            // Get EGL interface
            EGL10 egl = (EGL10) EGLContext.getEGL();
            if (egl == null) {
                Log.e(TAG, "Failed to obtain EGL interface");
                return;
            }

            try {
                if (!egl.eglMakeCurrent(
                        _display,
                        _gpuWorkerFakeSurface,
                        _gpuWorkerFakeSurface,
                        _gpuWorkerContext)) {
                    Log.e(TAG, "Failed to set GPU-worker EGL context active: " +
                            GLSurfaceView.getEglErrorString(egl.eglGetError()));
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to set GPU-worker EGL context active", e);
            }
        }
    }

    /**
     * GPU-worker thread epilogue
     */
    private class GpuWorkerThreadEpilogue
            extends MapRendererSetupOptions.IGpuWorkerThreadEpilogue {
        @Override
        public void method(IMapRenderer mapRenderer) {
            // Get EGL interface
            EGL10 egl = (EGL10) EGLContext.getEGL();
            if (egl == null) {
                Log.e(TAG, "Failed to obtain EGL interface");
                return;
            }

            try {
                if (!egl.eglWaitGL()) {
                    Log.e(TAG, "Failed to wait for GPU-worker EGL context: " +
                            GLSurfaceView.getEglErrorString(egl.eglGetError()));
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to wait for GPU-worker EGL context", e);
            }
        }
    }

    public interface IMapRendererSetupOptionsConfigurator {
        void configureMapRendererSetupOptions(MapRendererSetupOptions mapRendererSetupOptions);
    }
}

package net.osmand.core.android;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;
import android.os.SystemClock;
import android.opengl.GLSurfaceView;

import net.osmand.core.jni.AreaI;
import net.osmand.core.jni.ElevationConfiguration;
import net.osmand.core.jni.GridConfiguration;
import net.osmand.core.jni.FColorRGB;
import net.osmand.core.jni.FColorARGB;
import net.osmand.core.jni.IMapElevationDataProvider;
import net.osmand.core.jni.IMapKeyedSymbolsProvider;
import net.osmand.core.jni.IMapLayerProvider;
import net.osmand.core.jni.IMapRenderer;
import net.osmand.core.jni.IMapTiledSymbolsProvider;
import net.osmand.core.jni.MapLayerConfiguration;
import net.osmand.core.jni.SymbolSubsectionConfiguration;
import net.osmand.core.jni.MapRendererConfiguration;
import net.osmand.core.jni.MapRendererDebugSettings;
import net.osmand.core.jni.MapRendererSetupOptions;
import net.osmand.core.jni.MapState;
import net.osmand.core.jni.MapStubStyle;
import net.osmand.core.jni.PointI;
import net.osmand.core.jni.PointD;
import net.osmand.core.jni.QVectorPointI;
import net.osmand.core.jni.ZoomLevel;
import net.osmand.core.jni.MapSymbolInformationList;
import net.osmand.core.jni.MapAnimator;
import net.osmand.core.jni.MapMarkersAnimator;
import net.osmand.core.jni.MapRendererFramePreparedObservable;
import net.osmand.core.jni.MapRendererTargetChangedObservable;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * Base abstract class for all map renderers, based on native OsmAndCore::IMapRenderer.
 * <p/>
 * Created by Alexey Pelykh on 25.11.2014.
 */
public abstract class MapRendererView extends FrameLayout {
    private static final String TAG = "OsmAndCore:Android/MapRendererView";

    private final static long RELEASE_STOP_TIMEOUT = 4000;
    private final static long RELEASE_WAIT_TIMEOUT = 50;

    /**
     * Reference to OsmAndCore::IMapRenderer instance
     */
    protected IMapRenderer _mapRenderer;

    /**
     * Map renderer that is ready to export
     */
    protected volatile IMapRenderer _exportableMapRenderer;

    /**
     * Main rendering view
     */
    private RenderingView _renderingView;

    /**
     * Optional byte buffer for offscreen rendering
     */
    private ByteBuffer _byteBuffer;

    /**
     * Optional bitmap for offscreen rendering
     */
    private Bitmap _resultBitmap;

    /**
     * Offscreen rendering result is ready
     */
    private volatile boolean _renderingResultIsReady;

    /**
     * Rendering global parameters
     */
    private volatile boolean _frameReadingMode;
    private volatile boolean _batterySavingMode;
    private volatile int _maxFrameRate;
    private volatile long _maxFrameTime;

    private volatile boolean _frameRateChanged = false;
    private final Object _frameRateLock = new Object();

    /**
     * Rendering time metrics
     */
    private long _frameStartTime;
    private long _frameTotalTime;
    private long _frameRenderTime;
    private long _frameGPUFinishTime;
    private volatile float _frameRate;
    private volatile float _frameIdleTime;
    private volatile float _frameGPUWaitTime;
    private volatile float _frameRateLast1K;
    private volatile float _frameIdleTimeLast1K;
    private volatile float _frameGPUWaitTimeLast1K;

    /**
     * Reference to OsmAndCore::MapAnimator instance
     */
    protected MapAnimator _mapAnimator;
    private long _mapAnimationStartTime;
    private boolean _mapAnimationFinished;

    /**
     * Reference to OsmAndCore::MapMarkersAnimator instance
     */
    protected MapMarkersAnimator _mapMarkersAnimator;
    private long _mapMarkersAnimationStartTime;
    private boolean _mapMarkersAnimationFinished;

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

    public MapRendererSetupOptions setupOptions = new MapRendererSetupOptions();

    enum EGLThreadOperation {
        NO_OPERATION,
        CHOOSE_CONFIG,
        CREATE_CONTEXTS,
        CREATE_WINDOW_SURFACE,
        CREATE_PIXELBUFFER_SURFACE,
        INITIALIZE_RENDERING,
        RENDER_FRAME,
        RELEASE_RENDERING,
        DESTROY_SURFACE,
        DESTROY_CONTEXTS
    }

    /**
     * EGL Thread
     */
    public volatile EGLThread eglThread;

    /**
     * Width and height of current window
     */
    private int _windowWidth;
    private int _windowHeight;
    private boolean _inWindow;

    private int frameId;

    /**
     * Rendering is meant to be initialized for the first time
     */
    private volatile boolean isInitializing;

    /**
     * Rendering is meant to be reinitialized
     */
    private volatile boolean isReinitializing;

    /**
     * Rendering is suspended, but renderer needs to be kept initialized
     */
    private volatile boolean isSuspended;

    /**
     * Target surface is present and ready for drawing
     */
    private volatile boolean isSurfaceReady;

    /**
     * OpenGL view is started and ready for rendering
     */
    private volatile boolean isViewStarted;

    /**
     * View is on pause
     */
    private volatile boolean isPaused;

    /**
     * Rendering should be initialized after view is resumed
     */
    private volatile boolean initOnResume;

    private volatile Runnable releaseTask;
    private volatile boolean waitRelease;

    private List<MapRendererViewListener> listeners = new ArrayList<>();

    public interface MapRendererViewListener {
        void onUpdateFrame(MapRendererView mapRenderer);
        void onFrameReady(MapRendererView mapRenderer);
    }

    public MapRendererView(Context context) {
        this(context, null);
    }

    public MapRendererView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MapRendererView(Context context, AttributeSet attrs, int defaultStyle) {
        super(context, attrs, defaultStyle);
    }

    public synchronized void setupRenderer(Context context, int bitmapWidth, int bitmapHeight, MapRendererView oldView) {
        Log.v(TAG, "setupRenderer()");
        NativeCore.checkIfLoaded();

        if (_mapRenderer != null) {
            synchronized (_mapRenderer) {
                _exportableMapRenderer = null;
                stopRenderingView();
                if (!isSuspended && _mapRenderer.isRenderingInitialized()) {
                    Log.v(TAG, "Rendering release due to setupRenderer()");
                    releaseRendering();
                }
                removeRenderingView();
            }
        }

        _inWindow = (bitmapWidth == 0 || bitmapHeight == 0);

        _windowWidth = bitmapWidth;
        _windowHeight = bitmapHeight;

        _byteBuffer = _inWindow ? null : ByteBuffer.allocateDirect(bitmapWidth * bitmapHeight * 4);

        // Set display density factor
        setupOptions.setDisplayDensityFactor(_inWindow ? getResources().getDisplayMetrics().density : 1.0f);

        // Get already present renderer if available
        IMapRenderer oldRenderer = oldView != null ? oldView.suspendRenderer() : null;

        if (oldRenderer == null) {
            setMaximumFrameRate(1);

            isReinitializing = (isSuspended && _mapRenderer != null && _mapRenderer.isRenderingInitialized());
            if (!isReinitializing) {
                Log.v(TAG, "Setting up new renderer to initialize...");
                eglThread = new EGLThread("OpenGLThread");
                eglThread.start();
                _mapRenderer = createMapRendererInstance();
                isInitializing = true;
                isSuspended = false;
            } else {
                Log.v(TAG, "Setting up present renderer to reinitialize...");
                isInitializing = false;
            }
        } else {
            Log.v(TAG, "Setting up provided renderer to reinitialize...");

            if (_mapRenderer != oldRenderer && isSuspended && _mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Releasing suspended renderer...");
                synchronized (eglThread) {
                    eglThread.mapRenderer = _mapRenderer;
                    eglThread.startAndCompleteOperation(EGLThreadOperation.RELEASE_RENDERING);
                }
            }

            // Use previous frame rate limit for battery saving mode
            setMaximumFrameRate(oldView.getMaximumFrameRate());

            // Use present renderer
            _mapRenderer = oldRenderer;

            // Reinitialize renderer
            isInitializing = false;
            isReinitializing = true;

            // Take all EGL references from old map renderer view
            eglThread = oldView.eglThread;

            // Reset rendering options for current view
            if (eglThread.gpuWorkerContext != null && eglThread.gpuWorkerFakeSurface != null) {
                setupOptions.setGpuWorkerThreadEnabled(true);
                setupOptions.setGpuWorkerThreadPrologue(_gpuWorkerThreadPrologue.getBinding());
                setupOptions.setGpuWorkerThreadEpilogue(_gpuWorkerThreadEpilogue.getBinding());
            } else {
                setupOptions.setGpuWorkerThreadEnabled(false);
                setupOptions.setGpuWorkerThreadPrologue(null);
                setupOptions.setGpuWorkerThreadEpilogue(null);
            }
            setupOptions.setFrameUpdateRequestCallback(_renderRequestCallback.getBinding());
            _mapRenderer.setup(setupOptions);
        }

        // Create map animator for that map
        if (_mapAnimator == null) {
            _mapAnimator = new MapAnimator(false);
        }
        _mapAnimator.setMapRenderer(_mapRenderer);

        // Create map markers animator
        if (_mapMarkersAnimator == null) {
            _mapMarkersAnimator = new MapMarkersAnimator();
        }
        _mapMarkersAnimator.setMapRenderer(_mapRenderer);

        synchronized (_mapRenderer) {
            initOnResume = isPaused;
            startRenderingView(context);
        }
    }

    public boolean setViewport(AreaI viewport, boolean forceUpdate) {
        return this._mapRenderer.setViewport(viewport, forceUpdate);
    }

    @Override
    public void setVisibility(int visibility) {
        if (_renderingView != null) {
            _renderingView.setVisibility(visibility);
        }
        super.setVisibility(visibility);
    }

    public void addListener(MapRendererViewListener listener) {
        synchronized (_mapRenderer) {
            if (!isSuspended && !this.listeners.contains(listener)) {
                List<MapRendererViewListener> listeners = new ArrayList<>();
                listeners.addAll(this.listeners);
                listeners.add(listener);
                this.listeners = listeners;
            }
        }
    }

    public void removeListener(MapRendererViewListener listener) {
        synchronized (_mapRenderer) {
            if (!isSuspended && this.listeners.contains(listener)) {
                List<MapRendererViewListener> listeners = new ArrayList<>();
                listeners.addAll(this.listeners);
                listeners.remove(listener);
                this.listeners = listeners;
            }
        }
    }

    /**
     * Method to create instance of OsmAndCore::IMapRenderer
     *
     * @return Reference to OsmAndCore::IMapRenderer instance
     */
    protected abstract IMapRenderer createMapRendererInstance();

    public Bitmap getBitmap() {
        if (!isSuspended && _byteBuffer != null) {
            if (_resultBitmap == null) {
                _resultBitmap = Bitmap.createBitmap(_windowWidth, _windowHeight, Bitmap.Config.ARGB_8888);
            }
            if (_renderingResultIsReady) {
                synchronized (_byteBuffer) {
                    _byteBuffer.rewind();
                    _resultBitmap.copyPixelsFromBuffer(_byteBuffer);
                    _renderingResultIsReady = false;
                }
                if (_frameReadingMode) {
                    Matrix matrix = new Matrix();
                    matrix.preScale(1.0f, -1.0f);
                    _resultBitmap = Bitmap.createBitmap(_resultBitmap, 0, 0, _windowWidth, _windowHeight, matrix, true);
                }
            }
            return _resultBitmap;
        }
        else
            return null;
    }

    public synchronized IMapRenderer suspendRenderer() {
        Log.v(TAG, "suspendRenderer()");

        if (_mapRenderer == null || !_mapRenderer.isRenderingInitialized() || _exportableMapRenderer == null) {
            return null;
        }

        synchronized (_mapRenderer) {
            stopRenderingView();
            if (isSuspended) {
                Log.v(TAG, "Renderer was already suspended");
                return null;
            } else {
                Log.v(TAG, "Suspending renderer to reuse it in other view");
                isSuspended = true;
                isInitializing = false;
                isReinitializing = false;
            }
            removeRenderingView();

            // Clean up data
            listeners = new ArrayList<>();
            _mapAnimator = null;
            _mapMarkersAnimator = null;
        }
        return _exportableMapRenderer.isRenderingInitialized() ? _exportableMapRenderer : null;
    }

    public synchronized void stopRenderer() {
        Log.v(TAG, "stopRenderer()");

        if (_mapRenderer == null) {
            Log.w(TAG, "Can't stop absent renderer");
            return;
        }

        synchronized (_mapRenderer) {
            if (isSuspended) {
                if (_mapRenderer.isRenderingInitialized()) {
                    Log.v(TAG, "Stopping suspended renderer...");
                    synchronized (eglThread) {
                        eglThread.mapRenderer = _mapRenderer;
                        eglThread.startAndCompleteOperation(EGLThreadOperation.RELEASE_RENDERING);
                    }
                }
            } else {
                Log.v(TAG, "Stopping active renderer...");
                stopRenderingView();
                releaseRendering();
                removeRenderingView();
            }
            // Clean up data
            listeners = new ArrayList<>();
            _mapAnimator = null;
            _mapMarkersAnimator = null;
        }

        if (eglThread != null) {
            synchronized (eglThread) {
                eglThread.stopThread = true;
                eglThread.notifyAll();
                while (!eglThread.isStopped) {
                    try {
                        eglThread.wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
            eglThread = null;
        }
    }

    private void releaseRendering() {
        Log.v(TAG, "releaseRendering()");
        if (!isSuspended && _mapRenderer != null && _mapRenderer.isRenderingInitialized()) {
            Log.v(TAG, "Release rendering...");
            synchronized (eglThread) {
                eglThread.mapRenderer = _mapRenderer;
                eglThread.startAndCompleteOperation(EGLThreadOperation.RELEASE_RENDERING);
            }
        }
    }

    public void startRenderingView(Context context) {
        Log.v(TAG, "startRenderingView()");
        if (context != null) {
            _renderingView = new RenderingView(context);
            _renderingView.isOffscreen = !_inWindow;
        }
        if (_renderingView != null && (context == null || !initOnResume)) {
            _renderingView.setPreserveEGLContextOnPause(true);
            _renderingView.setEGLContextClientVersion(3);
            eglThread.configChooser =
                new ComponentSizeChooser(RED_SIZE, GREEN_SIZE, BLUE_SIZE,
                                       ALPHA_SIZE, DEPTH_SIZE, STENCIL_SIZE);
            _renderingView.setEGLConfigChooser(eglThread.configChooser);
            _renderingView.setEGLContextFactory(new EGLContextFactory());
            _renderingView.setEGLWindowSurfaceFactory(
                _inWindow ? new WindowSurfaceFactory() : new PixelbufferSurfaceFactory(_windowWidth, _windowHeight));
            _renderingView.setRenderer(new RendererProxy());
            _renderingView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
            isViewStarted = true;
            if (_inWindow) {
                addView(_renderingView, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
            } else {
                _renderingView.initializeView(_windowWidth, _windowHeight);
                isSurfaceReady = true;
            }
        }
    }

    public void stopRenderingView() {
        Log.v(TAG, "stopRenderingView()");
        if (isViewStarted) {
            isViewStarted = false;
            if (_renderingView != null) {
                _renderingView.onPause();
            }
        } else if (initOnResume) {
            initOnResume = false;
        }
    }

    public void removeRenderingView() {
        Log.v(TAG, "removeRenderingView()");
        isSurfaceReady = false;
        if (_renderingView != null) {
            if (_renderingView.isOffscreen) {
                _renderingView.removeView();
            } else {
                removeAllViews();
            }
            _renderingView = null;
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        Log.v(TAG, "onAttachedToWindow()");
        isSurfaceReady = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        Log.v(TAG, "onDetachedFromWindow()");
        NativeCore.checkIfLoaded();
        if (isSurfaceReady) {
            isSurfaceReady = false;
            // Surface and context are going to be destroyed, thus try to release rendering
            // before that will happen
            Log.v(TAG, "Rendering release due to onDetachedFromWindow()");
            releaseRendering();
        }

        super.onDetachedFromWindow();
    }

    public final void handleOnCreate(Bundle savedInstanceState) {
        Log.v(TAG, "handleOnCreate()");
        NativeCore.checkIfLoaded();
        isSurfaceReady = true;
    }

    public final void handleOnSaveInstanceState(Bundle outState) {
        Log.v(TAG, "handleOnSaveInstanceState()");
        NativeCore.checkIfLoaded();
    }

    public final void handleOnDestroy() {
        Log.v(TAG, "handleOnDestroy()");
        NativeCore.checkIfLoaded();
        if (isSurfaceReady) {
            isSurfaceReady = false;
            // Don't delete map renderer here, since context destruction will happen later.
            // Map renderer will be automatically deleted by GC anyways. But queue
            // action to release rendering
            Log.v(TAG, "Rendering release due to handleOnDestroy()");
            releaseRendering();
        }
    }

    public final void handleOnLowMemory() {
        Log.v(TAG, "handleOnLowMemory()");
        NativeCore.checkIfLoaded();

        //TODO: notify IMapRenderer to unload as much resources as possible
    }

    public final void handleOnPause() {
        Log.v(TAG, "handleOnPause()");
        NativeCore.checkIfLoaded();
        isPaused = true;

        // Inform rendering view that activity was paused
        if (isViewStarted && !isSuspended && _renderingView != null) {
            _renderingView.onPause();
        }
    }

    public final void handleOnResume() {
        Log.v(TAG, "handleOnResume()");
        NativeCore.checkIfLoaded();
        isPaused = false;

        if (_renderingView != null) {
            if (initOnResume && (isInitializing || isReinitializing)) {
                initOnResume = false;
                startRenderingView(null);
            } else if (isViewStarted && !isSuspended) {
                // Inform rendering view that activity was resumed
                _renderingView.onResume();
            }
        }
    }

    private final void requestRender() {
        //Log.v(TAG, "requestRender()");
        NativeCore.checkIfLoaded();

        // Request GLSurfaceView render a frame
        if (isViewStarted && !isSuspended && _renderingView != null) {
            _renderingView.requestRender();
        }
    }

    public final MapAnimator getMapAnimator() {
        NativeCore.checkIfLoaded();

        return _mapAnimator;
    }

    public final MapMarkersAnimator getMapMarkersAnimator() {
        NativeCore.checkIfLoaded();

        return _mapMarkersAnimator;
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

    public final int getDefaultWorkerThreadsLimit() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getDefaultThreadsLimit();
    }

    public final int getResourceWorkerThreadsLimit() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getResourceWorkerThreadsLimit();
    }

    public final void setResourceWorkerThreadsLimit(int limit) {
        NativeCore.checkIfLoaded();

        _mapRenderer.setResourceWorkerThreadsLimit(limit);
    }

    public final void resetResourceWorkerThreadsLimit() {
        NativeCore.checkIfLoaded();

        _mapRenderer.resetResourceWorkerThreadsLimit();
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

    public final MapState getState() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getFutureState();
    }

    public final int getWindowWidth() {
        NativeCore.checkIfLoaded();

        return _windowWidth;
    }

    public final int getWindowHeight() {
        NativeCore.checkIfLoaded();

        return _windowHeight;
    }

    public final int getFrameId() {
        return frameId;
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

    public final boolean isSymbolsLoadingActive() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isSymbolsLoadingActive();
    }

    public final float getSymbolsLoadingTime() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getPreviousElapsedSymbolsLoadingTime();
    }

    public final boolean isFrameInvalidated() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isFrameInvalidated();
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

    public final boolean setElevationConfiguration(ElevationConfiguration elevationConfiguration) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setElevationConfiguration(elevationConfiguration);
    }

    public final boolean setGridConfiguration(GridConfiguration gridConfiguration) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setGridConfiguration(gridConfiguration);
    }

    public final boolean setElevationDataProvider(IMapElevationDataProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setElevationDataProvider(provider);
    }

    public final boolean resetElevationDataProvider() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.resetElevationDataProvider();
    }

    public final boolean setElevationScaleFactor(float scaleFactor) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setElevationScaleFactor(scaleFactor);
    }

    public final float getElevationScaleFactor() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getElevationScaleFactor();
    }

    public final boolean addSymbolsProvider(IMapTiledSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.addSymbolsProvider(provider);
    }

    public final boolean addSymbolsProvider(int subsectionIndex, IMapTiledSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.addSymbolsProvider(subsectionIndex, provider);
    }

    public final boolean addSymbolsProvider(IMapKeyedSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.addSymbolsProvider(provider);
    }

    public final boolean addSymbolsProvider(int subsectionIndex, IMapKeyedSymbolsProvider provider) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.addSymbolsProvider(subsectionIndex, provider);
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

    public final void updateSubsection(int subsectionIndex) {
        NativeCore.checkIfLoaded();

        _mapRenderer.updateSubsection(subsectionIndex);
    }

    public final boolean setSymbolSubsectionConfiguration(
            int subsectionIndex,
            SymbolSubsectionConfiguration symbolSubsectionConfiguration) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSymbolSubsectionConfiguration(subsectionIndex, symbolSubsectionConfiguration);
    }

    public final boolean setFieldOfView(float fieldOfView) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFieldOfView(fieldOfView);
    }

    public final boolean setVisibleDistance(float visibleDistance) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setVisibleDistance(visibleDistance);
    }

    public final boolean setDetailedDistance(float detailedDistance) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setDetailedDistance(detailedDistance);
    }

    public final boolean setSkyColor(FColorRGB color) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSkyColor(color);
    }

    public final float getAzimuth() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getAzimuth();
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

        PointI fixedPixel = _mapRenderer.getState().getFixedPixel();
        if (fixedPixel.getX() >= 0 && fixedPixel.getY() >= 0) {
            return _mapRenderer.getState().getFixedLocation31();
        } else {
            return _mapRenderer.getState().getTarget31();
        }
    }

    public final PointI getTargetScreenPosition() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getFixedPixel();
    }

    public final boolean setTarget(PointI target31) {
        NativeCore.checkIfLoaded();

        if (_windowWidth > 0 && _windowHeight > 0) {
            return _mapRenderer.setMapTargetLocation(target31);
        } else {
            return _mapRenderer.setTarget(target31);
        }
    }

    public final boolean setTarget(PointI target31, boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setTarget(target31, forcedUpdate, disableUpdate);
    }

    public final boolean setMapTarget(PointI screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapTarget(screenPoint, location31);
    }

    public final boolean setMapTarget(PointI screenPoint, PointI location31,
        boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapTarget(screenPoint, location31, forcedUpdate, disableUpdate);
    }

    public final boolean resetMapTarget() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.resetMapTarget();
    }

    public final boolean resetMapTargetPixelCoordinates(PointI screenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.resetMapTargetPixelCoordinates(screenPoint);
    }

    public final boolean setMapTargetPixelCoordinates(PointI screenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapTargetPixelCoordinates(screenPoint);
    }

    public final boolean setMapTargetPixelCoordinates(PointI screenPoint, boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapTargetPixelCoordinates(screenPoint, forcedUpdate, disableUpdate);
    }

    public final boolean setMapTargetLocation(PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapTargetLocation(location31);
    }

    public final boolean setMapTargetLocation(PointI location31, boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMapTargetLocation(location31, forcedUpdate, disableUpdate);
    }

    public final float getMapTargetHeightInMeters() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMapTargetHeightInMeters();
    }

    public final PointI getSecondaryTarget() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getAimLocation31();
    }

    public final PointI getSecondaryTargetScreenPosition() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getFixedPixel();
    }

    public final boolean setSecondaryTarget(PointI screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSecondaryTarget(screenPoint, location31);
    }

    public final boolean setSecondaryTarget(PointI screenPoint, PointI location31,
        boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSecondaryTarget(screenPoint, location31, forcedUpdate, disableUpdate);
    }

    public final boolean setSecondaryTargetPixelCoordinates(PointI screenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSecondaryTargetPixelCoordinates(screenPoint);
    }

    public final boolean setSecondaryTargetPixelCoordinates(PointI screenPoint, boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSecondaryTargetPixelCoordinates(screenPoint, forcedUpdate, disableUpdate);
    }

    public final boolean setSecondaryTargetLocation(PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSecondaryTargetLocation(location31);
    }

    public final boolean setSecondaryTargetLocation(PointI location31, boolean forcedUpdate, boolean disableUpdate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSecondaryTargetLocation(location31, forcedUpdate, disableUpdate);
    }

    public final int getAimingActions() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getAimingActions();
    }

    public final boolean setAimingActions(int actionBits) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setAimingActions(actionBits);
    }

    public final float getFlatZoom() {
        NativeCore.checkIfLoaded();

        float visualZoom = getFlatVisualZoom();
        float zoomFloatPart = visualZoom >= 1.0f ? visualZoom - 1.0f : (visualZoom - 1.0f) * 2.0f;
        return getFlatZoomLevel().ordinal() + zoomFloatPart;
    }

    public final float getZoom() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getSurfaceZoomLevel().ordinal() + (_mapRenderer.getState().getSurfaceVisualZoom() >= 1.0f
            ? _mapRenderer.getState().getSurfaceVisualZoom() - 1.0f
            : (_mapRenderer.getState().getSurfaceVisualZoom() - 1.0f) * 2.0f);
    }

    public final boolean setFlatZoom(float zoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFlatZoom(zoom);
    }

    public final boolean setZoom(float zoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setZoom(zoom);
    }

    public final boolean setFlatZoom(ZoomLevel zoomLevel, float visualZoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFlatZoom(zoomLevel, visualZoom);
    }

    public final boolean setZoom(ZoomLevel zoomLevel, float visualZoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setZoom(zoomLevel, visualZoom);
    }

    public final ZoomLevel getZoomLevel() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getSurfaceZoomLevel();
    }

    public final ZoomLevel getFlatZoomLevel() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getZoomLevel();
    }

    public final boolean setFlatZoomLevel(ZoomLevel zoomLevel) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFlatZoomLevel(zoomLevel);
    }

    public final boolean setZoomLevel(ZoomLevel zoomLevel) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setZoomLevel(zoomLevel);
    }

    public final float getVisualZoom() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getSurfaceVisualZoom();
    }

    public final float getFlatVisualZoom() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getVisualZoom();
    }

    public final boolean setFlatVisualZoom(float visualZoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFlatVisualZoom(visualZoom);
    }

    public final boolean setVisualZoom(float visualZoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setVisualZoom(visualZoom);
    }

    public final boolean setVisualZoomShift(float visualZoomShift) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setVisualZoomShift(visualZoomShift);
    }

    public final boolean restoreFlatZoom(float heightInMeters) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.restoreFlatZoom(heightInMeters);
    }

    public final boolean setStubsStyle(MapStubStyle style) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setStubsStyle(style);
    }

    public final boolean setBackgroundColor(FColorRGB color) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setBackgroundColor(color);
    }

    public final boolean setFogColor(FColorRGB color) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setFogColor(color);
    }

    public final boolean setMyLocationCircleColor(FColorARGB color) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMyLocationColor(color);
    }

    public final boolean setMyLocationCirclePosition(PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMyLocation31(location31);
    }

    public final boolean setMyLocationCircleRadius(float radiusInMeters) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMyLocationRadiusInMeters(radiusInMeters);
    }

    public final boolean setMyLocationSectorDirection(float directionAngle) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMyDirection(directionAngle);
    }

    public final boolean setMyLocationSectorRadius(float radius) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMyDirectionRadius(radius);
    }

    public final boolean setSymbolsOpacity(float opacityFactor) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSymbolsOpacity(opacityFactor);
    }

    public final float getSymbolsOpacity() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsOpacity();
    }

    public final boolean setDateTime(long dateTime) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setDateTime(dateTime);
    }

    public final boolean changeTimePeriod() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.changeTimePeriod();
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

    public final boolean setMinZoomLevel(ZoomLevel zoomLevel) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMinZoomLevel(zoomLevel);
    }

    public final boolean setMaxZoomLevel(ZoomLevel zoomLevel) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setMaxZoomLevel(zoomLevel);
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

    public final boolean getLocationFromScreenPoint(PointD screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getLocationFromScreenPoint(screenPoint, location31);
    }

    public final boolean getLocationFromScreenPoint(PointI screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getLocationFromScreenPoint(screenPoint, location31);
    }

    public final boolean getLocationFromElevatedPoint(PointI screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getLocationFromElevatedPoint(screenPoint, location31);
    }

    public final float getHeightAndLocationFromElevatedPoint(PointI screenPoint, PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getHeightAndLocationFromElevatedPoint(screenPoint, location31);
    }

    public final float getSurfaceZoomAfterPinch(
        PointI firstLocation31, float firstHeigthInMeters, PointI firstScreenPoint,
        PointI secondLocation31, float secondHeightInMeters, PointI secondScreenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSurfaceZoomAfterPinch(
            firstLocation31, firstHeigthInMeters, firstScreenPoint,
            secondLocation31, secondHeightInMeters, secondScreenPoint
        );
    }

    public final float getSurfaceZoomAfterPinchWithParams(
        PointI fixedLocation31, float surfaceZoom, float azimuth,
        PointI firstLocation31, float firstHeightInMeters, PointI firstScreenPoint,
        PointI secondLocation31, float secondHeightInMeters, PointI secondScreenPoint) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSurfaceZoomAfterPinchWithParams(
            fixedLocation31, surfaceZoom, azimuth,
            firstLocation31, firstHeightInMeters, firstScreenPoint,
            secondLocation31, secondHeightInMeters, secondScreenPoint
        );
    }

    public final boolean getZoomAndRotationAfterPinch(
                                            PointI firstLocation31, float firstHeightInMeters, PointI firstPoint,
                                            PointI secondLocation31, float secondHeightInMeters, PointI secondPoint,
                                            PointD zoomAndRotate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getZoomAndRotationAfterPinch(firstLocation31, firstHeightInMeters, firstPoint,
                secondLocation31, secondHeightInMeters, secondPoint, zoomAndRotate);
    }

    public final boolean getTiltAndRotationAfterMove(
                                            PointI firstLocation31, float firstHeightInMeters, PointI firstPoint,
                                            PointI secondLocation31, float secondHeightInMeters, PointI secondPoint,
                                            PointD tiltAndRotate) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getTiltAndRotationAfterMove(firstLocation31, firstHeightInMeters, firstPoint,
                secondLocation31, secondHeightInMeters, secondPoint, tiltAndRotate);
    }

    public final float getLocationHeightInMeters(PointI location31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getLocationHeightInMeters(location31);
    }

    public final boolean getNewTargetByScreenPoint(PointI screenPoint, PointI location31, PointI target31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getNewTargetByScreenPoint(screenPoint, location31, target31);
    }

    public final boolean getNewTargetByScreenPoint(PointI screenPoint, PointI location31, PointI target31,
        float height) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getNewTargetByScreenPoint(screenPoint, location31, target31, height);
    }

    public final float getMapTargetDistance(PointI location31, boolean checkOffScreen) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getMapTargetDistance(location31, checkOffScreen);
    }

    public final float getCameraHeightInMeters() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getCameraHeightInMeters();
    }

    public final int getTileZoomOffset() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getTileZoomOffset();
    }

    public final boolean getScreenPointFromLocation(PointI location31, PointI outScreenPoint, boolean checkOffScreen) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.obtainScreenPointFromPosition(location31, outScreenPoint, checkOffScreen);
    }

    public final boolean getElevatedPointFromLocation(PointI location31, PointI outScreenPoint, boolean checkOffScreen) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.obtainElevatedPointFromPosition(location31, outScreenPoint, checkOffScreen);
    }

    public final AreaI getVisibleBBox31() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getVisibleBBox31();
    }

    public final boolean isPositionVisible(PointI position31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isPositionVisible(position31);
    }

    public final boolean isPathVisible(QVectorPointI path31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isPathVisible(path31);
    }

    public final boolean isAreaVisible(AreaI area31) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isAreaVisible(area31);
    }

    public final boolean isTileVisible(int tileX, int tileY, int zoom) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.isTileVisible(tileX, tileY, zoom);
    }

    public final double getTileSizeInMeters() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getTileSizeInMeters();
    }

    public final double getPixelsToMetersScaleFactor() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getPixelsToMetersScaleFactor();
    }

    public final double getTileSizeOnScreenInPixels() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getTileSizeOnScreenInPixels();
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

    public final boolean enableFrameReader() {
        if (_frameReadingMode || _byteBuffer != null) {
            return true;
        } else if (_windowWidth > 0 && _windowHeight > 0) {
            synchronized (eglThread) {
                _byteBuffer = ByteBuffer.allocateDirect(_windowWidth * _windowHeight * 4);
                _renderingResultIsReady = false;
            }
            _frameReadingMode = true;
            return true;
        }
        return false;
    }

    public final void disableFrameReader() {
        if (_frameReadingMode) {
            synchronized (eglThread) {
                _byteBuffer = null;
            }
            _frameReadingMode = false;
        }
    }

    public final boolean isBatterySavingModeEnabled() {
        return _batterySavingMode;
    }

    public final void enableBatterySavingMode() {
        _batterySavingMode = true;
        setMaximumFrameRate(20);
    }

    public final void disableBatterySavingMode() {
        _batterySavingMode = false;
        setMaximumFrameRate(_maxFrameRate);
    }

    public final int getMaximumFrameRate() {
        return _maxFrameRate;
    }

    public final int getMaxAllowedFrameRate() {
        return _batterySavingMode ? 20 : getMaxHardwareFrameRate();
    }

    public final void setMaximumFrameRate(int maximumFramesPerSecond) {
        synchronized (_frameRateLock) {
            maximumFramesPerSecond = Math.min(maximumFramesPerSecond, getMaxAllowedFrameRate());

            _maxFrameRate = maximumFramesPerSecond;
            _maxFrameTime = Math.round(1000.0f / (float) _maxFrameRate);
            _frameRateChanged = true;

            _frameRateLock.notifyAll();
        }
    }

    public final int getMaxHardwareFrameRate() {

        int maxFrameRate = 60;
        try {
            android.view.Display display = null;
            if (_renderingView != null && _renderingView.getContext() != null) {
                android.view.WindowManager windowManager = (android.view.WindowManager)
                    _renderingView.getContext().getSystemService(android.content.Context.WINDOW_SERVICE);
                if (windowManager != null) {
                    display = windowManager.getDefaultDisplay();
                }
            }

            if (display != null) {
                float refreshRate = display.getRefreshRate();
                maxFrameRate = Math.round(refreshRate);
            }
        } catch (Exception e) {

        }

        return maxFrameRate;
    }

    public final float getBasicThreadsCPULoad() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getBasicThreadsCPULoad();
    }

    public final float getFrameRate() {
        return _frameRate;
    }

    public final float getIdleTimePart() {
        return _frameIdleTime;
    }

    public final float getGPUWaitTimePart() {
        return _frameGPUWaitTime;
    }

    public final float getFrameRateLast1K() {
        return _frameRateLast1K;
    }

    public final float getIdleTimePartLast1K() {
        return _frameIdleTimeLast1K;
    }

    public final float getGPUWaitTimePartLast1K() {
        return _frameGPUWaitTimeLast1K;
    }

    public final void resumeMapAnimation() {
        synchronized (_mapRenderer) {
            _mapAnimationStartTime = SystemClock.uptimeMillis();
            _mapAnimationFinished = false;
            if (_mapAnimator != null) {
                _mapAnimator.resume();
            }
        }
    }

    public final boolean isMapAnimationPaused() {
        boolean result = true;
        synchronized (_mapRenderer) {
            if (_mapAnimator != null) {
                result = _mapAnimator.isPaused();
            }
        }
        return result;
    }

    public final void pauseMapAnimation() {
        synchronized (_mapRenderer) {
            if (_mapAnimator != null) {
                _mapAnimator.pause();
            }
        }
    }

    public final void stopMapAnimation() {
        synchronized (_mapRenderer) {
            if (_mapAnimator != null) {
                _mapAnimator.pause();
                _mapAnimator.getAllAnimations();
            }
        }
    }

    public final boolean isMapAnimationFinished() {
        return _mapAnimationFinished;
    }

    public final void resumeMapMarkersAnimation() {
        synchronized (_mapRenderer) {
            _mapMarkersAnimationStartTime = SystemClock.uptimeMillis();
            _mapMarkersAnimationFinished = false;
            if (_mapMarkersAnimator != null) {
                _mapMarkersAnimator.resume();
            }
        }
    }

    public final boolean isMapMarkersAnimationPaused() {
        boolean result = true;
        synchronized (_mapRenderer) {
            if (_mapMarkersAnimator != null) {
                result = _mapMarkersAnimator.isPaused();
            }
        }
        return result;
    }

    public final void pauseMapMarkersAnimation() {
        synchronized (_mapRenderer) {
            if (_mapMarkersAnimator != null) {
                _mapMarkersAnimator.pause();
            }
        }
    }

    public final void stopMapMarkersAnimation() {
        synchronized (_mapRenderer) {
            if (_mapMarkersAnimator != null) {
                _mapMarkersAnimator.pause();
                _mapMarkersAnimator.getAllAnimations();
            }
        }
    }

    public final boolean isMapMarkersAnimationFinished() {
        return _mapMarkersAnimationFinished;
    }

    public static String getEglErrorString(int error) {
        switch (error) {
            case EGL10.EGL_SUCCESS:
                return "EGL_SUCCESS";
            case EGL10.EGL_NOT_INITIALIZED:
                return "EGL_NOT_INITIALIZED";
            case EGL10.EGL_BAD_ACCESS:
                return "EGL_BAD_ACCESS";
            case EGL10.EGL_BAD_ALLOC:
                return "EGL_BAD_ALLOC";
            case EGL10.EGL_BAD_ATTRIBUTE:
                return "EGL_BAD_ATTRIBUTE";
            case EGL10.EGL_BAD_CONFIG:
                return "EGL_BAD_CONFIG";
            case EGL10.EGL_BAD_CONTEXT:
                return "EGL_BAD_CONTEXT";
            case EGL10.EGL_BAD_CURRENT_SURFACE:
                return "EGL_BAD_CURRENT_SURFACE";
            case EGL10.EGL_BAD_DISPLAY:
                return "EGL_BAD_DISPLAY";
            case EGL10.EGL_BAD_MATCH:
                return "EGL_BAD_MATCH";
            case EGL10.EGL_BAD_NATIVE_PIXMAP:
                return "EGL_BAD_NATIVE_PIXMAP";
            case EGL10.EGL_BAD_NATIVE_WINDOW:
                return "EGL_BAD_NATIVE_WINDOW";
            case EGL10.EGL_BAD_PARAMETER:
                return "EGL_BAD_PARAMETER";
            case EGL10.EGL_BAD_SURFACE:
                return "EGL_BAD_SURFACE";
            case EGL11.EGL_CONTEXT_LOST:
                return "EGL_CONTEXT_LOST";
            default:
                return "0x" + Integer.toHexString(error);
        }
    }

    public static boolean isMSAASupported() {
        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        if (display == EGL10.EGL_NO_DISPLAY) {
            return false;
        }

        int[] version = new int[2];
        if (!egl.eglInitialize(display, version)) {
            return false;
        }

        int[] configSpec = {
            EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT | EGL10.EGL_PBUFFER_BIT,
            EGL10.EGL_RED_SIZE, RED_SIZE,
            EGL10.EGL_GREEN_SIZE, GREEN_SIZE,
            EGL10.EGL_BLUE_SIZE, BLUE_SIZE,
            EGL10.EGL_ALPHA_SIZE, ALPHA_SIZE,
            EGL10.EGL_DEPTH_SIZE, DEPTH_SIZE,
            EGL10.EGL_STENCIL_SIZE, STENCIL_SIZE,
            EGL10.EGL_SAMPLE_BUFFERS, 1,
            EGL10.EGL_SAMPLES, 2, 
            EGL10.EGL_RENDERABLE_TYPE, 0x0040,
            EGL10.EGL_NONE
        };

        int[] numConfig = new int[1];
        if (!egl.eglChooseConfig(display, configSpec, null, 0, numConfig)) {
            egl.eglTerminate(display);
            return false;
        }

        boolean hasMSAA = numConfig[0] > 0;
        egl.eglTerminate(display);
        return hasMSAA;
    }

    private static final int RED_SIZE = 8;
    private static final int GREEN_SIZE = 8;
    private static final int BLUE_SIZE = 8;
    private static final int ALPHA_SIZE = 0;
    private static final int DEPTH_SIZE = 16;
    private static final int STENCIL_SIZE = 0;

    private static boolean msaaEnabled = false;

    public static boolean isMSAAEnabled() {
        return msaaEnabled;
    }

    public static void setMSAAEnabled(boolean enableMSAA) {
        msaaEnabled = isMSAASupported() && enableMSAA;
    }

    private abstract class BaseConfigChooser
            implements GLSurfaceView.EGLConfigChooser {
        public BaseConfigChooser(int[] configSpec) {
            mConfigSpec = filterConfigSpec(configSpec);
        }

        public EGLConfig makeConfig(EGL10 egl, EGLDisplay display) {
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(display, mConfigSpec, null, 0, num_config)) {
                Log.e(TAG, "Failed to choose EGL config");
                return null;
            }

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                Log.e(TAG, "No EGL configs match specified requirements");
                return null;
            }

            EGLConfig[] configs = new EGLConfig[numConfigs];
            if (!egl.eglChooseConfig(display, mConfigSpec, configs, numConfigs, num_config)) {
                Log.e(TAG, "Failed to choose suitable EGL configs");
                return null;
            }
            EGLConfig config = chooseConfig(egl, display, configs);
            if (config == null) {
                Log.e(TAG, "Failed to choose required EGL config");
            }
            return config;
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            Log.v(TAG, "ComponentSizeChooser.chooseConfig()...");
            synchronized (eglThread) {
                eglThread.startAndCompleteOperation(EGLThreadOperation.CHOOSE_CONFIG);
            }
            return makeConfig(egl, display);
        }

        abstract EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                                        EGLConfig[] configs);

        protected int[] mConfigSpec;

        private int[] filterConfigSpec(int[] configSpec) {
            /* We know none of the subclasses define EGL_RENDERABLE_TYPE.
             * And we know the configSpec is well formed.
             */
            int len = configSpec.length;
            int[] newConfigSpec = new int[len + 2];
            System.arraycopy(configSpec, 0, newConfigSpec, 0, len-1);
            newConfigSpec[len-1] = EGL10.EGL_RENDERABLE_TYPE;
            newConfigSpec[len] = 0x0040; /* EGL_OPENGL_ES3_BIT_KHR */
            newConfigSpec[len+1] = EGL10.EGL_NONE;
            return newConfigSpec;
        }
    }

    /**
     * Choose a configuration with exactly the specified r,g,b,a sizes,
     * and at least the specified depth and stencil sizes.
     */
    private class ComponentSizeChooser extends BaseConfigChooser {
        public ComponentSizeChooser(int redSize, int greenSize, int blueSize,
                                    int alphaSize, int depthSize, int stencilSize) {
            super(new int[] {
                    EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT | EGL10.EGL_PBUFFER_BIT,
                    EGL10.EGL_RED_SIZE, redSize,
                    EGL10.EGL_GREEN_SIZE, greenSize,
                    EGL10.EGL_BLUE_SIZE, blueSize,
                    EGL10.EGL_ALPHA_SIZE, alphaSize,
                    EGL10.EGL_DEPTH_SIZE, depthSize,
                    EGL10.EGL_STENCIL_SIZE, stencilSize,
                    EGL10.EGL_NONE});
            mValue = new int[1];
            mRedSize = redSize;
            mGreenSize = greenSize;
            mBlueSize = blueSize;
            mAlphaSize = alphaSize;
            mDepthSize = depthSize;
            mStencilSize = stencilSize;
        }

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                                      EGLConfig[] configs) {
            EGLConfig bestMSAA4xConfig = null;
            EGLConfig bestOtherMSAAConfig = null;
            EGLConfig bestNonMSAAConfig = null;
            int maxMSAA4xDepth = -1;
            int maxOtherMSAADepth = -1;
            int maxOtherMSAASamples = -1;
            int maxNonMSAADepth = -1;

            for (EGLConfig config : configs) {
                int depth = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
                int stencil = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);
                if (depth < mDepthSize || stencil < mStencilSize) {
                    continue;
                }

                int red = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
                int green = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
                int blue = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
                int alpha = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);
                if (red != mRedSize || green != mGreenSize || blue != mBlueSize || alpha != mAlphaSize) {
                    continue;
                }

                int sampleBuffers = findConfigAttrib(egl, display, config, EGL10.EGL_SAMPLE_BUFFERS, 0);
                int samples = findConfigAttrib(egl, display, config, EGL10.EGL_SAMPLES, 0);

                if (sampleBuffers == 1 && samples >= 2) {
                    if (samples == 4) {
                        // Prefer 4x MSAA
                        if (depth >= maxMSAA4xDepth) {
                            bestMSAA4xConfig = config;
                            maxMSAA4xDepth = depth;
                        }
                    } else {
                        // Other MSAA levels
                        if (samples > maxOtherMSAASamples ||
                            (samples == maxOtherMSAASamples && depth >= maxOtherMSAADepth)) {
                            bestOtherMSAAConfig = config;
                            maxOtherMSAADepth = depth;
                            maxOtherMSAASamples = samples;
                        }
                    }
                } else if (sampleBuffers == 0) {
                    if (depth >= maxNonMSAADepth) {
                        bestNonMSAAConfig = config;
                        maxNonMSAADepth = depth;
                    }
                }
            }

            if (isMSAAEnabled()) {
                return bestMSAA4xConfig != null ? bestMSAA4xConfig :
                       bestOtherMSAAConfig != null ? bestOtherMSAAConfig :
                       bestNonMSAAConfig;
            } else {
                return bestNonMSAAConfig != null ? bestNonMSAAConfig :
                       bestOtherMSAAConfig != null ? bestOtherMSAAConfig :
                       bestMSAA4xConfig;
            }
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display,
                                     EGLConfig config, int attribute, int defaultValue) {

            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }

        private int[] mValue;
        // Subclasses can adjust these values:
        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;
    }

    private class WindowSurfaceFactory implements GLSurfaceView.EGLWindowSurfaceFactory {

        public WindowSurfaceFactory() {
            dummySurfaceAttributes = new int[] {
                EGL10.EGL_WIDTH, 1,
                EGL10.EGL_HEIGHT, 1,
                EGL10.EGL_NONE
            };
        }

        public EGLSurface createWindowSurface(EGL10 egl, EGLDisplay display, EGLConfig config, Object nativeWindow) {
            Log.v(TAG, "WindowSurfaceFactory.createWindowSurface()...");
            synchronized (eglThread) {
                eglThread.nativeWindow = nativeWindow;
                eglThread.startAndCompleteOperation(EGLThreadOperation.CREATE_WINDOW_SURFACE);
            }

            // Create dummy surface
            EGLSurface result = null;
            try {
                result = egl.eglCreatePbufferSurface(display, config, dummySurfaceAttributes);
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "Failed to create dummy window surface", e);
            }
            return result;
        }

        public void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface) {
            Log.v(TAG, "WindowSurfaceFactory.destroySurface()...");
            egl.eglDestroySurface(display, surface);
            synchronized (eglThread) {
                eglThread.startAndCompleteOperation(EGLThreadOperation.DESTROY_SURFACE);
            }
        }

        private int[] dummySurfaceAttributes;
    }

    private class PixelbufferSurfaceFactory implements GLSurfaceView.EGLWindowSurfaceFactory {

        public PixelbufferSurfaceFactory(int width, int height) {
            surfaceWidth = width;
            surfaceHeight = height;
            dummySurfaceAttributes = new int[] {
                EGL10.EGL_WIDTH, 1,
                EGL10.EGL_HEIGHT, 1,
                EGL10.EGL_NONE
            };
        }

        public EGLSurface createWindowSurface(EGL10 egl, EGLDisplay display, EGLConfig config, Object nativeWindow) {
            Log.v(TAG, "PixelbufferSurfaceFactory.createWindowSurface()...");
            synchronized (eglThread) {
                eglThread.surfaceWidth = surfaceWidth;
                eglThread.surfaceHeight = surfaceHeight;
                eglThread.startAndCompleteOperation(EGLThreadOperation.CREATE_PIXELBUFFER_SURFACE);
            }

            // Create dummy surface
            EGLSurface result = null;
            try {
                result = egl.eglCreatePbufferSurface(display, config, dummySurfaceAttributes);
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "Failed to create dummy window surface", e);
            }
            return result;
        }

        public void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface) {
            Log.v(TAG, "PixelbufferSurfaceFactory.destroySurface()...");
            egl.eglDestroySurface(display, surface);
            synchronized (eglThread) {
                eglThread.startAndCompleteOperation(EGLThreadOperation.DESTROY_SURFACE);
            }
        }

        private int surfaceWidth;
        private int surfaceHeight;
        private int[] dummySurfaceAttributes;
    }

    /**
     * EGL context factory
     * <p/>
     * Implements creation of main and GPU-worker contexts along with needed resources
     */
    private final class EGLContextFactory implements GLSurfaceView.EGLContextFactory {

        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig config) {
            Log.v(TAG, "EGLContextFactory.createContext()...");

            if (isSuspended && !isReinitializing) {
                Log.e(TAG, "No use to create contexts for moved renderer");
                return null;
            }

            // In case context is created while rendering is initialized, release it first
            if (!isReinitializing && _mapRenderer != null && _mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Rendering is still initialized during context creation, force releasing it!");

                // Since there's no more context, where previous resources were created,
                // they are lost. Forcibly release rendering
                releaseRendering();
            }

            if (config == null) {
                return null;
            }

            synchronized (eglThread) {
                eglThread.startAndCompleteOperation(EGLThreadOperation.CREATE_CONTEXTS);
            }

            // Create dummy EGL context for GLSurfaceView only
            EGLContext dummyContext;
            try {
                dummyContext = egl.eglCreateContext(
                        display,
                        config,
                        EGL10.EGL_NO_CONTEXT,
                        eglThread.contextAttributes);
            } catch (Exception e) {
                Log.e(TAG, "Failed to create dummy EGL context", e);
                return null;
            }
            if (dummyContext == null || dummyContext == EGL10.EGL_NO_CONTEXT) {
                Log.e(TAG, "Failed to create dummy EGL context: " + getEglErrorString(egl.eglGetError()));
                return null;
            }

            return dummyContext;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            Log.v(TAG, "EGLContextFactory.destroyContext()...");

            // In case context is destroyed while rendering is initialized, release it first
            if (!isSuspended && _mapRenderer != null && _mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Rendering is still initialized during context destruction, force releasing it!");

                // Since there's no more context, where previous resources were created,
                // they are lost. Forcibly release rendering
                releaseRendering();
            }

            // Destroy dummy context
            egl.eglDestroyContext(display, context);

            if (!isSuspended) {
                synchronized (eglThread) {
                    eglThread.startAndCompleteOperation(EGLThreadOperation.DESTROY_CONTEXTS);
                }
            }
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

            if (isSuspended && !isReinitializing) {
                return;
            }

            // New context was created, but rendering was initialized before,
            // so, release rendering to allow it to initialize on next
            // call to onSurfaceChanged
            if (!isReinitializing && _mapRenderer != null && _mapRenderer.isRenderingInitialized()) {
                Log.v(TAG, "Release rendering due to context recreation");
                releaseRendering();
            }
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            Log.v(TAG, "RendererProxy.onSurfaceChanged()...");

            if (isSuspended && !isReinitializing) {
                return;
            }

            // Set new "window" size and viewport that covers entire "window"
            _windowWidth = width;
            _windowHeight = height;
            if (_mapRenderer != null) {
                _mapRenderer.setWindowSize(new PointI(width, height));
                _mapRenderer.setViewport(new AreaI(new PointI(0, 0), new PointI(width, height)));
                _mapRenderer.setFlip(!_inWindow);
            }

            // Setup rendering options
            if (_mapRenderer != null && !_mapRenderer.isRenderingInitialized()) {
                if (eglThread.gpuWorkerContext != null && eglThread.gpuWorkerFakeSurface != null) {
                    setupOptions.setGpuWorkerThreadEnabled(true);
                    setupOptions.setGpuWorkerThreadPrologue(_gpuWorkerThreadPrologue.getBinding());
                    setupOptions.setGpuWorkerThreadEpilogue(_gpuWorkerThreadEpilogue.getBinding());
                } else {
                    setupOptions.setGpuWorkerThreadEnabled(false);
                    setupOptions.setGpuWorkerThreadPrologue(null);
                    setupOptions.setGpuWorkerThreadEpilogue(null);
                }
                setupOptions.setFrameUpdateRequestCallback(_renderRequestCallback.getBinding());
                _mapRenderer.setup(setupOptions);
            }

            // In case rendering is not initialized or just needs to be reinitialized, initialize it
            // (happens when surface is created for the first time, or recreated)
            if (_mapRenderer != null && (isReinitializing || !_mapRenderer.isRenderingInitialized())) {
                Log.v(TAG, "Rendering is initializing...");
                boolean ok = false;
                synchronized (eglThread) {
                    eglThread.ok = false;
                    eglThread.mapRenderer = _mapRenderer;
                    eglThread.surfaceWidth = width;
                    eglThread.surfaceHeight = height;
                    eglThread.startAndCompleteOperation(EGLThreadOperation.INITIALIZE_RENDERING);
                    ok = eglThread.ok;
                }
                if (ok) {
                    _exportableMapRenderer = _mapRenderer;
                    if (isReinitializing) {
                        isReinitializing = false;
                        isSuspended = false;
                    }
                    _frameStartTime = SystemClock.uptimeMillis();
                    _frameRenderTime = 0;
                    Log.v(TAG, "Rendering is initialized successfully");
                } else {
                    _exportableMapRenderer = null;
                    isReinitializing = false;
                    Log.e(TAG, "Failed to initialize rendering");
                }
                isInitializing = false;
            }
        }

        public void onDrawFrame(GL10 gl) {
            // In case rendering was not ready yet, don't do anything
            if (isSuspended || _mapRenderer == null || !_mapRenderer.isRenderingInitialized()) {
                Log.w(TAG, "Can't draw a frame: renderer either suspended or isn't yet initialized");
                return;
            }

            long waitTime = _frameGPUFinishTime + _mapRenderer.getWaitTime();

            long currTime = SystemClock.uptimeMillis();

            _frameTotalTime = Math.max(currTime - _frameStartTime, 1);
            _frameStartTime = currTime;
            _frameIdleTime = (float) (_frameTotalTime - _frameRenderTime) / (float) _frameTotalTime;
            _frameGPUWaitTime = (float) waitTime / (float) _frameTotalTime;
            _frameIdleTimeLast1K = _frameIdleTimeLast1K > 0.0 ? (_frameIdleTimeLast1K * 999.0f + _frameIdleTime) / 1000.0f : _frameIdleTime;
            _frameGPUWaitTimeLast1K = _frameGPUWaitTimeLast1K > 0.0 ? (_frameGPUWaitTimeLast1K * 999.0f + _frameGPUWaitTime) / 1000.0f : _frameGPUWaitTime;

            _mapAnimationFinished = _mapAnimator.update((currTime - _mapAnimationStartTime) / 1000f);
            _mapAnimationStartTime = currTime;
            _mapMarkersAnimationFinished = _mapMarkersAnimator.update((currTime - _mapMarkersAnimationStartTime) / 1000f);
            _mapMarkersAnimationStartTime = currTime;

            for (MapRendererViewListener listener : listeners) {
                listener.onUpdateFrame(MapRendererView.this);
            }

            long preFlushTime;
            synchronized (eglThread) {
                eglThread.mapRenderer = _mapRenderer;
                eglThread.byteBuffer = _byteBuffer;
                eglThread.renderingResultIsReady = false;
                eglThread.startAndCompleteOperation(EGLThreadOperation.RENDER_FRAME);
                if (eglThread.renderingResultIsReady) {
                    _renderingResultIsReady = true;
                }
                preFlushTime = eglThread.preFlushTime;
            }

            frameId++;

            if (!_frameReadingMode && _byteBuffer != null) {
                for (MapRendererViewListener listener : listeners) {
                    listener.onFrameReady(MapRendererView.this);
                }
            }

            long postRenderTime = SystemClock.uptimeMillis();

            _frameGPUFinishTime = postRenderTime - preFlushTime;
            _frameRenderTime = Math.max(postRenderTime - _frameStartTime, 1);
            _frameRate = 1000.0f / (float) _frameRenderTime;
            _frameRateLast1K =
                _frameRateLast1K > 0.0 ? (_frameRateLast1K * 999.0f + _frameRate) / 1000.0f : _frameRate;

            if (_maxFrameRate > 0) {
                long extraTime = _maxFrameTime - _frameRenderTime;
                if (extraTime > 0) {
                    try {
                        synchronized (_frameRateLock) {
                            if (_frameRateChanged) {
                                _frameRateChanged = false;
                            } else {
                                _frameRateLock.wait(extraTime);
                            }
                        }
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }
    }

    /**
     * Callback handler to request frame render
     */
    private class RenderRequestCallback
            extends MapRendererSetupOptions.IFrameUpdateRequestCallback {
        @Override
        public void method(IMapRenderer mapRenderer) {
            if (!isSuspended && _renderingView != null) {
                _renderingView.requestRender();
            }
        }
    }


    /**
     * GPU-worker thread prologue
     */
    private class GpuWorkerThreadPrologue
            extends MapRendererSetupOptions.IGpuWorkerThreadPrologue {
        @Override
        public void method(IMapRenderer mapRenderer) {
            if (eglThread.display == null) {
                Log.e(TAG, "EGL display is missing");
                return;
            }

            if (eglThread.gpuWorkerContext == null) {
                Log.e(TAG, "GPU-worker context is missing");
                return;
            }

            if (eglThread.gpuWorkerFakeSurface == null) {
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
                        eglThread.display,
                        eglThread.gpuWorkerFakeSurface,
                        eglThread.gpuWorkerFakeSurface,
                        eglThread.gpuWorkerContext)) {
                    Log.e(TAG, "Failed to set GPU-worker EGL context active: " +
                            getEglErrorString(egl.eglGetError()));
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
                            getEglErrorString(egl.eglGetError()));
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to wait for GPU-worker EGL context", e);
            }
        }
    }

    public interface IMapRendererSetupOptionsConfigurator {
        void configureMapRendererSetupOptions(MapRendererSetupOptions mapRendererSetupOptions);
    }

    public class RenderingView extends GLSurfaceView {

        public boolean isOffscreen = false;

        public RenderingView(Context context) {
            super(context);
        }

        public void initializeView(int bitmapWidth, int bitmapHeight) {
            if (isOffscreen) {
                super.surfaceCreated(null);
                super.surfaceChanged(null, 0, bitmapWidth, bitmapHeight);
            }
        }

        public void removeView() {
            if (isOffscreen) {
                super.onDetachedFromWindow();
            }
        }
    }

    private class EGLThread extends Thread {

        private EGL10 egl;
        private EGLDisplay display;
        private EGLConfig config;
        private EGLContext context;
        private EGLSurface surface;
        GL10 gl;

        protected volatile EGLContext gpuWorkerContext;
        protected volatile EGLSurface gpuWorkerFakeSurface;

        protected volatile ComponentSizeChooser configChooser;

        protected volatile Object nativeWindow;

        protected volatile int surfaceWidth;
        protected volatile int surfaceHeight;

        protected volatile IMapRenderer mapRenderer;

        protected volatile ByteBuffer byteBuffer;
        protected volatile boolean renderingResultIsReady;
        protected volatile boolean stopThread;
        protected volatile boolean isStopped;

        private final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

        /**
         * EGL attributes used to initialize EGL context:
         * - EGL context must support at least OpenGLES 3.0
         */
        protected final int[] contextAttributes = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL10.EGL_NONE
        };

        /**
         * EGL attributes used to initialize GPU-worker EGL surface with 1x1 pixels size
         */
        private final int[] gpuWorkerSurfaceAttributes = {
            EGL10.EGL_WIDTH, 1,
            EGL10.EGL_HEIGHT, 1,
            EGL10.EGL_NONE
        };

        public volatile EGLThreadOperation eglThreadOperation = EGLThreadOperation.NO_OPERATION;
        public volatile boolean ok;
        public volatile long preFlushTime = SystemClock.uptimeMillis();

        public EGLThread(String name) {
            super(name);
        }

        // NOTE: It needs to be called from synchronized block
        public void startAndCompleteOperation(EGLThreadOperation operation) {
            eglThreadOperation = operation;
            notifyAll();
            while (eglThreadOperation != EGLThreadOperation.NO_OPERATION) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }

        @Override
        public void run() {
            egl = (EGL10) EGLContext.getEGL();
            while (!stopThread) {
                synchronized (this) {
                    switch (eglThreadOperation) {
                        case CHOOSE_CONFIG:
                        if (display == null) {
                            display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
                            if (display == EGL10.EGL_NO_DISPLAY) {
                                Log.e(TAG, "Failed to get EGL display");
                                display = null;
                            }
                            int[] version = new int[2];
                            if(!egl.eglInitialize(display, version)) {
                                Log.e(TAG, "Failed to initialize EGL display connection");
                                display = null;
                            }
                        }
                        if (display != null && config == null) {
                            config = configChooser.makeConfig(egl, display);
                        }
                        break;

                        case CREATE_CONTEXTS:
                        // Create GPU-worker EGL context if needed
                        if (gpuWorkerContext == null) {
                            try {
                                gpuWorkerContext = egl.eglCreateContext(
                                        display,
                                        config,
                                        EGL10.EGL_NO_CONTEXT,
                                        contextAttributes);
                            } catch (Exception e) {
                                Log.e(TAG, "Failed to create GPU-worker EGL context", e);
                            }
                            if (gpuWorkerContext == null || gpuWorkerContext == EGL10.EGL_NO_CONTEXT) {
                                Log.e(TAG, "Failed to create GPU-worker EGL context: " +
                                        getEglErrorString(egl.eglGetError()));
                                gpuWorkerContext = null;
                            }
                            // Create GPU-worker EGL surface
                            if (gpuWorkerContext != null) {
                                if (gpuWorkerFakeSurface != null) {
                                    Log.w(TAG, "Previous GPU-worker EGL surface was not destroyed properly!");
                                    gpuWorkerFakeSurface = null;
                                }
                                try {
                                    gpuWorkerFakeSurface = egl.eglCreatePbufferSurface(
                                            display,
                                            config,
                                            gpuWorkerSurfaceAttributes);
                                } catch (Exception e) {
                                    Log.e(TAG, "Failed to create GPU-worker EGL surface", e);
                                    gpuWorkerFakeSurface = null;
                                }
                                if (gpuWorkerFakeSurface == null || gpuWorkerFakeSurface == EGL10.EGL_NO_SURFACE) {
                                    Log.e(TAG, "Failed to create GPU-worker EGL surface: " +
                                            getEglErrorString(egl.eglGetError()));

                                    egl.eglDestroyContext(display, gpuWorkerContext);
                                    gpuWorkerContext = null;
                                    gpuWorkerFakeSurface = null;
                                }
                            }
                        }
                        // Create main EGL context
                        if (context == null) {
                            try {
                                context = egl.eglCreateContext(
                                        display,
                                        config,
                                        gpuWorkerContext != null ? gpuWorkerContext : EGL10.EGL_NO_CONTEXT,
                                        contextAttributes);
                            } catch (Exception e) {
                                Log.e(TAG, "Failed to create main EGL context", e);
                            }
                            if (context == null || context == EGL10.EGL_NO_CONTEXT) {
                                Log.e(TAG, "Failed to create main EGL context: "
                                    + getEglErrorString(egl.eglGetError()));
                                context = null;
                            }
                        }

                        break;

                        case CREATE_WINDOW_SURFACE:
                        surface = null;
                        if (display != null && config != null) {
                            try {
                                surface = egl.eglCreateWindowSurface(display, config, nativeWindow, null);
                            } catch (IllegalArgumentException e) {
                                Log.e(TAG, "Failed to create window surface", e);
                            }
                        }
                        if (surface == null || surface == EGL10.EGL_NO_SURFACE) {
                            surface = null;
                        } else {
                            try {
                                if (!egl.eglMakeCurrent(display, surface, surface, context)) {
                                    Log.e(TAG, "Failed to set main EGL context to window active: "
                                        + getEglErrorString(egl.eglGetError()));
                                }
                            } catch (Exception e) {
                                Log.e(TAG, "Failed to set main EGL context to window active", e);
                            }
                            gl = (GL10) context.getGL();
                        }
                        break;

                        case CREATE_PIXELBUFFER_SURFACE:
                        surface = null;
                        if (display != null && config != null) {
                            int[] surfaceAttributes = new int[] {
                                EGL10.EGL_WIDTH, surfaceWidth,
                                EGL10.EGL_HEIGHT, surfaceHeight,
                                EGL10.EGL_NONE
                            };
                            try {
                                surface = egl.eglCreatePbufferSurface(display, config, surfaceAttributes);
                            } catch (IllegalArgumentException e) {
                                Log.e(TAG, "Failed to create pixelbuffer surface", e);
                            }
                        }
                        if (surface == null || surface == EGL10.EGL_NO_SURFACE) {
                            surface = null;
                        } else {
                            try {
                                if (!egl.eglMakeCurrent(display, surface, surface, context)) {
                                    Log.e(TAG, "Failed to set main EGL context to pixelbuffer active: "
                                        + getEglErrorString(egl.eglGetError()));
                                }
                            } catch (Exception e) {
                                Log.e(TAG, "Failed to set main EGL context to pixelbuffer active", e);
                            }
                            gl = (GL10) context.getGL();
                        }
                        break;

                        case INITIALIZE_RENDERING:
                        ok = mapRenderer.initializeRendering(false);
                        requestRender();
                        break;

                        case RENDER_FRAME:
                        mapRenderer.update();
                        boolean isReady = mapRenderer.prepareFrame();
                        if (isReady) {
                            mapRenderer.renderFrame();
                        }
                        preFlushTime = SystemClock.uptimeMillis();
                        if (isReady) {
                            gl.glFlush();
                            if (byteBuffer != null) {
                                gl.glFinish();
                                if (_frameReadingMode) {
                                    egl.eglSwapBuffers(display, surface);
                                }
                                synchronized (byteBuffer) {
                                    byteBuffer.rewind();
                                    gl.glReadPixels(0, 0, surfaceWidth, surfaceHeight,
                                        GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, byteBuffer);
                                    renderingResultIsReady = true;
                                }
                            } else {
                                egl.eglSwapBuffers(display, surface);
                            }
                            requestRender();
                        }
                        break;

                        case RELEASE_RENDERING:
                        _mapAnimationFinished = true;
                        _mapMarkersAnimationFinished = true;
                        mapRenderer.releaseRendering(true);
                        break;

                        case DESTROY_SURFACE:
                        if (display != null && surface != null) {
                            egl.eglMakeCurrent(display, EGL10.EGL_NO_SURFACE,
                                EGL10.EGL_NO_SURFACE,
                                EGL10.EGL_NO_CONTEXT);
                            egl.eglDestroySurface(display, surface);
                            surface = null;
                        }
                        break;

                        case DESTROY_CONTEXTS:
                        if (display != null) {
                            // Destroy main context
                            if (context != null) {
                                egl.eglDestroyContext(display, context);
                                context = null;
                            }
                            // Destroy GPU-worker EGL surface (if present)
                            if (gpuWorkerFakeSurface != null) {
                                egl.eglDestroySurface(display, gpuWorkerFakeSurface);
                                gpuWorkerFakeSurface = null;
                            }
                            // Destroy GPU-worker EGL context (if present)
                            if (gpuWorkerContext != null) {
                                egl.eglDestroyContext(display, gpuWorkerContext);
                                gpuWorkerContext = null;
                            }
                            // Remove EGL config
                            if (config != null) {
                                config = null;
                            }
                            // Terminate connection to EGL display
                            egl.eglTerminate(display);
                            display = null;
                        }
                    }
                    eglThreadOperation = EGLThreadOperation.NO_OPERATION;
                    notifyAll();
                    try {
                        wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
            synchronized (this) {
                isStopped = true;
                notifyAll();
            }
        }
    }
}

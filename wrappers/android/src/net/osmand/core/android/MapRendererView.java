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
import net.osmand.core.jni.FColorRGB;
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
import net.osmand.core.jni.MapRendererState;
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
    private final static long RELEASE_WAIT_TIMEOUT = 400;

    /**
     * Main GLSurfaceView
     */
    private volatile GLSurfaceView _glSurfaceView;

    /**
     * Reference to OsmAndCore::IMapRenderer instance
     */
    protected IMapRenderer _mapRenderer;

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

    private int frameId;

    private boolean isPresent;
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

    public void setupRenderer(Context context, int bitmapWidth, int bitmapHeight, MapRendererView oldView) {
        NativeCore.checkIfLoaded();

        boolean inWindow = (bitmapWidth == 0 || bitmapHeight == 0);

        synchronized (this) {
            if (!inWindow)
                _byteBuffer = ByteBuffer.allocateDirect(bitmapWidth * bitmapHeight * 4);

                // Get display density factor
            _densityFactor = inWindow ? getResources().getDisplayMetrics().density : 1.0f;

            // Create instance of OsmAndCore::IMapRenderer
            if (oldView == null)
                _mapRenderer = createMapRendererInstance();
            else
                _mapRenderer = oldView.getRenderer();

            // Create GLSurfaceView and add it to hierarchy
            if (inWindow && _glSurfaceView != null)
                removeAllViews();
            _glSurfaceView = new GLSurfaceView(context);
            if (inWindow)
                addView(_glSurfaceView, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
            else {
                _windowWidth = bitmapWidth;
                _windowHeight = bitmapHeight;
            }

            // Create map animator for that map
            if (_mapAnimator == null)
                _mapAnimator = new MapAnimator(false);
            _mapAnimator.setMapRenderer(_mapRenderer);

            // Create map markers animator
            if (_mapMarkersAnimator == null)
                _mapMarkersAnimator = new MapMarkersAnimator();
            _mapMarkersAnimator.setMapRenderer(_mapRenderer);

            // Configure GLSurfaceView
            _glSurfaceView.setPreserveEGLContextOnPause(true);
            _glSurfaceView.setEGLContextClientVersion(3);
            _glSurfaceView.setEGLConfigChooser(new ComponentSizeChooser(8, 8, 8, 0, 16, 0, inWindow));
            _glSurfaceView.setEGLContextFactory(new EGLContextFactory());
            if (!inWindow)
                _glSurfaceView.setEGLWindowSurfaceFactory(new PixelbufferSurfaceFactory(bitmapWidth, bitmapHeight));
            _glSurfaceView.setRenderer(new RendererProxy());
            _glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
            if (!inWindow) {
                _glSurfaceView.surfaceCreated(null);
                _glSurfaceView.surfaceChanged(null, 0, bitmapWidth, bitmapHeight);
                isPresent = true;
            }
        }
    }

    @Override
    public void setVisibility(int visibility) {
        if (_glSurfaceView != null)
            _glSurfaceView.setVisibility(visibility);
        super.setVisibility(visibility);
    }

    public void addListener(MapRendererViewListener listener) {
        if (!this.listeners.contains(listener)) {
            List<MapRendererViewListener> listeners = new ArrayList<>();
            listeners.addAll(this.listeners);
            listeners.add(listener);
            this.listeners = listeners;
        }
    }

    public void removeListener(MapRendererViewListener listener) {
        if (this.listeners.contains(listener)) {
            List<MapRendererViewListener> listeners = new ArrayList<>();
            listeners.addAll(this.listeners);
            listeners.remove(listener);
            this.listeners = listeners;
        }
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

    public Bitmap getBitmap() {
        if (_byteBuffer != null) {
            if (_renderingResultIsReady) {
                Bitmap bitmap = Bitmap.createBitmap(_windowWidth, _windowHeight, Bitmap.Config.ARGB_8888);
                synchronized (_byteBuffer) {
                    _byteBuffer.rewind();
                    bitmap.copyPixelsFromBuffer(_byteBuffer);
                    _renderingResultIsReady = false;
                }
                Matrix matrix = new Matrix();
                matrix.preScale(1.0f, -1.0f);
                _resultBitmap = Bitmap.createBitmap(bitmap, 0, 0, _windowWidth, _windowHeight, matrix, true);
            }
            if (_resultBitmap == null)
                _resultBitmap = Bitmap.createBitmap(_windowWidth, _windowHeight, Bitmap.Config.ARGB_8888);
            return _resultBitmap;
        }
        else
            return null;
    }

    public IMapRenderer getRenderer() {
        return _mapRenderer;
    }

    public void stopRenderer() {
        Log.v(TAG, "removeRendering()");
        NativeCore.checkIfLoaded();

        if (isPresent && _glSurfaceView != null) {
            isPresent = false;
            Log.v(TAG, "Rendering release due to removeRendering()");
            _glSurfaceView.onPause();
            releaseRendering();
            removeAllViews();
            _byteBuffer = null;
            _resultBitmap = null;
            _mapAnimator = null;
            _mapMarkersAnimator = null;
            _glSurfaceView = null;
        }
    }

    private void releaseRendering() {
        if(releaseTask == null && _glSurfaceView != null) {
            waitRelease = true;
            releaseTask = new Runnable() {
                @Override
                public void run() {
                    if (_mapRenderer.isRenderingInitialized()) {
                        Log.v(TAG, "Forcibly releasing rendering by schedule");
                        _mapRenderer.releaseRendering(true);
                    }
                    synchronized (this) {
                        waitRelease = false;
                        this.notifyAll();
                    }                
                }
            };
            _glSurfaceView.queueEvent(releaseTask);
        }
        if(_glSurfaceView != null) {
            synchronized (releaseTask) {
                long stopTime = System.currentTimeMillis();
                while(waitRelease){
                    if (System.currentTimeMillis() > stopTime + RELEASE_STOP_TIMEOUT)
                        return;
                    try {
                        releaseTask.wait(RELEASE_WAIT_TIMEOUT);
                    } catch (InterruptedException ex) {}
                }
            }
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        Log.v(TAG, "onAttachedToWindow()");
        isPresent = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        Log.v(TAG, "onDetachedFromWindow()");
        NativeCore.checkIfLoaded();

        if (isPresent) {
            isPresent = false;
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
        isPresent = true;
    }

    public final void handleOnSaveInstanceState(Bundle outState) {
        Log.v(TAG, "handleOnSaveInstanceState()");
        NativeCore.checkIfLoaded();

        //TODO: do something here
    }

    public final void handleOnDestroy() {
        Log.v(TAG, "handleOnDestroy()");
        NativeCore.checkIfLoaded();

        if (isPresent) {
            isPresent = false;
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

        // Inform GLSurfaceView that activity was paused
        if (_glSurfaceView != null)
            _glSurfaceView.onPause();
    }

    public final void handleOnResume() {
        Log.v(TAG, "handleOnResume()");
        NativeCore.checkIfLoaded();

        // Inform GLSurfaceView that activity was resumed
        if (_glSurfaceView != null)
            _glSurfaceView.onResume();
    }

    public final void requestRender() {
        //Log.v(TAG, "requestRender()");
        NativeCore.checkIfLoaded();

        // Request GLSurfaceView render a frame
        if (_glSurfaceView != null)
            _glSurfaceView.requestRender();
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
        if (fixedPixel.getX() >= 0 && fixedPixel.getY() >= 0)
            return _mapRenderer.getState().getFixedLocation31();
        else
            return _mapRenderer.getState().getTarget31();
    }

    public final PointI getTargetScreenPosition() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getState().getFixedPixel();
    }

    public final boolean setTarget(PointI target31) {
        NativeCore.checkIfLoaded();

        if (_windowWidth > 0 && _windowHeight > 0)
            return _mapRenderer.setMapTargetLocation(target31);
        else
            return _mapRenderer.setTarget(target31);
    }

    public final boolean setTarget(PointI target31, float heightInMeters) {
        NativeCore.checkIfLoaded();

        if (_windowWidth > 0 && _windowHeight > 0)
            return _mapRenderer.setMapTargetLocation(target31, heightInMeters);
        else
            return _mapRenderer.setTarget(target31);
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

    public final boolean setSymbolsOpacity(float opacityFactor) {
        NativeCore.checkIfLoaded();

        return _mapRenderer.setSymbolsOpacity(opacityFactor);
    }

    public final float getSymbolsOpacity() {
        NativeCore.checkIfLoaded();

        return _mapRenderer.getSymbolsOpacity();
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

    public final void resumeMapAnimation() {
        _mapAnimationStartTime = SystemClock.uptimeMillis();
        _mapAnimationFinished = false;
        _mapAnimator.resume();
    }

    public final boolean isMapAnimationPaused() {
        return _mapAnimator.isPaused();
    }

    public final void pauseMapAnimation() {
        _mapAnimator.pause();
    }

    public final void stopMapAnimation() {
        _mapAnimator.pause();
        _mapAnimator.getAllAnimations();
    }

    public final boolean isMapAnimationFinished() {
        return _mapAnimationFinished;
    }

    public final void resumeMapMarkersAnimation() {
        _mapMarkersAnimationStartTime = SystemClock.uptimeMillis();
        _mapMarkersAnimationFinished = false;
        _mapMarkersAnimator.resume();
    }

    public final boolean isMapMarkersAnimationPaused() {
        return _mapMarkersAnimator.isPaused();
    }

    public final void pauseMapMarkersAnimation() {
        _mapMarkersAnimator.pause();
    }

    public final void stopMapMarkersAnimation() {
        _mapMarkersAnimator.pause();
        _mapMarkersAnimator.getAllAnimations();
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

    private class PixelbufferSurfaceFactory implements GLSurfaceView.EGLWindowSurfaceFactory {

        public PixelbufferSurfaceFactory(int width, int height) {
            surfaceAttributes = new int[] {
                    EGL10.EGL_WIDTH, width,
                    EGL10.EGL_HEIGHT, height,
                    EGL10.EGL_NONE};
        }

        public EGLSurface createWindowSurface(EGL10 egl, EGLDisplay display,
                EGLConfig config, Object nativeWindow) {
            EGLSurface result = null;
            try {
                result = egl.eglCreatePbufferSurface(display, config, surfaceAttributes);
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "eglCreateWindowSurface", e);
            }
            return result;
        }

        public void destroySurface(EGL10 egl, EGLDisplay display,
                EGLSurface surface) {
            egl.eglDestroySurface(display, surface);
        }

        private int[] surfaceAttributes;
    }

    private abstract class BaseConfigChooser
            implements GLSurfaceView.EGLConfigChooser {
        public BaseConfigChooser(int[] configSpec) {
            mConfigSpec = filterConfigSpec(configSpec);
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(display, mConfigSpec, null, 0,
                    num_config)) {
                Log.e(TAG, "Failed to choose EGL config");
            }

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                Log.e(TAG, "No EGL configs match specified requirements");
            }

            EGLConfig[] configs = new EGLConfig[numConfigs];
            if (!egl.eglChooseConfig(display, mConfigSpec, configs, numConfigs,
                    num_config)) {
                Log.e(TAG, "Failed to choose suitable EGL configs");
            }
            EGLConfig config = chooseConfig(egl, display, configs);
            if (config == null) {
                Log.e(TAG, "Failed to choose required EGL config");
            }
            return config;
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
                                    int alphaSize, int depthSize, int stencilSize, boolean inWindow) {
            super(new int[] {
                    EGL10.EGL_SURFACE_TYPE, inWindow ? EGL10.EGL_WINDOW_BIT : EGL10.EGL_PBUFFER_BIT,
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
            EGLConfig bestConfig = null;
            int maxDepthSize = -1;
            for (EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config,
                        EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib(egl, display, config,
                        EGL10.EGL_STENCIL_SIZE, 0);
                if ((d >= mDepthSize) && (s >= mStencilSize)) {
                    int r = findConfigAttrib(egl, display, config,
                            EGL10.EGL_RED_SIZE, 0);
                    int g = findConfigAttrib(egl, display, config,
                            EGL10.EGL_GREEN_SIZE, 0);
                    int b = findConfigAttrib(egl, display, config,
                            EGL10.EGL_BLUE_SIZE, 0);
                    int a = findConfigAttrib(egl, display, config,
                            EGL10.EGL_ALPHA_SIZE, 0);
                    if ((r == mRedSize) && (g == mGreenSize)
                            && (b == mBlueSize) && (a == mAlphaSize)) {
                        if (d > maxDepthSize) {
                            maxDepthSize = d;
                            bestConfig = config;
                        }
                    }
                }
            }
            return bestConfig;
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

    /**
     * EGL context factory
     * <p/>
     * Implements creation of main and GPU-worker contexts along with needed resources
     */
    private final class EGLContextFactory implements GLSurfaceView.EGLContextFactory {
        private int EGL_CONTEXT_CLIENT_VERSION = 0x3098;        
        /**
         * EGL attributes used to initialize EGL context:
         * - EGL context must support at least OpenGLES 3.0
         */
        private final int[] contextAttributes = {
                EGL_CONTEXT_CLIENT_VERSION, 3,
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
                        getEglErrorString(egl.eglGetError()));

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
                        getEglErrorString(egl.eglGetError()));
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
                            getEglErrorString(egl.eglGetError()));

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
            _mapRenderer.setViewport(new AreaI(new PointI(0, 0), new PointI(width, height)));

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

            long currTime = SystemClock.uptimeMillis();
            _mapAnimationFinished = _mapAnimator.update((currTime - _mapAnimationStartTime) / 1000f);
            _mapAnimationStartTime = currTime;
            _mapMarkersAnimationFinished = _mapMarkersAnimator.update((currTime - _mapMarkersAnimationStartTime) / 1000f);
            _mapMarkersAnimationStartTime = currTime;

            for (MapRendererViewListener listener : listeners) {
                listener.onUpdateFrame(MapRendererView.this);
            }

            // Allow renderer to update
            _mapRenderer.update();

            // In case a new frame was prepared, render it
            if (_mapRenderer.prepareFrame()) {
                frameId++;
                _mapRenderer.renderFrame();
            }


            // Flush all the commands to GPU
            gl.glFlush();

            // Read the result raster to byte buffer when rendering off-screen
            if (_byteBuffer != null) {
                gl.glFinish();
                synchronized (_byteBuffer) {
                    _byteBuffer.rewind();
                    gl.glReadPixels(0, 0, _windowWidth, _windowHeight, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, _byteBuffer);
                    _renderingResultIsReady = true;
                }
                for (MapRendererViewListener listener : listeners) {
                    listener.onFrameReady(MapRendererView.this);
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
            if (_glSurfaceView != null)
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
}

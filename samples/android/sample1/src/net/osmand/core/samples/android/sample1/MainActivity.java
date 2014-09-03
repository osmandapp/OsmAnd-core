package net.osmand.core.samples.android.sample1;

import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import net.osmand.core.jni.*;

public class MainActivity extends ActionBarActivity {
    static {
        try {
            System.loadLibrary("gnustl_shared");
        }
        catch( UnsatisfiedLinkError e ) {
            System.err.println("Failed to load 'gnustl_shared':" + e);
            System.exit(0);
        }
        try {
            System.loadLibrary("Qt5Core");
        }
        catch( UnsatisfiedLinkError e ) {
            System.err.println("Failed to load 'Qt5Core':" + e);
            System.exit(0);
        }
        try {
            System.loadLibrary("Qt5Network");
        }
        catch( UnsatisfiedLinkError e ) {
            System.err.println("Failed to load 'Qt5Network':" + e);
            System.exit(0);
        }
        try {
            System.loadLibrary("Qt5Sql");
        }
        catch( UnsatisfiedLinkError e ) {
            System.err.println("Failed to load 'Qt5Sql':" + e);
            System.exit(0);
        }
        try {
            System.loadLibrary("OsmAndCore_shared");
        }
        catch( UnsatisfiedLinkError e ) {
            System.err.println("Failed to load 'OsmAndCore_shared':" + e);
            System.exit(0);
        }
        try {
            System.loadLibrary("OsmAndCoreJNI");
        }
        catch( UnsatisfiedLinkError e ) {
            System.err.println("Failed to load 'OsmAndCoreJNI':" + e);
            System.exit(0);
        }
    }

    private static final String TAG = "OsmAndCoreSample";

    private IMapStylesCollection _mapStylesCollection;
    private MapStyle _mapStyle;
    private ObfsCollection _obfsCollection;
    private MapPresentationEnvironment _mapPresentationEnvironment;
    private Primitiviser _primitiviser;
    private BinaryMapDataProvider _binaryMapDataProvider;
    private BinaryMapPrimitivesProvider _binaryMapPrimitivesProvider;
    private BinaryMapStaticSymbolsProvider _binaryMapStaticSymbolsProvider;
    private BinaryMapRasterBitmapTileProvider _binaryMapRasterBitmapTileProvider;
    private IMapRenderer _mapRenderer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.i(TAG, "Initializing core...");
        OsmAndCore.InitializeCore();

        Log.i(TAG, "Going to resolve default embedded style...");
        _mapStylesCollection = new MapStylesCollection();
        _mapStyle = _mapStylesCollection.getBakedStyle("default");
        if (_mapStyle == null)
        {
            Log.e(TAG, "Failed to resolve style 'default'");
            System.exit(0);
        }

        Log.i(TAG, "Going to prepare OBFs collection");
        _obfsCollection = new ObfsCollection();
        _obfsCollection.addDirectory(Environment.getExternalStorageState() + "/osmand", true);

        Log.i(TAG, "Going to prepare all resources for renderer");
        _mapPresentationEnvironment = new MapPresentationEnvironment(
                _mapStyle,
                1.0f, //TODO: Here should be DPI
                "en"); //TODO: here should be current locale
        //mapPresentationEnvironment->setSettings(configuration.styleSettings);
        _primitiviser = new Primitiviser(
                _mapPresentationEnvironment);
        _binaryMapDataProvider = new BinaryMapDataProvider(
                _obfsCollection);
        _binaryMapPrimitivesProvider = new BinaryMapPrimitivesProvider(
                _binaryMapDataProvider,
                _primitiviser,
                256);
        _binaryMapStaticSymbolsProvider = new BinaryMapStaticSymbolsProvider(
                _binaryMapPrimitivesProvider,
                256);
        _binaryMapRasterBitmapTileProvider = new BinaryMapRasterBitmapTileProvider_Software(
                _binaryMapPrimitivesProvider);

        Log.i(TAG, "Going to create renderer");
        _mapRenderer = OsmAndCore.createMapRenderer(MapRendererClass.AtlasMapRenderer_OpenGLES2);
        if (_mapRenderer == null)
        {
            Log.e(TAG, "Failed to create map renderer 'AtlasMapRenderer_OpenGLES2'");
            System.exit(0);
        }

        MapRendererSetupOptions rendererSetupOptions = new MapRendererSetupOptions();
        /*
        rendererSetup.frameUpdateRequestCallback = []()
        {
            //QMutexLocker scopedLocker(&glutWasInitializedFlagMutex);

            if(glutWasInitialized)
                glutPostRedisplay();
        };
        rendererSetup.gpuWorkerThread.enabled = useGpuWorker;
        if(rendererSetup.gpuWorkerThread.enabled)
        {
            #if defined(WIN32)
            const auto currentDC = wglGetCurrentDC();
            const auto currentContext = wglGetCurrentContext();
            const auto workerContext = wglCreateContext(currentDC);

            rendererSetup.gpuWorkerThread.enabled = (wglShareLists(currentContext, workerContext) == TRUE);
            assert(currentContext == wglGetCurrentContext());

            rendererSetup.gpuWorkerThread.prologue = [currentDC, workerContext]()
            {
                const auto result = (wglMakeCurrent(currentDC, workerContext) == TRUE);
                verifyOpenGL();
            };

            rendererSetup.gpuWorkerThread.epilogue = []()
            {
                glFinish();
            };
            #endif
        }
        */
        _mapRenderer.setup(rendererSetupOptions);
        /*
        viewport.top = 0;
        viewport.left = 0;
        viewport.bottom = 600;
        viewport.right = 800;
        renderer->setWindowSize(OsmAnd::PointI(800, 600));
        renderer->setViewport(viewport);
        */

        _mapRenderer.setAzimuth(0.0f);
        _mapRenderer.setElevationAngle(35.0f);

        // Amsterdam via Mapnik
        _mapRenderer.setTarget(new PointI(
            1102430866,
            704978668));
        _mapRenderer.setZoom(10.0f);
        _mapRenderer.setRasterLayerProvider(RasterMapLayerId.BaseLayer, OnlineTileSources.getBuiltIn().createProviderFor("Mapnik (OsmAnd)"));

        _glSurfaceView = (GLSurfaceView) findViewById(R.id.glSurfaceView);
        _glSurfaceView.setEGLContextClientVersion(2);
        _glSurfaceView.setEGLConfigChooser(true);
        //TODO:_glSurfaceView.setPreserveEGLContextOnPause(true);
        _glSurfaceView.setRenderer(new Renderer());
        _glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    private GLSurfaceView _glSurfaceView;

    private class Renderer implements GLSurfaceView.Renderer {
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            //TODO: create second context
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            /*_mapRenderer.setViewport(new AreaI(0, 0, height, width));
            _mapRenderer.setWindowSize(new PointI(width, height));

            if (!_mapRenderer.getIsRenderingInitialized())
                _mapRenderer.initializeRendering();*/
        }

        public void onDrawFrame(GL10 gl) {
            /*if (_mapRenderer.prepareFrame())
                _mapRenderer.renderFrame();
            _mapRenderer.processRendering();*/
        }
    }

    @Override
    protected void onDestroy() {
        if (_mapStylesCollection != null) {
            _mapStylesCollection.delete();
            _mapStylesCollection = null;
        }

        if (_mapStyle != null) {
            _mapStyle.delete();
            _mapStyle = null;
        }

        if (_obfsCollection != null) {
            _obfsCollection.delete();
            _obfsCollection = null;
        }

        if (_mapPresentationEnvironment != null) {
            _mapPresentationEnvironment.delete();
            _mapPresentationEnvironment = null;
        }

        if (_primitiviser != null) {
            _primitiviser.delete();
            _primitiviser = null;
        }

        if (_binaryMapDataProvider != null) {
            _binaryMapDataProvider.delete();
            _binaryMapDataProvider = null;
        }

        if (_binaryMapPrimitivesProvider != null) {
            _binaryMapPrimitivesProvider.delete();
            _binaryMapPrimitivesProvider = null;
        }

        if (_binaryMapStaticSymbolsProvider != null) {
            _binaryMapStaticSymbolsProvider.delete();
            _binaryMapStaticSymbolsProvider = null;
        }

        if (_binaryMapRasterBitmapTileProvider != null) {
            _binaryMapRasterBitmapTileProvider.delete();
            _binaryMapRasterBitmapTileProvider = null;
        }

        if (_mapRenderer != null) {
            _mapRenderer.delete();
            _mapRenderer = null;
        }

        OsmAndCore.ReleaseCore();

        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}

package net.osmand.core.samples.android.sample1;

import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import net.osmand.core.jni.*;

public class MainActivity extends ActionBarActivity {
    private MapStyles _mapStyles;
    private MapStyle _mapStyle;
    private ObfsCollection _obfsCollection;
    private IMapRenderer _mapRenderer;

    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("OsmAndCore_android");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        OsmAndCore.InitializeCore();

//        _mapStyles = new MapStyles();
//        _mapStyle = new MapStyle();

//        _obfsCollection = new ObfsCollection();
//        _obfsCollection.watchDirectory(Environment.getExternalStorageState() + "/osmand", true);
        /*
        renderer = OsmAnd::createAtlasMapRenderer_OpenGL3();
        if(!renderer)
        {
            std::cout << "No supported renderer" << std::endl;
            OsmAnd::ReleaseCore();
            return EXIT_FAILURE;
        }
        */

        /*
        OsmAnd::MapRendererSetupOptions rendererSetup;
        rendererSetup.frameUpdateRequestCallback = []()
        {
            //QMutexLocker scopedLocker(&glutWasInitializedFlagMutex);

            if(glutWasInitialized)
                glutPostRedisplay();
        };
        rendererSetup.displayDensityFactor = density;
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
        renderer->setup(rendererSetup);
        viewport.top = 0;
        viewport.left = 0;
        viewport.bottom = 600;
        viewport.right = 800;
        renderer->setWindowSize(OsmAnd::PointI(800, 600));
        renderer->setViewport(viewport);
        */

        /*
        renderer->setAzimuth(0.0f);
        renderer->setElevationAngle(35.0f);
        renderer->setFogColor(OsmAnd::FColorRGB(1.0f, 1.0f, 1.0f));

        // Amsterdam
        renderer->setTarget(OsmAnd::PointI(
                1102430866,
                704978668));
        renderer->setZoom(10.0f);
        */

        /*
        auto renderConfig = renderer->configuration;
        renderConfig.heixelsPerTileSide = 32;
        renderer->setConfiguration(renderConfig);

        renderer->initializeRendering();
        */

        _glSurfaceView = (GLSurfaceView) findViewById(R.id.glSurfaceView);
        _glSurfaceView.setEGLContextClientVersion(2);
        _glSurfaceView.setEGLConfigChooser(true);
        _glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        _glSurfaceView.setRenderer(new Renderer());
    }

    private GLSurfaceView _glSurfaceView;

    private class Renderer implements GLSurfaceView.Renderer {
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
//            _mapRenderer.initializeRendering();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            /*viewport.top = 0;
            viewport.left = 0;
            viewport.bottom = 600;
            viewport.right = 800;
            renderer->setWindowSize(OsmAnd::PointI(800, 600));
            renderer->setViewport(viewport);
            _mapRenderer.setWindowSize();
            _mapRenderer.setViewport();*/
        }

        public void onDrawFrame(GL10 gl) {
//            if (_mapRenderer.prepareFrame())
//                _mapRenderer.renderFrame();
//            _mapRenderer.processRendering();
        }
    }

    @Override
    protected void onDestroy() {
        if (_obfsCollection != null) {
            _obfsCollection.delete();
            _obfsCollection = null;
        }

        if (_mapStyle != null) {
            _mapStyle.delete();
            _mapStyle = null;
        }
        if (_mapStyles != null) {
            _mapStyles.delete();
            _mapStyles = null;
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

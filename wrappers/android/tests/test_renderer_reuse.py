"""Test the actual renderer handoff and EGL thread ownership methods in isolation.

Run with Python 3 and JDK 17+ on PATH, or set JAVA_HOME. An optional first
argument selects a different repository checkout (for regression controls).
Platform/native dependencies use minimal doubles; this does not compile the app
or validate Android lifecycle ordering, real EGL drivers, or rendering output.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[3]


def block(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]



core = (ROOT / 'wrappers/android/src/net/osmand/core/android/MapRendererView.java').read_text()

core_methods = '\n'.join(block(core, signature) for signature in [
    'public synchronized void setupRenderer(',
    'public synchronized IMapRenderer suspendRenderer()',
    'public synchronized boolean prepareRendererForReuse()',
    'public synchronized void stopRenderer()',
    'private void releaseRendering()',
    'public void stopRenderingView()',
    'public final void handleOnPause()',
    'private static class EGLThread extends Thread',
])
core_test = r'''
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

class MapRendererView {
    static String TAG = "test";
    static class Log {
        static void v(String a, String b) {}
        static void w(String a, String b) {}
        static void e(String a, String b) {}
        static void e(String a, String b, Exception e) {}
    }
    static class NativeCore { static void checkIfLoaded() {} }
    static class Context {}
    static class Metrics { float density = 1; }
    static class Resources { Metrics getDisplayMetrics() { return new Metrics(); } }
    Resources getResources() { return new Resources(); }
    static class IMapRenderer {
        boolean initialized = true;
        boolean isRenderingInitialized() { return initialized; }
        boolean initializeRendering(boolean flag) { initialized = true; return true; }
        void releaseRendering(boolean flag) { initialized = false; }
        void setup(Options o) {}
        void update() {}
        boolean prepareFrame() { return true; }
        boolean renderFrame() { return true; }
    }
    static class Options {
        void setDisplayDensityFactor(float v) {}
        void setGpuWorkerThreadEnabled(boolean v) {}
        void setGpuWorkerThreadPrologue(Object v) {}
        void setGpuWorkerThreadEpilogue(Object v) {}
        void setFrameUpdateRequestCallback(Object v) {}
    }
    static class Callback { Object getBinding() { return this; } }
    static class MapAnimator {
        MapAnimator(boolean flag) {}
        void setMapRenderer(IMapRenderer r) {}
    }
    static class MapMarkersAnimator { void setMapRenderer(IMapRenderer r) {} }
    static class RenderingView { int pauses; void onPause() { pauses++; } }
    static class EGLDisplay {}
    static class EGLConfig {}
    static class EGLSurface {}
    static class EGLContext {
        static Object getEGL() { return new EGL10(); }
        Object getGL() { return new GL10(); }
    }
    static class GL10 {
        static int GL_RGBA = 1, GL_UNSIGNED_BYTE = 2;
        void glFlush() {}
        void glFinish() {}
        void glReadPixels(int x, int y, int w, int h, int f, int t, ByteBuffer b) {}
    }
    static class EGL10 {
        static int EGL_DEFAULT_DISPLAY=0, EGL_WIDTH=1, EGL_HEIGHT=2, EGL_NONE=3;
        static EGLDisplay EGL_NO_DISPLAY;
        static EGLContext EGL_NO_CONTEXT;
        static EGLSurface EGL_NO_SURFACE;
        int swaps;
        EGLDisplay eglGetDisplay(int d) { return new EGLDisplay(); }
        boolean eglInitialize(EGLDisplay d, int[] v) { return true; }
        EGLContext eglCreateContext(EGLDisplay d, EGLConfig c, EGLContext x, int[] a) { return new EGLContext(); }
        EGLSurface eglCreatePbufferSurface(EGLDisplay d, EGLConfig c, int[] a) { return new EGLSurface(); }
        EGLSurface eglCreateWindowSurface(EGLDisplay d, EGLConfig c, Object w, Object a) { return new EGLSurface(); }
        boolean eglMakeCurrent(EGLDisplay d, EGLSurface a, EGLSurface b, EGLContext c) { return true; }
        void eglDestroyContext(EGLDisplay d, EGLContext c) {}
        void eglDestroySurface(EGLDisplay d, EGLSurface s) {}
        void eglSwapBuffers(EGLDisplay d, EGLSurface s) { swaps++; }
        void eglTerminate(EGLDisplay d) {}
        int eglGetError() { return 0; }
    }
    static class ComponentSizeChooser {
        EGLConfig makeConfig(EGL10 egl, EGLDisplay display) { return new EGLConfig(); }
    }
    static class SystemClock { static long uptimeMillis() { return System.currentTimeMillis(); } }
    static String getEglErrorString(int error) { return "test"; }
    enum EGLThreadOperation { NO_OPERATION, CHOOSE_CONFIG, CREATE_CONTEXTS, CREATE_WINDOW_SURFACE,
        CREATE_PIXELBUFFER_SURFACE, INITIALIZE_RENDERING, RENDER_FRAME, RELEASE_RENDERING,
        DESTROY_SURFACE, DESTROY_CONTEXTS }
    IMapRenderer _mapRenderer, _exportableMapRenderer;
    EGLThread eglThread;
    MapAnimator _mapAnimator;
    MapMarkersAnimator _mapMarkersAnimator;
    Options setupOptions = new Options();
    Callback _gpuWorkerThreadPrologue = new Callback(), _gpuWorkerThreadEpilogue = new Callback(),
        _renderRequestCallback = new Callback();
    int _windowWidth, _windowHeight, removals, maxFrameRate;
    boolean _inWindow, isSuspended, isInitializing, isReinitializing, isPaused, initOnResume,
        isViewStarted = true, _frameReadingMode, _isRenderingActive,
        _mapAnimationFinished, _mapMarkersAnimationFinished;
    ByteBuffer _byteBuffer;
    List<Object> listeners = new ArrayList<>();
    RenderingView _renderingView = new RenderingView();
    void removeRenderingView() { removals++; _renderingView = null; }
    void startRenderingView(Context context) { isViewStarted = true; }
    void setMaximumFrameRate(int rate) { maxFrameRate = rate; }
    int getMaximumFrameRate() { return maxFrameRate; }
    IMapRenderer createMapRendererInstance() { return new IMapRenderer(); }

    __METHODS__

    static int checks;
    static void check(boolean ok, String name) {
        checks++;
        if (!ok) throw new AssertionError(name);
    }
    static MapRendererView ready() {
        MapRendererView view = new MapRendererView();
        view._mapRenderer = view._exportableMapRenderer = new IMapRenderer();
        view.eglThread = new EGLThread("isolated-test", view);
        view.eglThread.gl = new GL10();
        view.eglThread.setDaemon(true);
        view.maxFrameRate = 20;
        return view;
    }
    public static void main(String[] args) throws Exception {
        MapRendererView empty = new MapRendererView();
        check(!empty.prepareRendererForReuse() && empty.suspendRenderer() == null, "absent renderer");
        MapRendererView notReady = ready();
        notReady._mapRenderer.initialized = false;
        check(!notReady.prepareRendererForReuse(), "uninitialized renderer");
        MapRendererView unpublished = ready();
        unpublished._exportableMapRenderer = null;
        check(!unpublished.prepareRendererForReuse(), "unpublished renderer");

        MapRendererView direct = ready();
        IMapRenderer immediate = direct.suspendRenderer();
        check(immediate == direct._mapRenderer, "immediate Android Auto handoff");
        check(direct.suspendRenderer() == null && !direct.prepareRendererForReuse(), "no duplicate direct export");

        MapRendererView old = ready();
        old.handleOnPause();
        check(!old.isSuspended && old._exportableMapRenderer != null && old.removals == 0, "background pause stays a pause");
        IMapRenderer renderer = old._mapRenderer;
        EGLThread thread = old.eglThread;
        check(old.prepareRendererForReuse(), "prepare before activity destruction");
        check(old.prepareRendererForReuse() && old.removals == 1, "idempotent prepare");
        check(renderer.initialized && old._exportableMapRenderer == renderer, "keep native resources available");
        check(old.listeners.isEmpty() && old._mapAnimator == null && old._mapMarkersAnimator == null, "detach old listeners/animators");
        MapRendererView next = new MapRendererView();
        next.setupRenderer(new Context(), 0, 0, old);
        check(next._mapRenderer == renderer && next.eglThread == thread, "reuse native renderer and thread");
        check(next.maxFrameRate == 20 && next.isReinitializing, "retain frame rate and rebind surface");
        check(old.suspendRenderer() == null && !old.prepareRendererForReuse(), "prepared export consumed once");
        check(thread.rendererView == next, "thread follows new view");
        for (java.lang.reflect.Field field : EGLThread.class.getDeclaredFields()) {
            check(!field.getName().startsWith("this$"), "no implicit original Activity reference");
        }

        thread.mapRenderer = renderer;
        thread.byteBuffer = ByteBuffer.allocateDirect(4);
        next._frameReadingMode = true;
        thread.start();
        synchronized (thread) { thread.startAndCompleteOperation(EGLThreadOperation.RENDER_FRAME); }
        check(next._isRenderingActive && !old._isRenderingActive, "frame state belongs to receiving view");
        check(thread.egl.swaps == 1, "frame reading mode belongs to receiving view");
        next.stopRenderer();
        check(next._mapAnimationFinished && next._mapMarkersAnimationFinished && !old._mapAnimationFinished,
            "release state belongs to receiving view");
        check(!renderer.initialized && thread.isStopped, "normal destruction still releases and stops");

        MapRendererView abandoned = ready();
        abandoned.eglThread.start();
        check(abandoned.prepareRendererForReuse(), "prepare abandonment");
        abandoned.stopRenderer();
        check(!abandoned._mapRenderer.initialized && abandoned.suspendRenderer() == null, "abandoned handoff releases normally");
        System.out.println("Core renderer handoff and thread ownership: " + checks + " assertions passed");
    }
}
'''.replace('__METHODS__', core_methods)

with tempfile.TemporaryDirectory(prefix='renderer-isolated-tests-') as temp:
    work = Path(temp)
    source_file = work / 'MapRendererView.java'
    source_file.write_text(core_test)
    java_bin = Path(os.environ['JAVA_HOME']) / 'bin' if 'JAVA_HOME' in os.environ else Path('')
    subprocess.run([str(java_bin / 'javac'), '-d', temp, str(source_file)], check=True)
    subprocess.run([str(java_bin / 'java'), '-cp', temp, 'MapRendererView'], check=True, timeout=15)

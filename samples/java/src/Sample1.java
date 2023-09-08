package net.osmand.core.samples;

import java.awt.Frame;

import javax.media.opengl.GLEventListener;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.awt.GLCanvas;

import com.jogamp.opengl.util.Animator;

import net.osmand.core.jni.*;

public class Sample1 implements GLEventListener {
  private final static boolean LOAD_STATIC = true;
  public static void main(String[] args) {
    try {
      System.loadLibrary("gnustl_shared");
    }
    catch( UnsatisfiedLinkError e ) {
      System.err.println("Failed to load gnustl_shared:" + e);
      System.exit(0);
    }
    try {
      System.loadLibrary("Qt5Core");
    }
    catch( UnsatisfiedLinkError e ) {
      System.err.println("Failed to load Qt5Core:" + e);
      System.exit(0);
    }
    try {
      System.loadLibrary("Qt5Network");
    }
    catch( UnsatisfiedLinkError e ) {
      System.err.println("Failed to load Qt5Network:" + e);
      System.exit(0);
    }
    try {
      System.loadLibrary("Qt5Sql");
    }
    catch( UnsatisfiedLinkError e ) {
      System.err.println("Failed to load Qt5Sql:" + e);
      System.exit(0);
    }
    if (!LOAD_STATIC) {
      try {
        System.loadLibrary("OsmAndCore_shared");
      }
      catch( UnsatisfiedLinkError e ) {
        System.err.println("Failed to load OsmAndCore_shared:" + e);
        System.exit(0);
      }
      try {
        System.loadLibrary("OsmAndCoreJNI");
      }
      catch( UnsatisfiedLinkError e ) {
        System.err.println("Failed to load OsmAndCoreJNI:" + e);
        System.exit(0);
      }
    } else {
       try {
        System.loadLibrary("OsmAndCoreWithJNI");
      }
      catch( UnsatisfiedLinkError e ) {
        System.err.println("Failed to load OsmAndCoreWithJNI:" + e);
        System.exit(0);
      }
    }
    _coreResourcesEmbeddedBundle = CoreResourcesEmbeddedBundle.loadFromLibrary("OsmAndCore_ResourcesBundle_shared");

    final Animator animator = new Animator();

    GLCanvas canvas = new GLCanvas();
    final Sample1 sample1 = new Sample1(canvas);

    Frame frame = new Frame("OsmAnd Core API samples for Java");
    frame.setSize(800, 600);
    frame.setLayout(new java.awt.BorderLayout());
    frame.addWindowListener(new java.awt.event.WindowAdapter() {
      @Override
      public void windowClosing(java.awt.event.WindowEvent e) {
        new Thread(new Runnable() {
          @Override
          public void run() {
            animator.stop();
            OsmAndCore.ReleaseCore();
            System.exit(0);
          }
        }).start();
      }
    });

    animator.add(canvas);
    canvas.addGLEventListener(sample1);

    frame.add(canvas, java.awt.BorderLayout.CENTER);

    frame.validate();
    frame.setVisible(true);
    animator.start();
  }

  private Sample1(final GLCanvas canvas) {
    _canvas = canvas;
  }

  private static CoreResourcesEmbeddedBundle _coreResourcesEmbeddedBundle;

  private GLCanvas _canvas;

  private float _displayDensityFactor;
  private int _referenceTileSize;
  private int _rasterTileSize;
  private IMapStylesCollection _mapStylesCollection;
  private ResolvedMapStyle _mapStyle;
  private ObfsCollection _obfsCollection;
  private MapPresentationEnvironment _mapPresentationEnvironment;
  private MapPrimitiviser _mapPrimitiviser;
  private ObfMapObjectsProvider _obfMapObjectsProvider;
  private MapPrimitivesProvider _mapPrimitivesProvider;
  private MapObjectsSymbolsProvider _mapObjectsSymbolsProvider;
  private MapRasterLayerProvider _mapRasterLayerProvider;
  private IMapRenderer _mapRenderer;
  private RenderRequestCallback _renderRequestCallback;
  private QIODeviceLogSink _fileLogSink;

  @Override
  public void init(GLAutoDrawable drawable) {
    OsmAndCore.InitializeCore(_coreResourcesEmbeddedBundle);

    _fileLogSink = QIODeviceLogSink.createFileLogSink("osmandcore.log");
    Logger.get().addLogSink(_fileLogSink);

    _displayDensityFactor = 1.0f;
    _referenceTileSize = 256;
    _rasterTileSize = 256;

    _mapStylesCollection = new MapStylesCollection();
    _mapStyle = _mapStylesCollection.getResolvedStyleByName("default");
    if (_mapStyle == null) {
      System.err.println("Failed to resolve style 'default'");
      release();
      OsmAndCore.ReleaseCore();
      System.exit(0);
    }

    _obfsCollection = new ObfsCollection();
    _obfsCollection.addDirectory("data", false);

    _mapPresentationEnvironment = new MapPresentationEnvironment(
      _mapStyle,
      _displayDensityFactor,
	  1.0f,
	  1.0f);
    _mapPresentationEnvironment.setLocaleLanguageId("en"); //TODO: here should be current locale
    _mapPrimitiviser = new MapPrimitiviser(
      _mapPresentationEnvironment);
    _obfMapObjectsProvider = new ObfMapObjectsProvider(
      _obfsCollection);
    _mapPrimitivesProvider = new MapPrimitivesProvider(
      _obfMapObjectsProvider,
      _mapPrimitiviser,
      _rasterTileSize);
    _mapObjectsSymbolsProvider = new MapObjectsSymbolsProvider(
      _mapPrimitivesProvider,
      _rasterTileSize);
    _mapRasterLayerProvider = new MapRasterLayerProvider_Software(
      _mapPrimitivesProvider);

    _mapRenderer = OsmAndCore.createMapRenderer(MapRendererClass.AtlasMapRenderer_OpenGL2plus);
    if (_mapRenderer == null) {
      System.err.println("Failed to create map renderer 'AtlasMapRenderer_OpenGL2plus'");
      release();
      OsmAndCore.ReleaseCore();
      System.exit(0);
    }

    MapRendererSetupOptions rendererSetupOptions = new MapRendererSetupOptions();
    rendererSetupOptions.setGpuWorkerThreadEnabled(false);
    _renderRequestCallback = new RenderRequestCallback();
    rendererSetupOptions.setFrameUpdateRequestCallback(_renderRequestCallback.getBinding());
    _mapRenderer.setup(rendererSetupOptions);

    AtlasMapRendererConfiguration atlasRendererConfiguration = AtlasMapRendererConfiguration.Casts.upcastFrom(_mapRenderer.getConfiguration());
    atlasRendererConfiguration.setReferenceTileSizeOnScreenInPixels(_referenceTileSize);
    _mapRenderer.setConfiguration(AtlasMapRendererConfiguration.Casts.downcastTo_MapRendererConfiguration(atlasRendererConfiguration));

    _mapRenderer.addSymbolsProvider(_mapObjectsSymbolsProvider);
    _mapRenderer.setAzimuth(0.0f);
    _mapRenderer.setElevationAngle(35.0f);

    _mapRenderer.setTarget(new PointI(
      1102430866,
      704978668));
    _mapRenderer.setZoom(10.0f);
    /*
    IMapRasterLayerProvider mapnik = OnlineTileSources.getBuiltIn().createProviderFor("Mapnik (OsmAnd)");
    if (mapnik == null)
      Log.e(TAG, "Failed to create mapnik");
    */
    _mapRenderer.setMapLayerProvider(0, _mapRasterLayerProvider);
  }

  private class RenderRequestCallback extends MapRendererSetupOptions.IFrameUpdateRequestCallback {
    @Override
    public void method(IMapRenderer mapRenderer) {
      _canvas.repaint();
    }
  }

  @Override
  public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
    _mapRenderer.setViewport(new AreaI(0, 0, height, width));
    _mapRenderer.setWindowSize(new PointI(width, height));

    if (!_mapRenderer.isRenderingInitialized()) {
      if (!_mapRenderer.initializeRendering(true))
        System.err.println("Failed to initialize rendering");
    }
  }

  @Override
  public void display(GLAutoDrawable drawable) {
    if (_mapRenderer == null || !_mapRenderer.isRenderingInitialized())
        return;
    _mapRenderer.update();

    if (_mapRenderer.prepareFrame())
      _mapRenderer.renderFrame();
  }

  @Override
  public void dispose(GLAutoDrawable drawable) {
    if (_mapRenderer != null && _mapRenderer.isRenderingInitialized())
        _mapRenderer.releaseRendering();
    release();
  }

  public void release() {
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

    if (_mapPrimitiviser != null) {
      _mapPrimitiviser.delete();
      _mapPrimitiviser = null;
    }

    if (_obfMapObjectsProvider != null) {
      _obfMapObjectsProvider.delete();
      _obfMapObjectsProvider = null;
    }

    if (_mapPrimitivesProvider != null) {
      _mapPrimitivesProvider.delete();
      _mapPrimitivesProvider = null;
    }

    if (_mapObjectsSymbolsProvider != null) {
      _mapObjectsSymbolsProvider.delete();
      _mapObjectsSymbolsProvider = null;
    }

    if (_mapRasterLayerProvider != null) {
      _mapRasterLayerProvider.delete();
      _mapRasterLayerProvider = null;
    }

    if (_mapRenderer != null) {
      _mapRenderer.delete();
      _mapRenderer = null;
    }
  }
}

package net.osmand.core.samples;

import java.awt.Frame;

import javax.media.opengl.GLEventListener;
import javax.media.opengl.GLAutoDrawable;

import net.osmand.core.jni.*;

public class Sample1 implements GLEventListener {
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

    Frame frame = new Frame("OsmAnd Core API samples for Java");
    frame.setSize(800, 600);
    frame.setLayout(new java.awt.BorderLayout());    

    frame.validate();
    frame.setVisible(true);
  }

  private float _displayDensityFactor;
  private int _referenceTileSize;
  private int _rasterTileSize;
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
  public void init(GLAutoDrawable drawable) {
    OsmAndCore.InitializeCore();

    _displayDensityFactor = 1.0f;
    _referenceTileSize = 256;
    _rasterTileSize = 256;

    _mapStylesCollection = new MapStylesCollection();
    _mapStyle = _mapStylesCollection.getBakedStyle("default");
    if (_mapStyle == null) {
      System.err.println("Failed to resolve style 'default'");
      System.exit(0);
    }

    _obfsCollection = new ObfsCollection();
    _obfsCollection.addDirectory("osmand", false);

    _mapPresentationEnvironment = new MapPresentationEnvironment(
      _mapStyle,
      _displayDensityFactor,
      "en"); //TODO: here should be current locale
    _primitiviser = new Primitiviser(
      _mapPresentationEnvironment);
    _binaryMapDataProvider = new BinaryMapDataProvider(
      _obfsCollection);
    _binaryMapPrimitivesProvider = new BinaryMapPrimitivesProvider(
      _binaryMapDataProvider,
      _primitiviser,
      _rasterTileSize);
    _binaryMapStaticSymbolsProvider = new BinaryMapStaticSymbolsProvider(
      _binaryMapPrimitivesProvider,
      _rasterTileSize);
    _binaryMapRasterBitmapTileProvider = new BinaryMapRasterBitmapTileProvider_Software(
      _binaryMapPrimitivesProvider);

    _mapRenderer = OsmAndCore.createMapRenderer(MapRendererClass.AtlasMapRenderer_OpenGL2plus);
    if (_mapRenderer == null) {
      System.err.println("Failed to create map renderer 'AtlasMapRenderer_OpenGL2plus'");
      System.exit(0);
    }

    AtlasMapRendererConfiguration atlasRendererConfiguration = AtlasMapRendererConfiguration.Casts.upcastFrom(_mapRenderer.getConfiguration());
    atlasRendererConfiguration.setReferenceTileSizeOnScreenInPixels(_referenceTileSize);
    _mapRenderer.setConfiguration(AtlasMapRendererConfiguration.Casts.downcastTo_MapRendererConfiguration(atlasRendererConfiguration));

    _mapRenderer.setAzimuth(0.0f);
    _mapRenderer.setElevationAngle(35.0f);

    // Amsterdam via Mapnik
    _mapRenderer.setTarget(new PointI(
      1102430866,
      704978668));
    _mapRenderer.setZoom(10.0f);
    /*
    IMapRasterBitmapTileProvider mapnik = OnlineTileSources.getBuiltIn().createProviderFor("Mapnik (OsmAnd)");
    if (mapnik == null)
      Log.e(TAG, "Failed to create mapnik");
    */
    _mapRenderer.setRasterLayerProvider(RasterMapLayerId.BaseLayer, _binaryMapRasterBitmapTileProvider);
  }

  @Override
  public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
  }

  @Override
  public void display(GLAutoDrawable drawable) {
  }

  @Override
  public void dispose(GLAutoDrawable drawable) {
  }


/*


  public Gears(int swapInterval) {
    this.swapInterval = swapInterval;
  }

  public Gears() {
    this.swapInterval = 1;
  }

  public void setGears(int g1, int g2, int g3) {
      gear1 = g1;
      gear2 = g2;
      gear3 = g3;
  }


 





  public static void gear(GL2 gl,
                          float inner_radius,
                          float outer_radius,
                          float width,
                          int teeth,
                          float tooth_depth)
  {
    int i;
    float r0, r1, r2;
    float angle, da;
    float u, v, len;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0f;
    r2 = outer_radius + tooth_depth / 2.0f;

    da = 2.0f * (float) Math.PI / teeth / 4.0f;

    gl.glShadeModel(GL2.GL_FLAT);

    gl.glNormal3f(0.0f, 0.0f, 1.0f);

    gl.glBegin(GL2.GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++)
      {
        angle = i * 2.0f * (float) Math.PI / teeth;
        gl.glVertex3f(r0 * (float)Math.cos(angle), r0 * (float)Math.sin(angle), width * 0.5f);
        gl.glVertex3f(r1 * (float)Math.cos(angle), r1 * (float)Math.sin(angle), width * 0.5f);
        if(i < teeth)
          {
            gl.glVertex3f(r0 * (float)Math.cos(angle), r0 * (float)Math.sin(angle), width * 0.5f);
            gl.glVertex3f(r1 * (float)Math.cos(angle + 3.0f * da), r1 * (float)Math.sin(angle + 3.0f * da), width * 0.5f);
          }
      }
    gl.glEnd();

    gl.glBegin(GL2.GL_QUADS);
    for (i = 0; i < teeth; i++)
      {
        angle = i * 2.0f * (float) Math.PI / teeth;
        gl.glVertex3f(r1 * (float)Math.cos(angle), r1 * (float)Math.sin(angle), width * 0.5f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + da), r2 * (float)Math.sin(angle + da), width * 0.5f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + 2.0f * da), r2 * (float)Math.sin(angle + 2.0f * da), width * 0.5f);
        gl.glVertex3f(r1 * (float)Math.cos(angle + 3.0f * da), r1 * (float)Math.sin(angle + 3.0f * da), width * 0.5f);
      }
    gl.glEnd();

    gl.glBegin(GL2.GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++)
      {
        angle = i * 2.0f * (float) Math.PI / teeth;
        gl.glVertex3f(r1 * (float)Math.cos(angle), r1 * (float)Math.sin(angle), -width * 0.5f);
        gl.glVertex3f(r0 * (float)Math.cos(angle), r0 * (float)Math.sin(angle), -width * 0.5f);
        gl.glVertex3f(r1 * (float)Math.cos(angle + 3 * da), r1 * (float)Math.sin(angle + 3 * da), -width * 0.5f);
        gl.glVertex3f(r0 * (float)Math.cos(angle), r0 * (float)Math.sin(angle), -width * 0.5f);
      }
    gl.glEnd();

    gl.glBegin(GL2.GL_QUADS);
    for (i = 0; i < teeth; i++)
      {
        angle = i * 2.0f * (float) Math.PI / teeth;
        gl.glVertex3f(r1 * (float)Math.cos(angle + 3 * da), r1 * (float)Math.sin(angle + 3 * da), -width * 0.5f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + 2 * da), r2 * (float)Math.sin(angle + 2 * da), -width * 0.5f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + da), r2 * (float)Math.sin(angle + da), -width * 0.5f);
        gl.glVertex3f(r1 * (float)Math.cos(angle), r1 * (float)Math.sin(angle), -width * 0.5f);
      }
    gl.glEnd();

    gl.glBegin(GL2.GL_QUAD_STRIP);
    for (i = 0; i < teeth; i++)
      {
        angle = i * 2.0f * (float) Math.PI / teeth;
        gl.glVertex3f(r1 * (float)Math.cos(angle), r1 * (float)Math.sin(angle), width * 0.5f);
        gl.glVertex3f(r1 * (float)Math.cos(angle), r1 * (float)Math.sin(angle), -width * 0.5f);
        u = r2 * (float)Math.cos(angle + da) - r1 * (float)Math.cos(angle);
        v = r2 * (float)Math.sin(angle + da) - r1 * (float)Math.sin(angle);
        len = (float)Math.sqrt(u * u + v * v);
        u /= len;
        v /= len;
        gl.glNormal3f(v, -u, 0.0f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + da), r2 * (float)Math.sin(angle + da), width * 0.5f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + da), r2 * (float)Math.sin(angle + da), -width * 0.5f);
        gl.glNormal3f((float)Math.cos(angle), (float)Math.sin(angle), 0.0f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + 2 * da), r2 * (float)Math.sin(angle + 2 * da), width * 0.5f);
        gl.glVertex3f(r2 * (float)Math.cos(angle + 2 * da), r2 * (float)Math.sin(angle + 2 * da), -width * 0.5f);
        u = r1 * (float)Math.cos(angle + 3 * da) - r2 * (float)Math.cos(angle + 2 * da);
        v = r1 * (float)Math.sin(angle + 3 * da) - r2 * (float)Math.sin(angle + 2 * da);
        gl.glNormal3f(v, -u, 0.0f);
        gl.glVertex3f(r1 * (float)Math.cos(angle + 3 * da), r1 * (float)Math.sin(angle + 3 * da), width * 0.5f);
        gl.glVertex3f(r1 * (float)Math.cos(angle + 3 * da), r1 * (float)Math.sin(angle + 3 * da), -width * 0.5f);
        gl.glNormal3f((float)Math.cos(angle), (float)Math.sin(angle), 0.0f);
      }
    gl.glVertex3f(r1 * (float)Math.cos(0), r1 * (float)Math.sin(0), width * 0.5f);
    gl.glVertex3f(r1 * (float)Math.cos(0), r1 * (float)Math.sin(0), -width * 0.5f);
    gl.glEnd();

    gl.glShadeModel(GL2.GL_SMOOTH);

    gl.glBegin(GL2.GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++)
      {
        angle = i * 2.0f * (float) Math.PI / teeth;
        gl.glNormal3f(-(float)Math.cos(angle), -(float)Math.sin(angle), 0.0f);
        gl.glVertex3f(r0 * (float)Math.cos(angle), r0 * (float)Math.sin(angle), -width * 0.5f);
        gl.glVertex3f(r0 * (float)Math.cos(angle), r0 * (float)Math.sin(angle), width * 0.5f);
      }
    gl.glEnd();
  }

  class GearsKeyAdapter extends KeyAdapter {
    @Override
	public void keyPressed(KeyEvent e) {
        int kc = e.getKeyCode();
        if(KeyEvent.VK_LEFT == kc) {
            view_roty -= 1;
        } else if(KeyEvent.VK_RIGHT == kc) {
            view_roty += 1;
        } else if(KeyEvent.VK_UP == kc) {
            view_rotx -= 1;
        } else if(KeyEvent.VK_DOWN == kc) {
            view_rotx += 1;
        }
    }
  }

  class GearsMouseAdapter extends MouseAdapter {
      @Override
	public void mousePressed(MouseEvent e) {
        prevMouseX = e.getX();
        prevMouseY = e.getY();
      }

      @Override
	public void mouseReleased(MouseEvent e) {
      }

      @Override
	public void mouseDragged(MouseEvent e) {
        final int x = e.getX();
        final int y = e.getY();
        int width=0, height=0;
        Object source = e.getSource();
        if(source instanceof Window) {
            Window window = (Window) source;
            width=window.getSurfaceWidth();
            height=window.getSurfaceHeight();
        } else if(source instanceof GLAutoDrawable) {
        	GLAutoDrawable glad = (GLAutoDrawable) source;
            width=glad.getSurfaceWidth();
            height=glad.getSurfaceHeight();
        } else if (GLProfile.isAWTAvailable() && source instanceof java.awt.Component) {
            java.awt.Component comp = (java.awt.Component) source;
            width=comp.getWidth();
            height=comp.getHeight();
        } else {
            throw new RuntimeException("Event source neither Window nor Component: "+source);
        }
        float thetaY = 360.0f * ( (float)(x-prevMouseX)/(float)width);
        float thetaX = 360.0f * ( (float)(prevMouseY-y)/(float)height);

        prevMouseX = x;
        prevMouseY = y;

        view_rotx += thetaX;
        view_roty += thetaY;
      }
  }*/
}

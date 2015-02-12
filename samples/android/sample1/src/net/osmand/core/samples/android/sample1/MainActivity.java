package net.osmand.core.samples.android.sample1;

import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;

import net.osmand.core.jni.*;
import net.osmand.core.android.*;

public class MainActivity extends ActionBarActivity {
    private static final String TAG = "OsmAndCoreSample";

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
    private IMapLayerProvider _mapLayerProvider0;
    private IMapLayerProvider _mapLayerProvider1;
    private QIODeviceLogSink _fileLogSink;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Initialize native core prior (if needed)
        if (NativeCore.isAvailable() && !NativeCore.isLoaded())
            NativeCore.load(CoreResourcesFromAndroidAssets.loadFromCurrentApplication(this));

		Logger.get().setSeverityLevelThreshold(LogSeverityLevel.Verbose);

        // Inflate views
        setContentView(R.layout.activity_main);

        // Get map view
        _mapView = (AtlasMapRendererView) findViewById(R.id.mapRendererView);

        // Additional log sink
        _fileLogSink = QIODeviceLogSink.createFileLogSink(
                Environment.getExternalStorageDirectory() + "/osmand/osmandcore.log");
        Logger.get().addLogSink(_fileLogSink);

        // Get device display density factor
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        _displayDensityFactor = displayMetrics.densityDpi / 160.0f;
        _referenceTileSize = (int)(256 * _displayDensityFactor);
        _rasterTileSize = Integer.highestOneBit(_referenceTileSize - 1) * 2;
        Log.i(TAG, "displayDensityFactor = " + _displayDensityFactor);
        Log.i(TAG, "referenceTileSize = " + _referenceTileSize);
        Log.i(TAG, "rasterTileSize = " + _rasterTileSize);

        Log.i(TAG, "Going to resolve default embedded style...");
        _mapStylesCollection = new MapStylesCollection();
        _mapStyle = _mapStylesCollection.getResolvedStyleByName("default");
        if (_mapStyle == null)
        {
            Log.e(TAG, "Failed to resolve style 'default'");
            System.exit(0);
        }

        Log.i(TAG, "Going to prepare OBFs collection");
        _obfsCollection = new ObfsCollection();
        Log.i(TAG, "Will load OBFs from " + Environment.getExternalStorageDirectory() + "/osmand");
        _obfsCollection.addDirectory(Environment.getExternalStorageDirectory() + "/osmand", false);

        Log.i(TAG, "Going to prepare all resources for renderer");
        _mapPresentationEnvironment = new MapPresentationEnvironment(
                _mapStyle,
                _displayDensityFactor,
                "en"); //TODO: here should be current locale
        //mapPresentationEnvironment->setSettings(configuration.styleSettings);
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

        _mapView.setReferenceTileSizeOnScreenInPixels(_referenceTileSize);

        _mapView.addSymbolsProvider(_mapObjectsSymbolsProvider);
        _mapView.setAzimuth(0.0f);
        _mapView.setElevationAngle(35.0f);

        _mapView.setTarget(new PointI(
                1102430866,
                704978668));
        _mapView.setZoom(10.0f);

        _mapLayerProvider0 = new MapRasterLayerProvider_Software(_mapPrimitivesProvider);
        _mapView.setMapLayerProvider(0, _mapLayerProvider0);
    }

    private AtlasMapRendererView _mapView;

    @Override
    protected void onResume() {
        super.onResume();

        _mapView.handleOnResume();
    }

    @Override
    protected void onPause() {
        _mapView.handleOnPause();

        super.onPause();
    }

    @Override
    protected void onDestroy() {
        _mapView.handleOnDestroy();

        super.onDestroy();
    }
}

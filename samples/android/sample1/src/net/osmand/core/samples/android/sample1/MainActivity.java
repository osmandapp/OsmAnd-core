package net.osmand.core.samples.android.sample1;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.util.DisplayMetrics;
import android.util.Log;

import net.osmand.core.android.AtlasMapRendererView;
import net.osmand.core.android.CoreResourcesFromAndroidAssets;
import net.osmand.core.android.NativeCore;
import net.osmand.core.jni.AmenitiesByNameSearch;
import net.osmand.core.jni.Amenity;
import net.osmand.core.jni.AreaI;
import net.osmand.core.jni.IMapLayerProvider;
import net.osmand.core.jni.IMapStylesCollection;
import net.osmand.core.jni.IObfsCollection;
import net.osmand.core.jni.IQueryController;
import net.osmand.core.jni.ISearch;
import net.osmand.core.jni.LatLon;
import net.osmand.core.jni.LogSeverityLevel;
import net.osmand.core.jni.Logger;
import net.osmand.core.jni.MapObjectsSymbolsProvider;
import net.osmand.core.jni.MapPresentationEnvironment;
import net.osmand.core.jni.MapPrimitivesProvider;
import net.osmand.core.jni.MapPrimitiviser;
import net.osmand.core.jni.MapRasterLayerProvider_Software;
import net.osmand.core.jni.MapRendererStateChanges;
import net.osmand.core.jni.MapStylesCollection;
import net.osmand.core.jni.ObfDataType;
import net.osmand.core.jni.ObfDataTypesMask;
import net.osmand.core.jni.ObfInfo;
import net.osmand.core.jni.ObfMapObjectsProvider;
import net.osmand.core.jni.ObfsCollection;
import net.osmand.core.jni.PointI;
import net.osmand.core.jni.QIODeviceLogSink;
import net.osmand.core.jni.ResolvedMapStyle;
import net.osmand.core.jni.Utilities;
import net.osmand.core.jni.ZoomLevel;

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

	private final static int MAX_RESULTS = 10;
	private int resCount = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        long startTime = System.currentTimeMillis();

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
                1.0f,
                1.0f,
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

		Log.d(TAG, "NATIVE_INITIALIZED = " + (System.currentTimeMillis() - startTime) / 1000f);

		System.out.println("=== Start search");
		resCount = 0;

		AmenitiesByNameSearch byNameSearch = new AmenitiesByNameSearch(_obfsCollection);
		AmenitiesByNameSearch.Criteria criteria = new AmenitiesByNameSearch.Criteria();
		criteria.setName("bank");

		_obfsCollection.obtainDataInterface();

		IObfsCollection.IAcceptorFunction acceptorFunction = new IObfsCollection.IAcceptorFunction() {
			@Override
			public boolean method(ObfInfo obfInfo) {
				// Amsterdam
				PointI topLeftPoint = Utilities.convertLatLonTo31(new LatLon(52.444457, 4.757098));
				PointI bottomRightPoint = Utilities.convertLatLonTo31(new LatLon(52.322610,4.975451));
				AreaI visibleArea = new AreaI(topLeftPoint, bottomRightPoint);


				ObfDataTypesMask mask = new ObfDataTypesMask();
				mask.set(ObfDataType.POI);

				//boolean res = obfInfo.containsDataFor(visibleArea, ZoomLevel.MinZoomLevel, ZoomLevel.MaxZoomLevel, mask);
				return true;//res;
			}
		};

		criteria.setSourceFilter(acceptorFunction.getBinding());

		ISearch.INewResultEntryCallback newResultEntryCallback = new ISearch.INewResultEntryCallback() {
			@Override
			public void method(ISearch.Criteria criteria, ISearch.IResultEntry resultEntry) {
				ResultEntry res = new ResultEntry(resultEntry);
				Amenity amenity = res.getAmenity();
				System.out.println("Poi found === " + amenity.getNativeName());
				amenity.getLocalizedNames();
				resCount++;
			}
		};

		byNameSearch.performSearch(criteria, newResultEntryCallback.getBinding(), new IQueryController() {
			@Override
			public boolean isAborted() {
				return resCount >= MAX_RESULTS;
			}
		});

		System.out.println("=== Finish search");

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

	private static class ResultEntry extends AmenitiesByNameSearch.ResultEntry {
		protected ResultEntry(ISearch.IResultEntry resultEntry) {
			super(ISearch.IResultEntry.getCPtr(resultEntry), false);
		}
	}
}

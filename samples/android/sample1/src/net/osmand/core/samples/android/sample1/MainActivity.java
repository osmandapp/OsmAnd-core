package net.osmand.core.samples.android.sample1;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;

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
import net.osmand.core.jni.MapStylesCollection;
import net.osmand.core.jni.ObfInfo;
import net.osmand.core.jni.ObfMapObjectsProvider;
import net.osmand.core.jni.ObfsCollection;
import net.osmand.core.jni.PointI;
import net.osmand.core.jni.QIODeviceLogSink;
import net.osmand.core.jni.QStringList;
import net.osmand.core.jni.QStringStringHash;
import net.osmand.core.jni.ResolvedMapStyle;
import net.osmand.core.jni.Utilities;

import java.text.Format;
import java.util.Formatter;

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

	private TextView textZoom;

	private GestureDetector gestureDetector;
	private float xI;
	private float yI;
	private float zoom;

	private final static int MAX_RESULTS = 10;

	private final static float INIT_LAT = 50.450117f;
	private final static float INIT_LON = 30.524142f;
	private final static float INIT_ZOOM = 10.0f;
	private int resCount = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		gestureDetector = new GestureDetector(this, new MapViewOnGestureListener());

        long startTime = System.currentTimeMillis();

        // Initialize native core prior (if needed)
        if (NativeCore.isAvailable() && !NativeCore.isLoaded())
            NativeCore.load(CoreResourcesFromAndroidAssets.loadFromCurrentApplication(this));

		Logger.get().setSeverityLevelThreshold(LogSeverityLevel.Verbose);

        // Inflate views
        setContentView(R.layout.activity_main);

        // Get map view
        _mapView = (AtlasMapRendererView) findViewById(R.id.mapRendererView);

		textZoom = (TextView) findViewById(R.id.text_zoom);

		findViewById(R.id.map_zoom_in_button).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setZoom(zoom + 1f);
			}
		});

		findViewById(R.id.map_zoom_out_button).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setZoom(zoom - 1f);
			}
		});

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
        _mapView.setElevationAngle(90.0f);

		setTarget(Utilities.convertLatLonTo31(new LatLon(INIT_LAT, INIT_LON)));
		setZoom(INIT_ZOOM);

        _mapLayerProvider0 = new MapRasterLayerProvider_Software(_mapPrimitivesProvider);
        _mapView.setMapLayerProvider(0, _mapLayerProvider0);

		System.out.println("NATIVE_INITIALIZED = " + (System.currentTimeMillis() - startTime) / 1000f);

		//runSearch();
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

	public boolean setTarget(PointI pointI) {
		xI = pointI.getX();
		yI = pointI.getY();
		return _mapView.setTarget(pointI);
	}

	@SuppressLint("DefaultLocale")
	public boolean setZoom(float zoom) {
		this.zoom = zoom;
		textZoom.setText(String.format("%.0f", zoom));
		return _mapView.setZoom(zoom);
	}

	public boolean onTouchEvent(MotionEvent event) {
		return gestureDetector.onTouchEvent(event);
	}

	private void runSearch() {
		System.out.println("=== Start search");
		resCount = 0;

		AmenitiesByNameSearch byNameSearch = new AmenitiesByNameSearch(_obfsCollection);
		AmenitiesByNameSearch.Criteria criteria = new AmenitiesByNameSearch.Criteria();
		criteria.setName("bank");

		/*
		PointI topLeftPoint = new PointI();
		PointI bottomRightPoint = new PointI();
		_mapView.getLocationFromScreenPoint(new PointI(0, 0), topLeftPoint);
		_mapView.getLocationFromScreenPoint(new PointI(_mapView.getWidth(), _mapView.getHeight()), bottomRightPoint);
		*/

		// Kyiv
		PointI topLeftPoint = Utilities.convertLatLonTo31(new LatLon(50.498509, 30.445950));
		PointI bottomRightPoint = Utilities.convertLatLonTo31(new LatLon(50.400741, 30.577443));

		final AreaI visibleArea = new AreaI(topLeftPoint, bottomRightPoint);

		IObfsCollection.IAcceptorFunction acceptorFunction = new IObfsCollection.IAcceptorFunction() {
			@Override
			public boolean method(ObfInfo obfInfo) {

				boolean res = true;//obfInfo.containsPOIFor(visibleArea);
				return res && resCount < MAX_RESULTS;
			}
		};

		criteria.setSourceFilter(acceptorFunction.getBinding());

		ISearch.INewResultEntryCallback newResultEntryCallback = new ISearch.INewResultEntryCallback() {
			@Override
			public void method(ISearch.Criteria criteria, ISearch.IResultEntry resultEntry) {
				Amenity amenity = new ResultEntry(resultEntry).getAmenity();
				System.out.println("Poi found === " + amenity.getNativeName());
				QStringStringHash locNames = amenity.getLocalizedNames();
				if (locNames.size() > 0) {
					QStringList keys = locNames.keys();
					StringBuilder sb = new StringBuilder("=== Localized names: ");
					for (int i = 0; i < keys.size(); i++) {
						String key = keys.get(i);
						sb.append(key).append("=").append(locNames.get(key)).append(" | ");
					}
					System.out.println(sb.toString());
				}
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

	private static class ResultEntry extends AmenitiesByNameSearch.ResultEntry {
		protected ResultEntry(ISearch.IResultEntry resultEntry) {
			super(ISearch.IResultEntry.getCPtr(resultEntry), false);
		}
	}

	private class MapViewOnGestureListener extends SimpleOnGestureListener {

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
			float fromX = e2.getX() + distanceX;
			float fromY = e2.getY() + distanceY;
			float toX = e2.getX();
			float toY = e2.getY();

			float dx = (fromX - toX);
			float dy = (fromY - toY);

			PointI newTarget = new PointI();
			_mapView.getLocationFromScreenPoint(new PointI(_mapView.getWidth() / 2 + (int)dx, _mapView.getHeight() / 2 + (int)dy), newTarget);

			setTarget(newTarget);

			return true;
		}
	}
}

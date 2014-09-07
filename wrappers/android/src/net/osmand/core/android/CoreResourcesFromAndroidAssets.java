package net.osmand.core.android;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources;

import net.osmand.core.jni.*;

public class CoreResourcesFromAndroidAssets extends ICoreResourcesProvider {
    public CoreResourcesFromAndroidAssets(final Context context) {
        _context = context;
    }

    private final Context _context;

    @Override
    public SWIGTYPE_p_QByteArray getResource(String name, float displayDensityFactor, SWIGTYPE_p_bool ok) {
        return null;
    }

    @Override
    public SWIGTYPE_p_QByteArray getResource(String name, SWIGTYPE_p_bool ok) {
        return null;
    }

    @Override
    public boolean containsResource(String name, float displayDensityFactor) {
        return false;
    }

    @Override
    public boolean containsResource(String name) {
        return false;
    }
}

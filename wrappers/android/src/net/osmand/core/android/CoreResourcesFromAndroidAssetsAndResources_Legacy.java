package net.osmand.core.android;

import net.osmand.core.jni.ICoreResourcesProvider;

public class CoreResourcesFromAndroidAssetsAndResources_Legacy extends ICoreResourcesProvider {
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

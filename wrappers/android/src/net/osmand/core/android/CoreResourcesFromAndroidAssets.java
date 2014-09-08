package net.osmand.core.android;

import java.lang.String;
import java.io.IOException;
import java.util.HashMap;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.util.Log;

import net.osmand.core.jni.*;

// This class provides reverse mapping from 'embed-resources.list' to files&folders scheme used by OsmAndCore_android.aar package
public class CoreResourcesFromAndroidAssets extends ICoreResourcesProvider {
    public CoreResourcesFromAndroidAssets(final Context context) {
        _context = context;

        _resourcesInBundle = collectBundledResources();
    }

    private final Context _context;
    private final HashMap<String, ResourceEntry> _resourcesInBundle;

    private final class ResourceEntry {
    }

    private HashMap<String, ResourceEntry> collectBundledResources() {
        final AssetManager assetManager = _context.getResources().getAssets();
        final HashMap<String, ResourceEntry> resourcesCollection = new HashMap<String, ResourceEntry>();

        collectBundledResourcesFrom(resourcesCollection, assetManager, "OsmAndCore_ResourcesBundle");

        return resourcesCollection;
    }

    private void collectBundledResourcesFrom(final HashMap<String, ResourceEntry> resourcesCollection, final AssetManager assetManager, final String path) {
        String[] subdirectories;
        try {
            assetManager.list(path);
        catch(IOException e) {
            Log.e("Failed to list '" + path + "'", e);
            return;
        }

        // If listing returned nothing, it means that path refers to a file (or nothing was found)
        if (subdirectories.length == 0) {
            final String fullFilename = path;
            try {
                final AssetFileDescriptor assetFd = assetManager.openFd(fullFilename);
                Log.d("CoreResourcesFromAndroidAssets", fullFilename + " is a resource");
                assetFd.close();
            } catch(IOException e) {
            }
            return;
        }

        // Process all subdirectories recursively
        for (String subdirName : subdirectories)
            collectBundledResourcesFrom(resourcesCollection, assetManager, path + "/" + subdirName);
    }

    @Override
    public SWIGTYPE_p_QByteArray getResource(String name, float displayDensityFactor, SWIGTYPE_p_bool ok_) {
        final BoolPtr ok = BoolPtr.frompointer(ok_);
        if (ok != null)
            ok.assign(false);

        return null;
    }

    @Override
    public SWIGTYPE_p_QByteArray getResource(String name, SWIGTYPE_p_bool ok_) {
        final BoolPtr ok = BoolPtr.frompointer(ok_);
        if (ok != null)
            ok.assign(false);

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

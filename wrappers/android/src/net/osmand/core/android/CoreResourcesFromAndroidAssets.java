package net.osmand.core.android;

import java.io.FileInputStream;
import java.lang.String;
import java.lang.Float;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.util.List;
import java.util.LinkedList;
import java.util.Map;
import java.util.HashMap;
import java.util.TreeMap;
import java.util.Iterator;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ApplicationInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.Log;

import net.osmand.core.jni.*;

// This class provides reverse mapping from 'embed-resources.list' to files&folders scheme used by OsmAndCore_android.aar package
public class CoreResourcesFromAndroidAssets extends interface_ICoreResourcesProvider {
    private static final String TAG = "CoreResourcesFromAndroidAssets";

    private CoreResourcesFromAndroidAssets(final Context context) {
        _context = context;
    }

    private boolean load() {
        final AssetManager assetManager = _context.getResources().getAssets();

        PackageInfo packageInfo = null;
        try {
            packageInfo = _context.getPackageManager().getPackageInfo(_context.getPackageName(), 0);
        } catch(NameNotFoundException e) {
            Log.e(TAG, "Failed to get own package info", e);
            return false;
        }
        _bundleFilename = packageInfo.applicationInfo.sourceDir;
        Log.i(TAG, "Located own package at '" + _bundleFilename + "'");
        
        // Load the index
        final List<String> resourcesInBundle = new LinkedList<String>();
        try {
            final InputStream resourcesIndexStream = assetManager.open("OsmAndCore_ResourcesBundle.index", AssetManager.ACCESS_BUFFER);

            final BufferedReader resourcesIndexBufferedReader = new BufferedReader(new InputStreamReader(resourcesIndexStream));
            String resourceInBundle;
            while ((resourceInBundle = resourcesIndexBufferedReader.readLine()) != null)
                resourcesInBundle.add(resourceInBundle);
        } catch (IOException e) {
            Log.e(TAG, "Failed to read bundle index", e);
            return false;
        }
        Log.i(TAG, "Application contains " + resourcesInBundle.size() + " resources");

        // Parse resources index
        final Pattern resourceNameWithQualifiersRegExp = Pattern.compile("(?:\\[(.*)\\]/)(.*)");
        for (String resourceInBundle : resourcesInBundle) {
            // Process resource name
            String pureResourceName = resourceInBundle;
            String[] qualifiers = null;
            final Matcher resourceNameComponentsMatcher = resourceNameWithQualifiersRegExp.matcher(resourceInBundle);
            if (resourceNameComponentsMatcher.matches()) {
                qualifiers = resourceNameComponentsMatcher.group(1).split(";");
                pureResourceName = resourceNameComponentsMatcher.group(2);
            }

            // Get location of this resource
            long declaredSize;
            long size;
            long offset;
            try {
                final AssetFileDescriptor resourceFd = assetManager.openFd("OsmAndCore_ResourcesBundle/" + resourceInBundle + (resourceInBundle.endsWith(".png") ? "" : ".qz"));
                declaredSize = resourceFd.getDeclaredLength();
                size = resourceFd.getLength();
                offset = resourceFd.getStartOffset();
                resourceFd.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to locate '" + resourceInBundle + "'", e);
                continue;
            }
            if (declaredSize != size) {
                Log.e(TAG, "Declared size does not match size for '" + resourceInBundle + "'");
                continue;
            }
            final ResourceData resourceData = new ResourceData();
            resourceData.offset = offset;
            resourceData.size = size;

            // Get resource entry for this resource
            ResourceEntry resourceEntry = _resources.get(pureResourceName);
            if (resourceEntry == null) {
                resourceEntry = new ResourceEntry();
                _resources.put(pureResourceName, resourceEntry);
            }
            if (qualifiers == null) {
                resourceEntry.defaultVariant = resourceData;
            } else {
                for (String qualifier : qualifiers) {
                    final String[] qualifierComponents = qualifier.trim().split("=");
                    
                    if (qualifierComponents.length == 2 && qualifierComponents[0].equals("ddf")) {
                        float ddfValue;
                        try {
                            ddfValue = Float.parseFloat(qualifierComponents[1]);
                        } catch (NumberFormatException e) {
                            Log.e(TAG, "Unsupported value '" + qualifierComponents[1] + "' for DDF qualifier", e);
                            continue;
                        }

                        if (resourceEntry.variantsByDisplayDensityFactor == null)
                            resourceEntry.variantsByDisplayDensityFactor = new TreeMap<Float, ResourceData>();
                        resourceEntry.variantsByDisplayDensityFactor.put(ddfValue, resourceData);
                    } else {
                        Log.w(TAG, "Unsupported qualifier '" + qualifier.trim() + "'");
                    }
                }
            }
        }

        return true;
    }

    private final Context _context;
    private String _bundleFilename;
    private final HashMap<String, ResourceEntry> _resources = new HashMap<String, ResourceEntry>();

    private final class ResourceData {
        public long offset;
        public long size;
    }
    private final class ResourceEntry {
        public ResourceData defaultVariant;
        public TreeMap<Float, ResourceData> variantsByDisplayDensityFactor;
    }

    @Override
    public SWIGTYPE_p_QByteArray getResource(String name, float displayDensityFactor, SWIGTYPE_p_bool ok_) {
        final BoolPtr ok = BoolPtr.frompointer(ok_);

		ResourceData resourceData = getResourceData(name, displayDensityFactor);
        if (resourceData == null) {
            ResourceEntry resourceEntry = _resources.get(name);
            if (resourceEntry != null) {
                resourceData = resourceEntry.defaultVariant;
            }
        }
        if (resourceData == null) {
            Log.w(TAG, "Requested resource [ddf=" + displayDensityFactor + "]'" + name + "' was not found, as well as '" + name + "'");
            if (ok != null)
                ok.assign(false);
            return SwigUtilities.emptyQByteArray();
        }

        final SWIGTYPE_p_QByteArray data;
		if (!name.endsWith(".png")) {
			data = SwigUtilities.qDecompress(SwigUtilities.readPartOfFile(
					_bundleFilename,
					resourceData.offset,
					resourceData.size));
		} else {
			data = SwigUtilities.readPartOfFile(
					_bundleFilename,
					resourceData.offset,
					resourceData.size);
		}

        if (data == null) {
            Log.e(TAG, "Failed to load data of '" + name + "'");
            if (ok != null)
                ok.assign(false);
            return SwigUtilities.emptyQByteArray();
        }

        if (ok != null)
            ok.assign(true);
        return data;
    }

    @Override
    public SWIGTYPE_p_QByteArray getResource(String name, SWIGTYPE_p_bool ok_) {
        final BoolPtr ok = BoolPtr.frompointer(ok_);
        
        final ResourceEntry resourceEntry = _resources.get(name);
        if (resourceEntry == null) {
            Log.w(TAG, "Requested resource '" + name + "' was not found");
            if (ok != null)
                ok.assign(false);
            return SwigUtilities.emptyQByteArray();
        }

        if (resourceEntry.defaultVariant == null) {
            Log.w(TAG, "Requested resource '" + name + "' was not found");
            if (ok != null)
                ok.assign(false);
            return SwigUtilities.emptyQByteArray();
        }

        final SWIGTYPE_p_QByteArray data;
		if (!name.endsWith(".png")) {
			data = SwigUtilities.qDecompress(SwigUtilities.readPartOfFile(
					_bundleFilename,
					resourceEntry.defaultVariant.offset,
					resourceEntry.defaultVariant.size));
		} else {
			data = SwigUtilities.readPartOfFile(
					_bundleFilename,
					resourceEntry.defaultVariant.offset,
					resourceEntry.defaultVariant.size);
		}
        if (data == null) {
            Log.e(TAG, "Failed to load data of '" + name + "'");
            if (ok != null)
                ok.assign(false);
            return SwigUtilities.emptyQByteArray();
        }

        if (ok != null)
            ok.assign(true);
        return data;
    }

    @Override
    public boolean containsResource(String name, float displayDensityFactor) {
        final ResourceEntry resourceEntry = _resources.get(name);
        if (resourceEntry == null || resourceEntry.variantsByDisplayDensityFactor == null)
            return false;

        // If there's variant for any DDF, it will be used
        return true;
    }

    @Override
    public boolean containsResource(String name) {
        final ResourceEntry resourceEntry = _resources.get(name);
        if (resourceEntry == null)
            return false;

        if (resourceEntry.defaultVariant == null)
            return false;

        return true;
    }

	public ResourceData getResourceData(String name, float displayDensityFactor) {
		final ResourceEntry resourceEntry = _resources.get(name);
		if (resourceEntry == null || resourceEntry.variantsByDisplayDensityFactor == null) {
			return null;
		}

		Map.Entry<Float, ResourceData> resourceDataEntry = resourceEntry.variantsByDisplayDensityFactor.ceilingEntry(displayDensityFactor);
		if (resourceDataEntry == null)
			resourceDataEntry = resourceEntry.variantsByDisplayDensityFactor.lastEntry();
		Log.d(TAG, "Using ddf=" + resourceDataEntry.getKey() + " while looking for " + displayDensityFactor + " of '" + name + "'");
		return resourceDataEntry.getValue();
	}

	public Drawable getIcon(String name, float displayDensityFactor) {
		ResourceData resourceData = getResourceData(name, displayDensityFactor);
		if (resourceData != null) {
			try {
				byte[] array = new byte[(int) resourceData.size];
				FileInputStream fis = new FileInputStream(_bundleFilename);
				fis.skip((int) resourceData.offset);
				fis.read(array, 0, (int) resourceData.size);
				fis.close();
				Bitmap bitmap = BitmapFactory.decodeByteArray(array, 0, (int) resourceData.size);
				return new BitmapDrawable(_context.getResources(), bitmap);

			} catch (Exception e) {
				Log.e(TAG, "Failed to read icon: " + name, e);
			}
		}
		return null;
	}

    public static CoreResourcesFromAndroidAssets loadFromCurrentApplication(final Context context) {
        final CoreResourcesFromAndroidAssets bundle = new CoreResourcesFromAndroidAssets(context);

        if (!bundle.load())
            return null;

        return bundle;
    }
}

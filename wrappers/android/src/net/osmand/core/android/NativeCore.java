package net.osmand.core.android;

import android.util.Log;

import net.osmand.core.BuildConfig;
import net.osmand.core.jni.ICoreResourcesProvider;
import net.osmand.core.jni.OsmAndCore;
import net.osmand.core.jni.interface_ICoreResourcesProvider;

/**
 * Created by Alexey Pelykh on 25.11.2014.
 */
public class NativeCore {
    private static final String TAG = "OsmAndCore:Android/NativeCore";

    /**
     * Safely load native library
     *
     * @param libraryName Name of the library without "lib" prefix and ".so" extension
     *                    (if applicable).
     * @return True if library was successfully loaded, false otherwise
     */
    private static boolean loadNativeLibrary(String libraryName) {
        try {
            System.loadLibrary(libraryName);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load '" + libraryName + "'", e);
            return false;
        }

        return true;
    }

    /**
     * Flag that indicates that native libraries were loaded successfully
     */
    private static final boolean s_loadedNativeLibraries;

    /**
     * Static block to perform loading of native libraries
     */
    static {
        s_loadedNativeLibraries = loadNativeLibrary("c++_shared") &&
                loadNativeLibrary("Qt5Core") &&
                loadNativeLibrary("Qt5Network") &&
                loadNativeLibrary("Qt5Sql") &&
                loadNativeLibrary("OsmAndCoreWithJNI");
    }

    /**
     * Check if native core is available
     * @return True if native core is available, false otherwise
     */
    public static final boolean isAvailable() {
        return s_loadedNativeLibraries;
    }

    /**
     * Synchronization object for native core initialization
     */
    private static final Object s_initSync = new Object();

    /**
     * Flag to indicate if native core successfully loaded
     */
    private static boolean s_isLoaded = false;

    /**
     * Bitness of loaded native core
     */
    private static int s_bitness = 0;

    /**
     * Method to check if native core was successfully loaded
     *
     * @return True if native core was successfully loaded, false otherwise
     */
    public static boolean isLoaded() {
        if (!s_loadedNativeLibraries)
            return false;

        synchronized (s_initSync) {
            return s_isLoaded;
        }
    }

    /**
     * Method to check if native core is 64-bit
     *
     * @return True if native core is native 64 bit code, false otherwise
     */
    public static boolean is64Bit() {
        if (!s_loadedNativeLibraries)
            return false;

        synchronized (s_initSync) {
            return s_bitness == 64;
        }
    }

    /**
     * Load native core using specified core resources provider, implemented in Java
     *
     * @param coreResourcesProvider Core resources provider, implemented in Java
     * @return True if native core was successfully loaded, false otherwise
     */
    public static boolean load(interface_ICoreResourcesProvider coreResourcesProvider, String appFontsPath) {
        if (!s_loadedNativeLibraries)
            return false;

        synchronized (s_initSync) {
            if (!load(coreResourcesProvider.instantiateProxy(true), appFontsPath))
                return false;

            // Since core resources provider (created in Java), was passed via SWIG,
            // release ownership on it.
            coreResourcesProvider.swigReleaseOwnership();

            return true;
        }
    }

    /**
     * Load native core using specified core resources provider
     *
     * @param coreResourcesProvider Core resources provider
     * @param appFontsPath Path to fonts directory
     * @return True if native core was successfully loaded, false otherwise
     */
    public static boolean load(ICoreResourcesProvider coreResourcesProvider, String appFontsPath) {
        if (!s_loadedNativeLibraries)
            return false;

        synchronized (s_initSync) {
            if (s_isLoaded) {
                Log.w(TAG, "Native core was already loaded, subsequent calls are explicit");
                return true;
            }

            // Initialize core
            s_bitness = OsmAndCore.InitializeCore(coreResourcesProvider, appFontsPath);
            s_isLoaded = s_bitness > 0;
            if (!s_isLoaded)
                return false;

            // Post-initialize core

            return true;
        }
    }

    public static boolean isPerformanceLogsEnabled() {
        if (!s_loadedNativeLibraries)
            return false;

        synchronized (s_initSync) {
            return OsmAndCore.isPerformanceMetricsEnabled();
        }
    }

    public static void enablePerformanceLogs(boolean enable) {
        if (!s_loadedNativeLibraries)
            return;

        synchronized (s_initSync) {
            OsmAndCore.enablePerformanceMetrics(enable);
        }
    }

    public static String getLastPerformanceMetricsResult() {
        if (!s_loadedNativeLibraries)
            return "";

        synchronized (s_initSync) {
            return OsmAndCore.getLastPerformanceMetricsResult();
        }
    }

    /**
     * This exception means that loadNativeCore() was never called, or never succeeded.
     */
    public static class NativeCoreNotLoadedException extends RuntimeException {
        public NativeCoreNotLoadedException() {
            super("MapRendererView.loadNativeCore() was never called, or never succeeded.");
        }
    }

    /**
     * Method to ensure that native core was loaded. If not, throw exception.
     */
    /*internal*/ final static void checkIfLoaded() {
        if (isLoaded())
            return;

        throw new NativeCoreNotLoadedException();
    }
}

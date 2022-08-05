package net.osmand.core.android;

import android.content.Context;
import android.util.AttributeSet;

import net.osmand.core.jni.AtlasMapRendererConfiguration;
import net.osmand.core.jni.IMapRenderer;
import net.osmand.core.jni.MapRendererClass;
import net.osmand.core.jni.OsmAndCore;

/**
 * Atlas Map
 * <p/>
 * Created by Alexey Pelykh on 25.11.2014.
 */
public class AtlasMapRendererView extends MapRendererView {
    public AtlasMapRendererView(Context context) {
        this(context, null);
    }

    public AtlasMapRendererView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public AtlasMapRendererView(Context context, AttributeSet attrs, int defaultStyle) {
        super(context, attrs, defaultStyle);
    }

    /**
     * Creates instance of 'AtlasMapRenderer_OpenGLES2'-type map renderer
     *
     * @return Reference to OsmAndCore::IMapRenderer instance
     */
    @Override
    protected IMapRenderer createMapRendererInstance() {
        return OsmAndCore.createMapRenderer(MapRendererClass.AtlasMapRenderer_OpenGLES2plus);
    }

    /**
     * Get current configuration of atlas map renderer
     * @return AtlasMapRendererConfiguration instance
     */
    public final AtlasMapRendererConfiguration getAtlasConfiguration() {
        return AtlasMapRendererConfiguration.Casts.upcastFrom(_mapRenderer.getConfiguration());
    }

    /**
     * Set current configuration of atlas map renderer
     */
    public final void setAtlasConfiguration(AtlasMapRendererConfiguration configuration) {
        _mapRenderer.setConfiguration(
                AtlasMapRendererConfiguration.Casts.downcastTo_MapRendererConfiguration(
                        configuration));
    }

    /**
     * Set reference tile size on screen in pixels
     * @param value Reference tile size on screen in pixels
     */
    public final void setReferenceTileSizeOnScreenInPixels(float value) {
        AtlasMapRendererConfiguration configuration = getAtlasConfiguration();
        configuration.setReferenceTileSizeOnScreenInPixels(value);
        setAtlasConfiguration(configuration);
    }

//    /**
//     * Obtain number of visible tiles
//     * @return Number of visible tiles
//     */
//    public final int getVisibleTilesCount() {
//        return
//    }
//    virtual unsigned int getVisibleTilesCount() const = 0;
//
//    virtual float getTileSizeOnScreenInPixels() const = 0;
}

#ifndef MAPTILESOURCE_H
#define MAPTILESOURCE_H

#include "OsmAndCore.h"
#include <QString>

class OSMAND_CORE_API MapTileSource
{
public:
    virtual int getMaximumZoomSupported() = 0;

    virtual QString getName() = 0;

    virtual int getTileSize() = 0;

    virtual QString getUrlToLoad(int x, int y, int zoom)=0;

    virtual int getMinimumZoomSupported()= 0;

    virtual QString getTileFormat() = 0;

    virtual int getBitDensity() {return 8;}

    virtual bool isEllipticYTile() {return false;}

    virtual bool couldBeDownloadedFromInternet() = 0;
};

//public static class TileSourceTemplate implements ITileSource {
//		private int maxZoom;
//		private int minZoom;
//		private String name;
//		protected int tileSize;
//		protected String urlToLoad;
//		protected String ext;
//		private int avgSize;
//		private int bitDensity;
//		private boolean ellipticYTile;
//		private String rule;
//		private boolean isRuleAcceptable = true;

//		public TileSourceTemplate(String name, String urlToLoad, String ext, int maxZoom, int minZoom, int tileSize, int bitDensity,
//				int avgSize) {
//			this.maxZoom = maxZoom;
//			this.minZoom = minZoom;
//			this.name = name;
//			this.tileSize = tileSize;
//			this.urlToLoad = urlToLoad;
//			this.ext = ext;
//			this.avgSize = avgSize;
//			this.bitDensity = bitDensity;
//		}

//		public void setEllipticYTile(boolean ellipticYTile) {
//			this.ellipticYTile = ellipticYTile;
//		}

//		@Override
//		public boolean isEllipticYTile() {
//			return ellipticYTile;
//		}

//		@Override
//		public int getBitDensity() {
//			return bitDensity;
//		}

//		public int getAverageSize() {
//			return avgSize;
//		}

//		@Override
//		public int getMaximumZoomSupported() {
//			return maxZoom;
//		}

//		@Override
//		public int getMinimumZoomSupported() {
//			return minZoom;
//		}

//		@Override
//		public String getName() {
//			return name;
//		}

//		@Override
//		public int getTileSize() {
//			return tileSize;
//		}

//		@Override
//		public String getTileFormat() {
//			return ext;
//		}

//		public boolean isRuleAcceptable() {
//			return isRuleAcceptable;
//		}

//		public void setRuleAcceptable(boolean isRuleAcceptable) {
//			this.isRuleAcceptable = isRuleAcceptable;
//		}

//		@Override
//		public String getUrlToLoad(int x, int y, int zoom) {
//			// use int to string not format numbers! (non-nls)
//			if (urlToLoad == null) {
//				return null;
//			}
//			return MessageFormat.format(urlToLoad, zoom + "", x + "", y + ""); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
//		}

//		public String getUrlTemplate() {
//			return urlToLoad;
//		}

//		@Override
//		public boolean couldBeDownloadedFromInternet() {
//			return urlToLoad != null;
//		}


//		@Override
//		public int hashCode() {
//			final int prime = 31;
//			int result = 1;
//			result = prime * result + ((name == null) ? 0 : name.hashCode());
//			return result;
//		}

//		@Override
//		public boolean equals(Object obj) {
//			if (this == obj)
//				return true;
//			if (obj == null)
//				return false;
//			if (getClass() != obj.getClass())
//				return false;
//			TileSourceTemplate other = (TileSourceTemplate) obj;
//			if (name == null) {
//				if (other.name != null)
//					return false;
//			} else if (!name.equals(other.name))
//				return false;
//			return true;
//		}

//		public void setRule(String rule) {
//			this.rule = rule;
//		}

//		public String getRule() {
//			return rule;
//		}
//	}

#endif // MAPTILESOURCE_H

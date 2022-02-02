#include "GlobalMercator.h"

/*
 TMS Global Mercator Profile
 ---------------------------

 Functions necessary for generation of tiles in Spherical Mercator projection,
 EPSG:3857.

 Such tiles are compatible with Google Maps, Bing Maps, Yahoo Maps,
 UK Ordnance Survey OpenSpace API, ...
 and you can overlay them on top of base maps of those web mapping applications.

 Pixel and tile coordinates are in TMS notation (origin [0,0] in bottom-left).

 What coordinate conversions do we need for TMS Global Mercator tiles::

      LatLon      <->       Meters      <->     Pixels    <->       Tile

  WGS84 coordinates   Spherical Mercator  Pixels in pyramid  Tiles in pyramid
      lat/lon            XY in meters     XY pixels Z zoom      XYZ from TMS
     EPSG:4326           EPSG:387
      .----.              ---------               --                TMS
     /      \     <->     |       |     <->     /----/    <->      Google
     \      /             |       |           /--------/          QuadTree
      -----               ---------         /------------/
    KML, public         WebMapService         Web Clients      TileMapService

 What is the coordinate extent of Earth in EPSG:3857?

   [-20037508.342789244, -20037508.342789244, 20037508.342789244, 20037508.342789244]
   Constant 20037508.342789244 comes from the circumference of the Earth in meters,
   which is 40 thousand kilometers, the coordinate origin is in the middle of extent.
   In fact you can calculate the constant as: 2 * math.pi * 6378137 / 2.0
   $ echo 180 85 | gdaltransform -s_srs EPSG:4326 -t_srs EPSG:3857
   Polar areas with abs(latitude) bigger then 85.05112878 are clipped off.

 What are zoom level constants (pixels/meter) for pyramid with EPSG:3857?

   whole region is on top of pyramid (zoom=0) covered by 256x256 pixels tile,
   every lower zoom level resolution is always divided by two
   initialResolution = 20037508.342789244 * 2 / 256 = 156543.03392804062

 What is the difference between TMS and Google Maps/QuadTree tile name convention?

   The tile raster itself is the same (equal extent, projection, pixel size),
   there is just different identification of the same raster tile.
   Tiles in TMS are counted from [0,0] in the bottom-left corner, id is XYZ.
   Google placed the origin [0,0] to the top-left corner, reference is XYZ.
   Microsoft is referencing tiles by a QuadTree name, defined on the website:
   http://msdn2.microsoft.com/en-us/library/bb259689.aspx

 The lat/lon coordinates are using WGS84 datum, yes?

   Yes, all lat/lon we are mentioning should use WGS84 Geodetic Datum.
   Well, the web clients like Google Maps are projecting those coordinates by
   Spherical Mercator, so in fact lat/lon coordinates on sphere are treated as if
   the were on the WGS84 ellipsoid.

   From MSDN documentation:
   To simplify the calculations, we use the spherical form of projection, not
   the ellipsoidal form. Since the projection is used only for map display,
   and not for displaying numeric coordinates, we don't need the extra precision
   of an ellipsoidal projection. The spherical projection causes approximately
   0.33 percent scale distortion in the Y direction, which is not visually
   noticeable.

 How do I create a raster in EPSG:3857 and convert coordinates with PROJ.4?

   You can use standard GIS tools like gdalwarp, cs2cs or gdaltransform.
   All of the tools supports -t_srs 'epsg:3857'.

   For other GIS programs check the exact definition of the projection:
   More info at http://spatialreference.org/ref/user/google-projection/
   The same projection is designated as EPSG:3857. WKT definition is in the
   official EPSG database.

   Proj4 Text:
     +proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0
     +k=1.0 +units=m +nadgrids=@null +no_defs

   Human readable WKT format of EPSG:3857:
      PROJCS["Google Maps Global Mercator",
          GEOGCS["WGS 84",
              DATUM["WGS_1984",
                  SPHEROID["WGS 84",6378137,298.257223563,
                      AUTHORITY["EPSG","7030"]],
                  AUTHORITY["EPSG","6326"]],
              PRIMEM["Greenwich",0],
              UNIT["degree",0.0174532925199433],
              AUTHORITY["EPSG","4326"]],
          PROJECTION["Mercator_1SP"],
          PARAMETER["central_meridian",0],
          PARAMETER["scale_factor",1],
          PARAMETER["false_easting",0],
          PARAMETER["false_northing",0],
          UNIT["metre",1,
              AUTHORITY["EPSG","9001"]]]
 */

#include "Utilities.h"

static const int kMaxZoomLevel = 32;
static const double kOriginShift = 2.0 * M_PI * 6378137 / 2.0;

OsmAnd::GlobalMercator::GlobalMercator(
    const uint32_t tileSize_ /*= 256*/)
    : tileSize(tileSize_)
    , initialResolution(2.0 * M_PI * 6378137 / tileSize_)
{
}

OsmAnd::GlobalMercator::~GlobalMercator()
{
}

// Resolution (meters/pixel) for given zoom level (measured at Equator)
double OsmAnd::GlobalMercator::resolution(const OsmAnd::ZoomLevel& zoom) const
{
    return initialResolution / Utilities::getPowZoom(zoom);
}

// Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:3857
OsmAnd::PointD OsmAnd::GlobalMercator::latLonToMeters(const OsmAnd::LatLon& latLon) const
{
    PointD res;
    res.x = latLon.longitude * kOriginShift / 180.0;
    res.y = log(tan((90.0 + latLon.latitude) * M_PI / 360.0)) / (M_PI / 180.0);
    res.y = res.y * kOriginShift / 180.0;
    return res;
}

// Converts XY point from Spherical Mercator EPSG:3857 to lat/lon in WGS84 Datum
OsmAnd::LatLon OsmAnd::GlobalMercator::metersToLatLon(double mx, double my) const
{
    double lon = (mx / kOriginShift) * 180.0;
    double lat = (my / kOriginShift) * 180.0;

    lat = 180.0 / M_PI * (2 * atan(exp(lat * M_PI / 180.0)) - M_PI_2);
    return LatLon(lat, lon);
}

// Converts pixel coordinates in given zoom level of pyramid to EPSG:3857
OsmAnd::PointD OsmAnd::GlobalMercator::pixelsToMeters(double px, double py, const OsmAnd::ZoomLevel& zoom) const
{
    double res = resolution(zoom);
    double mx = px * res - kOriginShift;
    double my = py * res - kOriginShift;
    return PointD(mx, my);
}

// Converts EPSG:3857 to pyramid pixel coordinates in given zoom level
OsmAnd::PointD OsmAnd::GlobalMercator::metersToPixels(double mx, double my, const OsmAnd::ZoomLevel& zoom) const
{
    double res = resolution(zoom);
    double px = (mx + kOriginShift) / res;
    double py = (my + kOriginShift) / res;
    return PointD(px, py);
}

// Returns a tile covering region in given pixel coordinates
OsmAnd::PointI OsmAnd::GlobalMercator::pixelsToTile(double px, double py) const
{
    int tx = int(ceil(px / double(tileSize)) - 1);
    int ty = int(ceil(py / double(tileSize)) - 1);
    return PointI(tx, ty);
}

// Move the origin of pixel coordinates to top-left corner
OsmAnd::PointI OsmAnd::GlobalMercator::pixelsToRaster(int px, int py, const OsmAnd::ZoomLevel& zoom) const
{
    int mapSize = tileSize << zoom;
    return PointI(px, mapSize - py);
}

// Returns tile for given mercator coordinates
OsmAnd::PointI OsmAnd::GlobalMercator::metersToTile(double mx, double my, const OsmAnd::ZoomLevel& zoom) const
{
    auto p = metersToPixels(mx, my, zoom);
    return pixelsToTile(p.x, p.y);
}

// Returns bounds of the given tile in EPSG:3857 coordinates
OsmAnd::AreaD OsmAnd::GlobalMercator::tileBounds(double tx, double ty, const OsmAnd::ZoomLevel& zoom) const
{
    auto min = pixelsToMeters(tx * tileSize, ty * tileSize, zoom);
    auto max = pixelsToMeters((tx + 1) * tileSize, (ty + 1) * tileSize, zoom);
    return AreaD(min.y, min.x, max.y, max.x);
}

// Returns bounds of the given tile in latitude/longitude using WGS84 datum
OsmAnd::AreaD OsmAnd::GlobalMercator::tileLatLonBounds(double tx, double ty, const OsmAnd::ZoomLevel& zoom) const
{
    auto bounds = tileBounds(tx, ty, zoom);
    auto minLatLon = metersToLatLon(bounds.top(), bounds.left());
    auto maxLatLon = metersToLatLon(bounds.bottom(), bounds.right());

    return AreaD(minLatLon.latitude, minLatLon.longitude, maxLatLon.latitude, maxLatLon.longitude);
}

// Maximal scaledown zoom of the pyramid closest to the pixelSize.
int OsmAnd::GlobalMercator::zoomForPixelSize(double pixelSize) const
{
    for (int i = 0; i < kMaxZoomLevel; i++)
        if (pixelSize > resolution(static_cast<ZoomLevel>(i)))
            return fmax(0, i - 1); // We don't want to scale up

    return kMaxZoomLevel - 1;
}

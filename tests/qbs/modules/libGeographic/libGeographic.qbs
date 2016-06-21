import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "/mnt/data_ssd/osmand/"
    property string root: OsmAnd_root + "/core/externals/geographiclib/"
    cpp.includePaths: root + "/upstream.original/include/"
}

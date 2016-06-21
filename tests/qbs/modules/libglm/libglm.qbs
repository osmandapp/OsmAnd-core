import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "/mnt/data_ssd/osmand/"  // FIXME
    property string root: OsmAnd_root + "/core/externals/glm/"
    cpp.includePaths: root + "/upstream.patched/"
}

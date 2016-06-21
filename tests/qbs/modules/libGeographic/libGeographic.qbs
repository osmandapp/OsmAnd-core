import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "../../../"
    property string root: OsmAnd_root + "/core/externals/geographiclib/"
    cpp.includePaths: root + "/upstream.original/include/"
}

import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "../../../"
    property string root: OsmAnd_root + "/core/externals/glm/"
    cpp.includePaths: root + "/upstream.patched/"
}

import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "../../../"
    property string root: OsmAnd_root + "/core/externals/skia/"
    property string includeRoot: root + "/upstream.patched/include/"
    cpp.includePaths: [
        includeRoot,
        includeRoot + "/animator/",
        includeRoot + "/c/",
        includeRoot + "/config/",
        includeRoot + "/core/",
        includeRoot + "/device/",
        includeRoot + "/effects/",
        includeRoot + "/gpu/",
        includeRoot + "/images/",
        includeRoot + "/pathops/",
        includeRoot + "/pdf/",
        includeRoot + "/pipe/",
        includeRoot + "/ports/",
        includeRoot + "/svg/",
        includeRoot + "/utils/",
        includeRoot + "/views/",
        includeRoot + "/xml/",
    ]
}

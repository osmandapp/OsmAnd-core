import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "../../../"
    cpp.dynamicLibraries: "OsmAndCore_shared_standalone"
    cpp.includePaths: OsmAnd_root + "/core/include/"
    cpp.libraryPaths: OsmAnd_root + "/binaries/linux/gcc-amd64/Debug/"  // FIXME
}

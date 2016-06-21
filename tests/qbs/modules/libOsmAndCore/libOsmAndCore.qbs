import qbs

Module {
    Depends { name: "cpp" }
    property string OsmAnd_root: "/mnt/data_ssd/osmand/"  // FIXME
    cpp.dynamicLibraries: "OsmAndCoreWithJNI_standalone"
    cpp.includePaths: OsmAnd_root + "/core/include/"
    cpp.libraryPaths: OsmAnd_root + "/binaries/linux/gcc-amd64/Debug/"  // FIXME
}

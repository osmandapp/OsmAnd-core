import qbs

Project {
    name: "Tests"
    references: [
        "unit/TestAddressSearch.qbs",
        "unit/TestCoordinateSearch.qbs",
        "unit/TestGeoTiffRasterWindow.qbs"
	]
    qbsSearchPaths: "qbs"
    AutotestRunner { }
}

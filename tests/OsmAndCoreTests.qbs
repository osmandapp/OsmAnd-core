import qbs

Project {
    name: "Tests"
    references: [
        "unit/TestAddressSearch.qbs",
        "unit/TestCoordinateSearch.qbs",
        "unit/TestPoiNameIndexBBox.qbs"
	]
    qbsSearchPaths: "qbs"
    AutotestRunner { }
}

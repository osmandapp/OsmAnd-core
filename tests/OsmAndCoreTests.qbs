import qbs

Project {
    name: "Tests"
    references: [
        "unit/TestAddressSearch.qbs",
        "unit/TestCoordinateSearch.qbs",
        "unit/TestVectorLineZoom.qbs"
	]
    qbsSearchPaths: "qbs"
    AutotestRunner { }
}

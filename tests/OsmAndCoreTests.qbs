import qbs

Project {
    name: "Tests"
    references: [
        "unit/TestAddressSearch.qbs",
        "unit/TestAddressSearchNew.qbs",
        "unit/TestCoordinateSearch.qbs"
	]
    qbsSearchPaths: "qbs"
    AutotestRunner { }
}

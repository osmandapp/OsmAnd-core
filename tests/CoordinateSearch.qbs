import qbs

Project {
    name: "Tests"
    references: ["unit/unit.qbs"]
    qbsSearchPaths: "qbs"
    AutotestRunner { }
}

import qbs

CppApplication {
    name: "TestSearchByCoordinate"
    type: ["application", "autotest"]
    files: ["TestCoordinateSearch.cpp"]

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.test" }

    cpp.cxxLanguageVersion: "c++11"
}

import qbs

CppApplication {
    name: "TestCoordinateSearch"
    type: ["application", "autotest"]
    files: ["TestCoordinateSearch.cpp"]

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.test" }
    Depends { name: "libOsmAndCore" }
    Depends { name: "libGeographic" }
    Depends { name: "libskia" }
    Depends { name: "libglm" }

    cpp.cxxLanguageVersion: "c++11"
}

import qbs

CppApplication {
    type: ["application", "autotest"]

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.test" }
    Depends { name: "libOsmAndCore" }
    Depends { name: "libGeographic" }
    Depends { name: "libskia" }
    Depends { name: "libglm" }

    cpp.cxxLanguageVersion: "c++11"
}

import qbs

CppApplication {
	name: "TestSearchByCoordinate"
	type: ["application", "autotest"]
    files: ["TestSearchByCoordinate.cpp"]

	Depends { name: "cpp" }
	Depends { name: "Qt.core" }
	Depends { name: "Qt.test" }

	cpp.cxxLanguageVersion: "c++11"
}

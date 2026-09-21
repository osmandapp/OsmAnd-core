import qbs
import "UnitTest.qbs" as UnitTest

UnitTest {
    name: "TestVectorLineZoom"
    cpp.cxxLanguageVersion: "c++17"
    files: ["TestVectorLineZoom.cpp"]
}

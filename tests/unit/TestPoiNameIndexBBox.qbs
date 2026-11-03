import qbs
import "UnitTest.qbs" as UnitTest

// Search by name with a bbox: name index data box with its corner outside the bbox

UnitTest {
    name: "TestPoiNameIndexBBox"
    files: ["TestPoiNameIndexBBox.cpp"]
}

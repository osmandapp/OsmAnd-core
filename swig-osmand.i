%module CoreOsmAnd
// # swig -c++ -java -package net.osmand.bridge -outdir OsmAnd-java/src/net/osmand/bridge -o src/java_core_wrap.cpp swig-osmand.i; 
%include "typemaps.i"
%include "std_string.i"
%include "std_vector.i"
namespace std {
   %template(IntVector) vector<int>;
   %template(DoubleVector) vector<double>;
   %template(StringVector) vector<string>;
   %template(ConstCharVector) vector<const char*>;
}

%{
#include "../src/Inspector.cpp"
%}


class ObfInspector {
public:
static int ObfInspector::inspector(std::vector<std::string> argv) ;
 };


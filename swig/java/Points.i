/*
 * Return borrowed PointI and PointF results as owned Java snapshots. Qt containers
 * can relocate their elements, and a borrowed SWIG proxy does not keep the
 * container (or an object containing a point member) alive.
 *
 * Include before PointsAndAreas.h and any point container instantiations.
 * Input typemaps are unchanged, including mutable reference/output arguments.
 * To update a container or parent object, use its setter with the modified copy.
 */

%define OSMAND_JAVA_POINT_VALUE(TYPE)
// References cover container getters; pointers also cover member getters and
// Nullable::getValuePtrOrNullptr(). Constructors and %newobject results already
// transfer ownership, so keep their allocation instead of copying and leaking it.
%typemap(out) TYPE &, const TYPE &, TYPE *, const TYPE * {
    $result = reinterpret_cast<jlong>($1 && !$owner ? new TYPE(*$1) : $1);
}

%typemap(javaout) TYPE &, const TYPE &, TYPE *, const TYPE * {
    long cPtr = $jnicall;
    return (cPtr == 0) ? null : new $javaclassname(cPtr, true);
  }
%enddef

// Use the underlying template types so typedefs and container const_reference
// aliases resolve to the same typemaps. By-value returns already own a copy.
OSMAND_JAVA_POINT_VALUE(OsmAnd::Point<int32_t>)
OSMAND_JAVA_POINT_VALUE(OsmAnd::Point<float>)

#undef OSMAND_JAVA_POINT_VALUE

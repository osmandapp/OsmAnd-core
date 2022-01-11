#include "ExplicitReferences.h"

#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"

void OsmAnd::InflateExplicitReferences()
{
    struct ExplicitReferences
    {
        ExplicitReferences()
        {
        }
    };

    static ExplicitReferences _explicitReferences;
}

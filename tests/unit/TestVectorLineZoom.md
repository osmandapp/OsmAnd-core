# VectorLine zoom regression test

`TestVectorLineZoom.cpp` exercises the public Core API with real `VectorLineBuilder`,
`VectorLine`, `SymbolsGroup` and generated vertex buffers. It does not extract
production source, mock Qt locks, replace MapState, or access private fields.

The executable is registered in `tests/OsmAndCoreTests.qbs` as an autotest product,
using the same `UnitTest.qbs` dependencies as the existing Core tests.

## Running

Use an existing Qbs profile configured for the Core test suite, with matching Qt,
Skia and `OsmAndCore_shared_standalone` libraries built from the current checkout.
From the Core repository, build and run only this product:

```sh
qbs build -f tests/OsmAndCoreTests.qbs -p TestVectorLineZoom
qbs run -f tests/OsmAndCoreTests.qbs -p TestVectorLineZoom
```

Select the configured profile with `profile:<name>` if it is not the default.
The shared-library search path must include the matching Core and Qt libraries.
A nonzero exit status indicates a failed assertion. The executable prints the
failing condition; a successful run prints `PASS: native VectorLine zoom regression tests`.

## Coverage

- Approach the same final scale from opposite directions at magnifier 50%, 100%
  and 200%; compare the generated X/Z vertices and mesh origin with a line created
  directly at the final scale.
- Verify that an exact 0.25 visual-zoom difference remains deferred, continuous
  smaller changes retain the original mesh, and a stable final update rebuilds it.
- Repeat every intermediate value three times over a sequence longer than the
  settling delay; verify that no subthreshold motion rebuild occurs.
- Alternate small float perturbations for two seconds; verify exactly one final
  rebuild, matching actual final geometry, and no pending work afterward.
- Verify surface-only and magnifier-only changes and immediate above-threshold updates.
- Feed NaN, positive/negative infinity, zero or invalid zoom levels after pending
  work has been armed. Verify that the mesh is retained, pending work is cleared,
  and subsequent valid input recovers to the same geometry as a newly created line.
- Verify that 100 stable updates neither replace the mesh nor report pending work.
- Verify that pending zoom work is not classified as a property change.

The original threshold-only implementation fails the residual-update expectations.
The initial PR implementation fails the repeated-intermediate-state check.
Earlier test runs also demonstrated its invalid-input pending-work failure.
These baseline failures must be verified by executing against the corresponding
library revisions, not by modifying or duplicating the algorithm inside the test.

This test does not create a GPU renderer and does not measure frame time, battery
usage, actual renderer idle, or concurrent resource creation. Those need separate
renderer/device verification. Passing `updatesPresent()` checks alone is not proof
that the full renderer reaches idle.

## iOS simulator runner

When existing ARM64 simulator Core and dependency archives are available in the
usual `baked/` and `binaries/` workspace layout, Qbs is not required:

```sh
python3 tests/run_vector_line_zoom_ios.py --device <booted-simulator-uuid>
python3 tests/run_vector_line_zoom_ios.py --device <booted-simulator-uuid> --revision 9aa423c3
python3 tests/run_vector_line_zoom_ios.py --device <booted-simulator-uuid> --revision ee8cbbc6
```

The first command should pass; the two baseline commands should exit nonzero.
The runner always compiles the same test from the current checkout. It compiles
real line implementation translation units and the geometry helper, then links
them with the existing simulator Core and dependency archives. It also recompiles
`VectorLineBuilder_P.cpp`, whose access to private line state depends on its layout.
No production methods or dependencies are replaced by test doubles.

`--revision` reads those implementation files from Git into the output directory;
it does not switch branches or modify the checkout. This compares the line
implementation revisions using the same dependency archives, not complete builds
of historical applications. Those archives and response-file include paths must
be compatible with the checkout. No full application build is performed.

Use `--output <directory>` to retain build and runtime logs at a chosen location.
Without it, the runner creates a temporary output directory and prints its path.

## Verified results

Executed on an iPad Pro 13-inch (M5) simulator, iOS 26.5:

| Line implementation | Result |
| --- | --- |
| Working tree with validity checks, named zoom state and delayed settling | Passed all native scenarios |
| Initial PR, `9aa423c3` | Failed: `short repeats do not settle zoom` |
| Before the original fix, `ee8cbbc6` | Failed: `residual reported to renderer` |

The test viewport uses shifted coordinates centered on zero for
`visibleBBoxShifted`, separately from the ordinary 31-bit `visibleBBox31`.

## Settling policy

The original 0.25 threshold and integer zoom-level changes still apply during
motion. A remaining difference is applied after 250 ms without a change larger
than 0.0001 in the floating zoom components. The stability anchor is retained
across smaller changes so tiny oscillations cannot reset the timer forever.
Cumulative movement beyond the tolerance restarts the interval.
The final rebuild uses the actual current MapState. Differences within that
small tolerance do not rearm pending work after the rebuild.

Elapsed time uses a monotonic clock. Tests poll the public update API with a
bounded two-second deadline rather than assuming a rebuild on an exact frame.
A pause longer than the settling delay during a gesture can legitimately cause
a rebuild. The implementation does not have gesture/animation lifecycle signals.

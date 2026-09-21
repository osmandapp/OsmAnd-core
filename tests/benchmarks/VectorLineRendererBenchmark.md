# VectorLine offscreen renderer benchmark

Run from the Core repository with compatible existing ARM64 simulator Core and
Qt/dependency archives, a booted simulator and the sibling `resources` repository:

```sh
python3 tests/run_vector_line_zoom_ios.py --device <uuid> --benchmark --points 10000 --lines 1 --repeat 3 --angle 90 --output /tmp/zoom-current
python3 tests/run_vector_line_zoom_ios.py --device <uuid> --benchmark --points 10000 --lines 1 --repeat 3 --angle 90 --revision ee8cbbc6 --output /tmp/zoom-before
```

Run variants sequentially, not concurrently. `--revision` selects the line and
geometry implementation, while both variants reuse the same dependency archives.
The runner compiles the real implementation translation units and this benchmark;
it does not replace renderer dependencies with mocks or rebuild the application.
Build and runtime logs are retained in the selected output directory.

## Scene and input

- Real Core OpenGL ES 3 renderer and a 512 by 512 offscreen framebuffer.
- Real VectorLinesCollection and generated line geometry; all lines must be
  registered as renderer symbols before timing starts.
- Deterministic synthetic tracks: X spans 80,000 internal map units; Y follows
  `8000 * sin(index * 0.07)`, offset by 1,000 units between lines. Width is 64,
  with the builder's default approximation enabled.
- No base map, labels, navigation, arrow icons or elevation provider. Angle 45
  tests a tilted camera, not streamed terrain data.
- Zoom moves from 16.0 to 16.2 across 120 input/render cycles. `--repeat 1`
  changes the zoom every cycle; `--repeat 3` retains each value for three cycles.
- Each cycle takes at least 16.667 ms, sleeping only after the timed work. Slow
  cycles are not shortened or dropped. Therefore runs with expensive updates
  take longer; this is a deterministic event-sequence benchmark, not a timed
  playback of a recorded touch gesture.
- GPU resources are uploaded on the render thread; the separate GPU worker is
  disabled. `glFinish()` includes completion of submitted GPU work in timings.

## Output

Times are milliseconds. The initial warmup is excluded.

- `frameMean`, `frameP95`, `frameMax`: wall time of update, conditional preparation,
  conditional rendering and GPU completion. These are offscreen processing times,
  not CADisplayLink frame intervals or measured application FPS.
- `updateP95`: time in the real renderer's `update()` call, which includes line
  geometry updates in `preUpdate()`.
- `late`: cycles exceeding 16.667 ms, not dropped display frames.
- `motionRebuilds`, `settleRebuilds`: line update notifications after warmup. The
  benchmark does not change line properties, so these indicate geometry updates.
- `idleAt`: zero-based post-input cycle at which real `IMapRenderer::isIdle()`
  became true. Negative means the check timed out.
- `idleFrames`: number of subsequent idle observations out of 60, spaced by 10 ms.
- `idleRebuilds`: additional line updates during those idle observations.
- `renderFailures`: rejected renders after warmup. Nonzero invalidates the run.

A zero exit code verifies scene initialization, successful measured rendering and
stable idle. It does not mean the measured performance meets a frame-time budget.

## Interpretation

Compare the same scene, input sequence, compiler configuration and simulator.
The existing local build flags are reused (currently Debug), so absolute times
must not be presented as Release iPhone performance or a battery measurement.
These dense synthetic tracks are stress inputs, not recordings of typical GPX
usage. Device checks with representative GPX data remain a separate measurement.

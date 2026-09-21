import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description='Build and run the native VectorLine test on an iOS simulator')
    parser.add_argument('--device', required=True, help='Booted simulator UUID')
    parser.add_argument('--revision', help='Core revision for the line implementation; defaults to working tree')
    parser.add_argument('--workspace', type=Path, help='Directory containing core, baked and binaries')
    parser.add_argument('--output', type=Path, help='Directory for executable and build log')
    parser.add_argument('--benchmark', action='store_true', help='Run the offscreen renderer benchmark')
    parser.add_argument('--points', type=int, default=10000)
    parser.add_argument('--lines', type=int, default=1)
    parser.add_argument('--repeat', type=int, default=3, help='Frames per distinct zoom value')
    parser.add_argument('--angle', type=int, default=90)
    args = parser.parse_args()
    core = Path(__file__).resolve().parents[1]
    workspace = (args.workspace or core.parent).resolve()
    output = (args.output or Path(tempfile.mkdtemp(prefix='vector-line-zoom-ios-'))).resolve()
    output.mkdir(parents=True, exist_ok=True)
    build = workspace / 'baked/fat-ios-clang.xcode'
    responses = list((build / 'build/OsmAndCore_static_standalone.build/Debug-iphonesimulator/Objects-normal/arm64').glob('*.resp'))
    if len(responses) != 1:
        parser.error('Expected one existing ARM64 simulator Core response file in ' + str(build))
    qt = core / 'externals/qtbase-ios/upstream.patched.ios.simulator.clang.static'
    libraries = workspace / 'binaries/ios.clang-iphonesimulator/Debug'
    if not (libraries / 'libOsmAndCore_static_standalone.a').is_file() or not (qt / 'lib/libQt5Core.a').is_file():
        parser.error('Existing simulator Core/dependency archives and simulator Qt are required')
    source = core / 'src/Map'
    if args.revision:
        revision = subprocess.check_output(['git', '-C', str(core), 'rev-parse', '--verify', args.revision + '^{commit}'], text=True).strip()
        source = output / 'src'
        source.mkdir(exist_ok=True)
        for name in ['VectorLine.cpp', 'VectorLine_P.cpp', 'VectorLine_P.h', 'VectorLineBuilder_P.cpp',
                     'GeometryModifiers.cpp', 'GeometryModifiers.h']:
            (source / name).write_bytes(subprocess.check_output(['git', '-C', str(core), 'show', revision + ':src/Map/' + name]))
        print('Line implementation revision:', revision, flush=True)
    else:
        print('Line implementation: current working tree', flush=True)
    flags = shlex.split(responses[0].read_text())
    flags = [flag.replace(str(core / 'externals/qtbase-ios/upstream.patched.ios.clang-fat.static'), str(qt)) for flag in flags]
    sdk = subprocess.check_output(['xcrun', '--sdk', 'iphonesimulator', '--show-sdk-path'], text=True).strip()
    compiler = ['xcrun', 'clang++', '-isysroot', sdk, '-target', 'arm64-apple-ios15.0-simulator']
    test_name = 'VectorLineRendererBenchmark.mm' if args.benchmark else 'TestVectorLineZoom.cpp'
    executable = output / Path(test_name).stem
    objects = []
    print('Build log:', output / 'build.log', flush=True)
    with (output / 'build.log').open('w') as log:
        for name in ['VectorLine.cpp', 'VectorLine_P.cpp', 'VectorLineBuilder_P.cpp', 'GeometryModifiers.cpp', test_name]:
            if name == test_name:
                path = core / 'tests' / ('benchmarks' if args.benchmark else 'unit') / name
            else:
                path = source / name
            obj = output / (name + '.o')
            objects.append(str(obj))
            subprocess.run(compiler + flags + ['-c', str(path), '-o', str(obj)], cwd=build,
                           stdout=log, stderr=log, check=True)
        command = compiler + objects + [str(path) for path in sorted(libraries.glob('*.a'))]
        command += [str(qt / 'lib' / name) for name in ['libQt5Core.a', 'libQt5Concurrent.a', 'libqtpcre2.a']]
        command += ['-lz', '-liconv', '-Wl,-dead_strip']
        for framework in ['Foundation', 'CoreFoundation', 'UIKit', 'CoreGraphics', 'CoreText', 'ImageIO',
                          'Security', 'SystemConfiguration', 'MobileCoreServices', 'IOKit', 'OpenGLES', 'QuartzCore', 'Accelerate']:
            command += ['-framework', framework]
        subprocess.run(command + ['-o', str(executable)], stdout=log, stderr=log, check=True)
    print('Running:', executable, flush=True)
    command = ['xcrun', 'simctl', 'spawn', args.device, str(executable)]
    if args.benchmark:
        command += [str(args.points), str(args.lines), str(args.repeat), str(args.angle), str(workspace / 'resources')]
    result = subprocess.run(command, capture_output=True, text=True)
    (output / 'run.log').write_text(result.stdout + result.stderr)
    print(result.stdout, end='')
    print(result.stderr, end='')
    raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()

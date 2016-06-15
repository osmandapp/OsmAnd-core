#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os
import struct
import zlib

# =============================================================================
# =============================================================================
# =============================================================================

class OsmAndCoreResourcesPacker(object):
    # -------------------------------------------------------------------------
    def __init__(self):
        return

    # -------------------------------------------------------------------------
    def pack(self, workroot):
        # Pack each file found in working directory
        for path, dirs, files in os.walk(workroot):
            def is_path_ignored(path):
                return path.startswith('.') or path.endswith('.png')

            if is_path_ignored(path):
                print("Ignoring '%s'" % (path))
                continue
            print("Listing '%s' with %d files" % (path, len(files)))
            for filename in files:
                if is_path_ignored(filename):
                    print("Ignoring '%s'" % (filename))
                    continue

                filepath = os.path.join(path, filename)

                originalSize = os.path.getsize(filepath)
                with open(filepath, "rb") as inputFile:
                    fileContent = inputFile.read()

                packedContent = zlib.compress(fileContent, 9)
                packedSize = len(packedContent)

                with open(filepath + ".qz", "wb") as compressedFile:
                    compressedFile.write(struct.pack(">i", originalSize))
                    compressedFile.write(packedContent)

                os.remove(filepath)

                print("Packed '%s' from %d to %d" % (os.path.relpath(filepath, workroot), originalSize, packedSize + 4))

        return True

# =============================================================================
# =============================================================================
# =============================================================================

if __name__=='__main__':
    # Working directory
    workingDir = os.getcwd()
    if len(sys.argv) >= 2:
        workingDir = sys.argv[1]
    print("Working in: %s" % (workingDir))

    packer = OsmAndCoreResourcesPacker()
    ok = packer.pack(workingDir)
    sys.exit(0 if ok else -1)

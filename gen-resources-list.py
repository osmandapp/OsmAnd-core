#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os
import zlib
import re

# =============================================================================
# =============================================================================
# =============================================================================

class OsmAndCoreResourcesListGenerator(object):
    # -------------------------------------------------------------------------
    def __init__(self):
        return

    # -------------------------------------------------------------------------
    def generate(self, root, resourcesSubpaths, rules, outputFilename):
    	# Open output file
        try:
            outputFile = open(outputFilename, "w")
        except IOError:
            print("Failed to open '%s' for writing" % (outputFilename))
            return False

        # List all files
        print("Looking for files...")
        filenames = []
        for subpath in resourcesSubpaths:
        	for resourcesRoot, dirs, files in os.walk(os.path.join(root, subpath)):
        		if resourcesRoot.startswith('.'):
        			print("Ignoring '%s'" % (resourcesRoot))
        			continue
        		print("Listing '%s' with %d files" % (resourcesRoot, len(files)))
        		for filename in files:
        			filepath = os.path.join(resourcesRoot, filename)
        			filenames.append(os.path.relpath(filepath, root))
        print("Found %d files to test" % (len(filenames)))

        # Apply each rule to each file entry
        for rule in rules:
        	print("Processing rule '%s' => '%s' rule:" % (rule[0], rule[1]))
        	processed = []
        	for filename in filenames:
        		if re.match(rule[0], filename) == None:
        			continue
        		listname = re.sub(rule[0], rule[1], filename);
        		processed.append(filename);
        		outputFile.write("/%s:%s\n" % (filename, listname));

        		print("\t '%s' => '%s'" % (filename, listname))
        	print("\t%d processed" % (len(processed)))
        	filenames[:] = [filename for filename in filenames if not filename in processed]

        print("%d unmatched" % (len(filenames)))

        outputFile.flush()
        outputFile.close()

        return True

# =============================================================================
# =============================================================================
# =============================================================================

if __name__=='__main__':
    # Get root directory of entire project
    rootDir = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), ".."))
    
    rules = [
        # Map styles and related:
        [r'resources/rendering_styles/default\.render\.xml', r'map/styles/default.render.xml'],
        [r'resources/rendering_styles/default\.map_styles_presets\.xml', r'map/presets/default.map_styles_presets.xml'],

        # Map icons:
        [r'resources/rendering_styles/style-icons/drawable-mdpi/h_(.*shield.*)\.png', r'map/shields/\1.png'],
        [r'resources/rendering_styles/style-icons/drawable-mdpi/h_(.*)\.png', r'map/shaders/\1.png'],
        [r'resources/rendering_styles/style-icons/drawable-mdpi/mm_(.*)\.png', r'map/map_icons/\1.png'],

        # Routing:
        [r'resources/routing/routing\.xml', r'routing/routing.xml'],

        # Data for ICU
        [r'core/externals/icu4c/upstream\.data/icudt\d+([lb])\.dat', r'icu4c/icu-data-\1.dat'],

        # Fonts:
        [r'resources/rendering_styles/fonts/(.*)/(.*)\.(ttf)', r'map/fonts/\1/\2.\3'],
        [r'resources/rendering_styles/fonts/(.*)\.(ttf)', r'map/fonts/\1.\2'],

        # Misc resources:
        [r'resources/rendering_styles/stubs/(.*)\.png', r'map/stubs/\1.png'],
    ]

    resourcesSubpaths = [
    	"resources",
    	"core/externals/icu4c/upstream.data"
    ]
    resourcesListFilename = rootDir + "/core/embed-resources.list";
    generator = OsmAndCoreResourcesListGenerator()
    ok = generator.generate(rootDir, resourcesSubpaths, rules, resourcesListFilename)
    sys.exit(0 if ok else -1)

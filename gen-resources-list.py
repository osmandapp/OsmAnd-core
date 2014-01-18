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
    def generate(self, resourcesRoot, rules, outputFilename):
    	# Open output file
        try:
            outputFile = open(outputFilename, "w")
        except IOError:
            print("Failed to open '%s' for writing" % (outputFilename))
            return False

        # List all files
        print("Looking for files...")
        filenames = []
        for root, dirs, files in os.walk(resourcesRoot):
        	if root.startswith('.'):
        		print("Ignoring '%s'" % (root))
        		continue
        	print("Listing '%s' with %d files" % (root, len(files)))
        	for filename in files:
        		filepath = os.path.join(root, filename)
        		filenames.append(os.path.relpath(filepath, resourcesRoot))
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
        		outputFile.write("/resources/%s:%s\n" % (filename, listname));

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
    	[r'rendering_styles/default\.render\.xml', r'map/styles/default.render.xml'],
    	[r'rendering_styles/style-icons/drawable-mdpi/h_(.*shield.*)\.png', r'map/shields/\1.png'],
    	[r'rendering_styles/style-icons/drawable-mdpi/h_(.*)\.png', r'map/shaders/\1.png'],
    	[r'rendering_styles/style-icons/drawable-mdpi/mm_(.*)\.png', r'map/map_icons/\1.png'],
    	[r'rendering_styles/stubs/(.*)\.png', r'map/stubs/\1.png'],
    	[r'routing/routing\.xml', r'routing/routing.xml'],
    ]

    resourcesRootPath = rootDir + "/resources";
    resourcesListFilename = rootDir + "/core/embed-resources.list";
    generator = OsmAndCoreResourcesListGenerator()
    ok = generator.generate(resourcesRootPath, rules, resourcesListFilename)
    sys.exit(0 if ok else -1)

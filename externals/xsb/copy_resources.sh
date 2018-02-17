mkdir -p ../../../ios/Resources/XSB/config/x86_64-apple-darwin/lib
cp -fR upstream.patched/XSB/config/x86_64-apple-darwin/lib ../../../ios/Resources/XSB/config/x86_64-apple-darwin

#./syslib	    	Prolog source files for XSB system predicates
#./cmplib	    	Prolog source files for XSB's compiler
#./prolog-commons   Prolog-commons files included with XSB (v. 3.3 and forward)
#./lib		    Prolog source files for useful, but non-essential XSB predicates
#./gpp		    Source files for the GNU pre-processor used by XSB's compiler

mkdir -p ../../../ios/Resources/XSB/prolog_includes
mkdir -p ../../../ios/Resources/XSB/syslib
mkdir -p ../../../ios/Resources/XSB/cmplib
mkdir -p ../../../ios/Resources/XSB/prolog-commons
mkdir -p ../../../ios/Resources/XSB/lib
mkdir -p ../../../ios/Resources/XSB/gpp

cp -f upstream.patched/XSB/prolog_includes/*.* ../../../ios/Resources/XSB/prolog_includes
cp -f upstream.patched/XSB/syslib/*.* ../../../ios/Resources/XSB/syslib
cp -f upstream.patched/XSB/cmplib/*.* ../../../ios/Resources/XSB/cmplib
cp -f upstream.patched/XSB/prolog-commons/*.* ../../../ios/Resources/XSB/prolog-commons
cp -f upstream.patched/XSB/lib/*.* ../../../ios/Resources/XSB/lib
cp -f upstream.patched/XSB/gpp/*.* ../../../ios/Resources/XSB/gpp


cp -fR upstream.patched/XSB ../../../ios/Resources

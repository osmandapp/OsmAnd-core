mkdir -p ../../../ios/Resources/XSB/config/x86_64-apple-darwin/lib
cp -fR upstream.patched/XSB/config/x86_64-apple-darwin/lib ../../../ios/Resources/XSB/config/x86_64-apple-darwin

#./syslib	    	Prolog source files for XSB system predicates
#./cmplib	    	Prolog source files for XSB's compiler
#./prolog-commons   Prolog-commons files included with XSB (v. 3.3 and forward)

mkdir -p ../../../ios/Resources/XSB/syslib
mkdir -p ../../../ios/Resources/XSB/cmplib
#mkdir -p ../../../ios/Resources/XSB/prolog-commons

cp -f upstream.patched/XSB/syslib/*.xwam ../../../ios/Resources/XSB/syslib
cp -f upstream.patched/XSB/cmplib/*.xwam ../../../ios/Resources/XSB/cmplib
#cp -fR upstream.patched/XSB/syslib/ ../../../ios/Resources/XSB/syslib
#cp -fR upstream.patched/XSB/cmplib/ ../../../ios/Resources/XSB/cmplib
#cp -fR upstream.patched/XSB/prolog-commons/ ../../../ios/Resources/XSB/prolog-commons

#cp -f upstream.patched/XSB/syslib/loader.xwam ../../../ios/Resources/XSB/syslib
#cp -f upstream.patched/XSB/syslib/*.xwam ../../../ios/Resources/XSB/syslib

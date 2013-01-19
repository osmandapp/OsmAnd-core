export UGCJ=gcj
$UGCJ -o core.o --classpath=OsmAnd-core.jar:lib/icu4j-49_1.jar:lib/bsh-core-2.0b4.jar:lib/junidecode-0.1.jar:lib/bzip2-20090327.jar:lib/commons-logging-1.1.1.jar:lib/json-20090211.jar:lib/kxml2-2.3.0.jar:lib/tuprolog.jar -c OsmAnd-core.jar
$UGCJ -o kxml.o -c lib/kxml2-2.3.0.jar 
$UGCJ -o json.o -c lib/json-20090211.jar 
$UGCJ -o logging.o -c lib/simple-logging.jar 
$UGCJ -o icu.o -c lib/icu4j-49_1.jar 
$UGCJ -o tuprolog.o -c lib/tuprolog.jar
$UGCJ -o junidecode.o -c lib/junidecode-0.1.jar
$UGCJ -o bsh.o -c lib/bsh-core-2.0b4.jar
$UGCJ -o bzip2.o -c lib/bzip2-20090327.jar
$UGCJ --main=net.osmand.binary.BinaryInspector \
 	-o native core.o kxml.o logging.o json.o icu.o tuprolog.o \
 	junidecode.o bsh.o bzip2.o ../binaries/linux/i686/libosmand.so

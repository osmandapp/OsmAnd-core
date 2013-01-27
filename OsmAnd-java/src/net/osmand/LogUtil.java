package net.osmand;

import java.text.Collator;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

/**
 * That class is replacing of standard LogFactory due to 
 * problems with Android implementation of LogFactory.
 * See Android analog of  LogUtil
 *
 * That class should be very simple & always use LogFactory methods,
 * there is an intention to delegate all static methods to LogFactory.
 */
public class LogUtil {
	
	public static Log getLog(Class<?> cl){
		return LogFactory.getLog(cl);
	}
	
	public static XmlPullParser newXMLPullParser() throws XmlPullParserException{
		return new org.kxml2.io.KXmlParser();
	}
	

	public static net.osmand.Collator primaryCollator(){
		final Collator instance = Collator.getInstance();
		instance.setStrength(Collator.PRIMARY);
		return wrapCollator(instance);
	}

	public static net.osmand.Collator wrapCollator(final Collator instance) {
		return new net.osmand.Collator() {
			
			@Override
			public int compare(Object o1, Object o2) {
				return instance.compare(o1, o2);
			}
			
			@Override
			public boolean equals(Object obj) {
				return instance.equals(obj);
			}

			@Override
			public boolean equals(String source, String target) {
				return instance.equals(source, target);
			}

			@Override
			public int compare(String source, String target) {
				return instance.compare(source, target);
			}
		};
	}
}

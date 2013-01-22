package org.apache.commons.logging;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogImpl;

public class LogFactory {

	public static Log getLog(Class<?> cl){
		return new LogImpl(cl);
	}
}

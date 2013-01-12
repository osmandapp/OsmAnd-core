package org.apache.commons.logging;

public class LogFactory {

	public static Log getLog(Class<?> cl){
		return new Log(cl);
	}
}

package org.apache.commons.logging;

public class LogImpl implements Log {

	private Class<?> cl;

	public LogImpl(Class<?> cl) {
		this.cl = cl;
	}
	
	private void log(String type, Object msg, Exception e){
		System.out.println(type.toUpperCase() + ": " + cl.getSimpleName() + "  " +msg);
		if(e != null) {
			e.printStackTrace();
		}
	}

	public void debug(Object msg) {
		log("DEBUG", msg, null);
	}
	
	public void debug(Object msg, Exception e) {
		log("DEBUG", msg, e);
	}
	public boolean isDebugEnabled(){
		return true;
	}

	public void info(Object msg) {
		log("INFO", msg, null);
	}
	
	public void info(Object msg, Exception e) {
		log("INFO", msg, e);
	}
	public boolean isInfoEnabled(){
		return true;
	}

	public void error(Object msg) {
		log("ERROR", msg, null);
	}
	
	public void error(Object msg, Exception e) {
		log("ERROR", msg, e);
	}
	public boolean isErrorEnabled(){
		return true;
	}
	
	public void warn(Object msg) {
		log("WARN", msg, null);
	}
	
	public void warn(Object msg, Exception e) {
		log("WARN", msg, e);
	}
	public boolean isWarnEnabled(){
		return true;
	}

}

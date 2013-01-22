package org.apache.commons.logging;

public interface Log {
	public void debug(Object msg);
	
	public void debug(Object msg, Exception e);
	
	public boolean isDebugEnabled();

	public void info(Object msg) ;
	
	public void info(Object msg, Exception e);
	
	public boolean isInfoEnabled();

	public void error(Object msg) ;
	
	public void error(Object msg, Exception e);
	
	public boolean isErrorEnabled();
	
	public void warn(Object msg);
	
	public void warn(Object msg, Exception e);
	
	public boolean isWarnEnabled();

}

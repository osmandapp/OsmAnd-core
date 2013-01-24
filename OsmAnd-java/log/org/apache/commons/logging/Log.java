package org.apache.commons.logging;

public interface Log {
	public void debug(Object msg);
	
	public void debug(Object msg, Throwable e);
	
	public boolean isDebugEnabled();

	public void info(Object msg) ;
	
	public void info(Object msg, Throwable e);
	
	public boolean isInfoEnabled();

	public void error(Object msg) ;
	
	public void error(Object msg, Throwable e);
	
	public boolean isErrorEnabled();
	
	public void warn(Object msg);
	
	public void warn(Object msg, Throwable e);
	
	public boolean isWarnEnabled();
	
	public void fatal(Object msg);
	
	public void fatal(Object msg, Throwable e);
	
	public boolean isFatalEnabled();

}

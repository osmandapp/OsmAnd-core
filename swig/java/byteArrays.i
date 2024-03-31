%typemap(jni) (char *BYTE, size_t LENGTH) "jbyteArray"
%typemap(jtype) (char *BYTE, size_t LENGTH) "byte[]"
%typemap(jstype) (char *BYTE, size_t LENGTH) "byte[]"
%typemap(in, numinputs=1) (char *BYTE, size_t LENGTH) {
  $1 = (char *) JCALL2(GetByteArrayElements, jenv, $input, 0);
  $2 = JCALL1(GetArrayLength, jenv, $input);
}

%typemap(argout) (char *BYTE, size_t LENGTH) {
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
}

%typemap(javain) (char *BYTE, size_t LENGTH) "$javainput"

/* Prevent default freearg typemap from being used */
%typemap(freearg) (char *BYTE, size_t LENGTH) ""

/*
  This allows to return array of bytes. Example:

  char* getBytesArray(size_t* LENGTH)
  {
      *LENGTH = bytes.size();
      return bytes.data();
  }

*/
%typemap(jni) signed char* getBytesArray "jbyteArray"
%typemap(jtype) signed char* getBytesArray "byte[]"
%typemap(jstype) signed char* getBytesArray "byte[]"
%typemap(javaout) signed char* getBytesArray {
  return $jnicall;
}

%typemap(in,numinputs=0,noblock=1) size_t* LENGTH { 
  size_t length=0;
  $1 = &length;
}

%typemap(out) signed char* getBytesArray {
  $result = JCALL1(NewByteArray, jenv, length);
  JCALL4(SetByteArrayRegion, jenv, $result, 0, length, $1);
}

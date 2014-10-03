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

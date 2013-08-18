/* -----------------------------------------------------------------------------
 * copy of std_string.i
 *
 * Typemaps for qstring and const qstring&
 * These are mapped to a Java String and are passed around by value.
 *
 * To use non-const std::string references use the following %apply.  Note 
 * that they are passed by value.
 * %apply const qstring & {qstring &};
 * ----------------------------------------------------------------------------- */

%{
#include <QString>
%}

%naturalvar QString;

class QString;

// string
%typemap(jni) QString "jstring"
%typemap(jtype) QString "String"
%typemap(jstype) QString "String"
%typemap(javadirectorin) QString "$jniinput"
%typemap(javadirectorout) QString "$javacall"

%typemap(in) QString 
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null QString");
     return $null;
    } 
    const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
    if (!$1_pstr) return $null;
    $1 = QString::fromStdString($1_pstr);
    jenv->ReleaseStringUTFChars($input, $1_pstr); %}

%typemap(directorout) string 
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
   } 
   const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
   if (!$1_pstr) return $null;
   $result = QString::fromStdString($1_pstr);
   jenv->ReleaseStringUTFChars($input, $1_pstr); %}

%typemap(directorin,descriptor="Ljava/lang/String;") QString 
%{ $input = jenv->NewStringUTF($1.toStdString().c_str()); %}

%typemap(out) QString 
%{ $result = jenv->NewStringUTF($1.toStdString().c_str()); %}

%typemap(javain) QString "$javainput"

%typemap(javaout) QString {
    return $jnicall;
  }

%typemap(typecheck) QString = char *;

%typemap(throws) QString
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

// const string &
%typemap(jni) const QString & "jstring"
%typemap(jtype) const QString & "String"
%typemap(jstype) const QString & "String"
%typemap(javadirectorin) const QString & "$jniinput"
%typemap(javadirectorout) const QString & "$javacall"

%typemap(in) const QString &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null QString");
     return $null;
   }
   const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
   if (!$1_pstr) return $null;
   QString $1_str(QString::fromStdString($1_pstr));
   $1 = &$1_str;
   jenv->ReleaseStringUTFChars($input, $1_pstr); %}

%typemap(directorout,warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const QString &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null QString");
     return $null;
   }
   const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
   if (!$1_pstr) return $null;
   /* possible thread/reentrant code problem */
   static QString $1_str;
   $1_str = QString::fromStdString($1_pstr);
   $result = &$1_str;
   jenv->ReleaseStringUTFChars($input, $1_pstr); %}

%typemap(directorin,descriptor="Ljava/lang/String;") const QString &
%{ $input = jenv->NewStringUTF($1.toStdString().c_str()); %}

%typemap(out) const QString & 
%{ $result = jenv->NewStringUTF($1->toStdString().c_str()); %}

%typemap(javain) const QString & "$javainput"

%typemap(javaout) const QString & {
    return $jnicall;
  }

%typemap(typecheck) const QString & = char *;

%typemap(throws) const QString &
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

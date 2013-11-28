
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include "gl/GrGLInterface.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


const GrGLInterface* GrGLCreateNativeInterface() {
    GrGLInterface* interface = SkNEW(GrGLInterface);
    interface->fBindingsExported = kES_GrGLBinding;

    interface->fActiveTexture = glActiveTexture;
    interface->fAttachShader = glAttachShader;
    interface->fBindAttribLocation = glBindAttribLocation;
    interface->fBindBuffer = glBindBuffer;
    interface->fBindTexture = glBindTexture;
    interface->fBindVertexArray = glBindVertexArrayOES;
    interface->fBlendColor = glBlendColor;
    interface->fBlendFunc = glBlendFunc;
    interface->fBufferData = glBufferData;
    interface->fBufferSubData = glBufferSubData;
    interface->fClear = glClear;
    interface->fClearColor = glClearColor;
    interface->fClearStencil = glClearStencil;
    interface->fColorMask = glColorMask;
    interface->fCompileShader = glCompileShader;
    interface->fCompressedTexImage2D = glCompressedTexImage2D;
    interface->fCopyTexSubImage2D = glCopyTexSubImage2D;
    interface->fCreateProgram = glCreateProgram;
    interface->fCreateShader = glCreateShader;
    interface->fCullFace = glCullFace;
    interface->fDeleteBuffers = glDeleteBuffers;
    interface->fDeleteProgram = glDeleteProgram;
    interface->fDeleteShader = glDeleteShader;
    interface->fDeleteTextures = glDeleteTextures;
    interface->fDeleteVertexArrays = glDeleteVertexArraysOES;
    interface->fDepthMask = glDepthMask;
    interface->fDisable = glDisable;
    interface->fDisableVertexAttribArray = glDisableVertexAttribArray;
    interface->fDrawArrays = glDrawArrays;
    interface->fDrawElements = glDrawElements;
    interface->fEnable = glEnable;
    interface->fEnableVertexAttribArray = glEnableVertexAttribArray;
    interface->fFinish = glFinish;
    interface->fFlush = glFlush;
    interface->fFrontFace = glFrontFace;
    interface->fGenBuffers = glGenBuffers;
    interface->fGenerateMipmap = glGenerateMipmap;
    interface->fGenTextures = glGenTextures;
    interface->fGenVertexArrays = glGenVertexArraysOES;
    interface->fGetBufferParameteriv = glGetBufferParameteriv;
    interface->fGetError = glGetError;
    interface->fGetIntegerv = glGetIntegerv;
    interface->fGetProgramInfoLog = glGetProgramInfoLog;
    interface->fGetProgramiv = glGetProgramiv;
    interface->fGetShaderInfoLog = glGetShaderInfoLog;
    interface->fGetShaderiv = glGetShaderiv;
    interface->fGetString = glGetString;
    interface->fGetUniformLocation = glGetUniformLocation;
    interface->fLineWidth = glLineWidth;
    interface->fLinkProgram = glLinkProgram;
    interface->fPixelStorei = glPixelStorei;
    interface->fReadPixels = glReadPixels;
    interface->fScissor = glScissor;
    interface->fShaderSource = (GrGLShaderSourceProc)glShaderSource;
    interface->fStencilFunc = glStencilFunc;
    interface->fStencilFuncSeparate = glStencilFuncSeparate;
    interface->fStencilMask = glStencilMask;
    interface->fStencilMaskSeparate = glStencilMaskSeparate;
    interface->fStencilOp = glStencilOp;
    interface->fStencilOpSeparate = glStencilOpSeparate;
    interface->fTexImage2D = glTexImage2D;
    interface->fTexParameteri = glTexParameteri;
    interface->fTexParameteriv = glTexParameteriv;
    interface->fTexSubImage2D = glTexSubImage2D;
    interface->fDiscardFramebuffer = glDiscardFramebufferEXT;
    interface->fUniform1f = glUniform1f;
    interface->fUniform1i = glUniform1i;
    interface->fUniform1fv = glUniform1fv;
    interface->fUniform1iv = glUniform1iv;
    interface->fUniform2f = glUniform2f;
    interface->fUniform2i = glUniform2i;
    interface->fUniform2fv = glUniform2fv;
    interface->fUniform2iv = glUniform2iv;
    interface->fUniform3f = glUniform3f;
    interface->fUniform3i = glUniform3i;
    interface->fUniform3fv = glUniform3fv;
    interface->fUniform3iv = glUniform3iv;
    interface->fUniform4f = glUniform4f;
    interface->fUniform4i = glUniform4i;
    interface->fUniform4fv = glUniform4fv;
    interface->fUniform4iv = glUniform4iv;
    interface->fUniformMatrix2fv = glUniformMatrix2fv;
    interface->fUniformMatrix3fv = glUniformMatrix3fv;
    interface->fUniformMatrix4fv = glUniformMatrix4fv;
    interface->fUseProgram = glUseProgram;
    interface->fVertexAttrib4fv = glVertexAttrib4fv;
    interface->fVertexAttribPointer = glVertexAttribPointer;
    interface->fViewport = glViewport;
    interface->fBindFramebuffer = glBindFramebuffer;
    interface->fBindRenderbuffer = glBindRenderbuffer;
    interface->fCheckFramebufferStatus = glCheckFramebufferStatus;
    interface->fDeleteFramebuffers = glDeleteFramebuffers;
    interface->fDeleteRenderbuffers = glDeleteRenderbuffers;
    interface->fFramebufferRenderbuffer = glFramebufferRenderbuffer;
    interface->fFramebufferTexture2D = glFramebufferTexture2D;

    interface->fGenFramebuffers = glGenFramebuffers;
    interface->fGenRenderbuffers = glGenRenderbuffers;
    interface->fGetFramebufferAttachmentParameteriv = glGetFramebufferAttachmentParameteriv;
    interface->fGetRenderbufferParameteriv = glGetRenderbufferParameteriv;
    interface->fRenderbufferStorage = glRenderbufferStorage;

    interface->fMapBuffer = glMapBufferOES;
    interface->fUnmapBuffer = glUnmapBufferOES;

    return interface;
}

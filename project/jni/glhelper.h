/*
 * Copyright (C) 2011 Olof Sjöbergh
 *
 */

#define  LOG_TAG    "libglmap"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

int colorIsEqual(GLubyte rgba1[4], GLubyte rgba2[4]);

void printGLString(const char *name, GLenum s);

void checkGlError(const char* op);

GLuint loadShader(GLenum shaderType, const char* pSource);

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);


/*
 * Copyright (C) 2011 Olof Sjöbergh
 *
 */

#include <jni.h>

#include "glmaprenderer.h"
#include "glmapjni.h"

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_init(JNIEnv * env, jobject obj)
{
    mapInit();
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_setWindowSize(JNIEnv * env, jobject obj,  jint width, jint height)
{
    mapSetWindowSize(width, height);
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_step(JNIEnv * env, jobject obj)
{
    mapRenderFrame();
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_move(JNIEnv * env, jobject obj, jdouble x, jdouble y, jdouble z)
{
    mapMove(x, y, z);
}


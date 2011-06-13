/*
 * Copyright (C) 2011 Olof Sjöbergh
 *
 */

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_init(JNIEnv * env, jobject obj);

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_setWindowSize(JNIEnv * env, jobject obj,  jint width, jint height);

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_step(JNIEnv * env, jobject obj);

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_move(JNIEnv * env, jobject obj, jdouble x, jdouble y, jdouble z);


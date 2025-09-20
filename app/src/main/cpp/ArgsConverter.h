//
// Created by XiaM on 2025/9/20.
//

#ifndef TEST_ARGSCONVERTER_H
#define TEST_ARGSCONVERTER_H

#include <jni.h>
#include <string>
#include "art.h"

// 参数提取函数
struct MethodArgs {
    jchar ch;
    jfloat f;
    jlong l;
    jdouble d;
    jstring str;
    jbyte b;
    jint n;
};

//java与native间需要参数和栈的转换
class ArgsConverter {
private:

public:
    static jobjectArray
    artArgs2JArray(ArtMethod *method, Object *thiz, Thread *self, char *shorty,
                   uint32_t *args, uint64_t *xregs, double *fregs);

    static void
    JArray2ARTArgs(jobjectArray javaArgs, char *shorty, uint32_t *args, uint64_t *xregs,
                   double *fregs, Thread *self);

    static MethodArgs parseArgsFromJArray(JNIEnv *env, jobjectArray args);

    static void setArgs2JArray(JNIEnv *env, jobjectArray args, const MethodArgs &newArgs);

};

#endif //TEST_ARGSCONVERTER_H

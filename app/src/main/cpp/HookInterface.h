//
// Created by XiaM on 2025/9/20.
//

#ifndef TEST_HOOKINTERFACE_H
#define TEST_HOOKINTERFACE_H

#include <jni.h>

class IHookCallback{
public:
    virtual ~IHookCallback() = default;

    // Before: 修改参数
    virtual void onBefore(JNIEnv* env, jobject thiz, jobjectArray args) = 0;

    // After: 修改返回值
    virtual void onAfter(JNIEnv* env, jobject thiz, jobjectArray args, jobject* result) = 0;

};

#endif //TEST_HOOKINTERFACE_H

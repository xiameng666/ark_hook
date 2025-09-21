//
// Created by XiaM on 2025/9/20.
//

#include "ArgsConverter.h"
#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "ArgsConverter", __VA_ARGS__)

#define GET_PRIM_TYPE_METHOD(type, jtype, short) \
jclass cls_##type = g_env->FindClass("java/lang/"#jtype);\
jmethodID meth_valueof_##type = g_env->GetStaticMethodID(cls_##type, "valueOf", "("#short")Ljava/lang/"#jtype";");\

#define GET_SETPRIM_TYPE_METHOD(type, jtype, short)\
GET_PRIM_TYPE_METHOD(type, jtype, short)\
jmethodID meth_##type##_value = g_env->GetMethodID(cls_##type, #type"Value", "()"#short);

#define GET_ARG(args, index, ret, jtype)  g_env->Call##ret##Method(g_env->GetObjectArrayElement(args, index),  meth_##jtype##_value)

// ART参数 → Java对象数组 (CallBack进入时)
jobjectArray
ArgsConverter::artArgs2JArray(JNIEnvExt *g_env, ArtMethod *method, Object *thiz, Thread *self,
                              char *shorty, uint32_t *args, uint64_t *xregs, double *fregs) {
    LOGV("转换1: ART参数 → Java对象数组");
    GET_SETPRIM_TYPE_METHOD(byte, Byte, B);
    GET_SETPRIM_TYPE_METHOD(char, Character, C);
    GET_SETPRIM_TYPE_METHOD(short, Short, S);
    GET_SETPRIM_TYPE_METHOD(int, Integer, I);
    GET_SETPRIM_TYPE_METHOD(long, Long, J);
    GET_SETPRIM_TYPE_METHOD(boolean, Boolean, Z);
    GET_SETPRIM_TYPE_METHOD(float, Float, F);
    GET_SETPRIM_TYPE_METHOD(double, Double, D);

    //参数个数
    int args_count = strlen(shorty) - 1;//减去返回值类型
    jclass cls = g_env->FindClass("java/lang/Object");
    jobjectArray ja = g_env->NewObjectArray(args_count, cls, nullptr);

    //遍历短名称，构造对象数组
    char* sz = shorty+1; //跳过返回值类型
    uint32_t* args_ptr = args + 1; //跳过this
    uint32_t idx = 0;
    while (*sz != '\0'){
        switch (*sz) {
            case 'B':
            {
                //取出数据
                jbyte v = *(jbyte*)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_byte, meth_valueof_byte, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'I':
            {
                //取出数据
                jint v = *(jint *)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_int, meth_valueof_int, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'C':
            {
                //取出数据
                jchar v = *(jchar*)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_char, meth_valueof_char, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'Z':
            {
                //取出数据
                jboolean v = *(jboolean*)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_boolean, meth_valueof_boolean, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'J':
            {
                //取出数据
                jlong v = *(jlong *)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_long, meth_valueof_long, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                args_ptr+=2;
                break;
            }
            case 'L':
            {
                //取出数据
                Object* v = (Object*)*(uint32_t*)args_ptr;

                //转换成引用
                jweak o = g_env->vm->AddWeakGlobalRef(self, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                args_ptr+=1;
                break;
            }
            case 'S':
            {
                //取出数据
                jshort v = *(jshort *)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_short, meth_valueof_short, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                args_ptr+=1;
                break;
            }
            case 'F':
            {
                //取出数据
                jfloat v = *(jfloat *)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_float, meth_valueof_float, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                args_ptr+=1;
                break;
            }
            case 'D':
            {
                //取出数据
                jdouble v = *(jdouble *)args_ptr;

                //填入数组
                jobject o = g_env->CallStaticObjectMethod(cls_double, meth_valueof_double, v);
                g_env->SetObjectArrayElement(ja, idx, o);

                //偏移后移
                args_ptr+=2;
                break;
            }
        }

        ++idx;
        ++sz;
    }

    return ja;
}

// Java对象数组 → ART参数 (CallBack执行Before/After后)
void
ArgsConverter::JArray2ARTArgs(JNIEnvExt *g_env, jobjectArray javaArgs, char *shorty, uint32_t *args,
                              uint64_t *xregs, double *fregs, Thread *self) {
    LOGV("转换4: Java对象数组 → ART参数");

    // 初始化JNI方法ID
    GET_SETPRIM_TYPE_METHOD(byte, Byte, B);
    GET_SETPRIM_TYPE_METHOD(char, Character, C);
    GET_SETPRIM_TYPE_METHOD(short, Short, S);
    GET_SETPRIM_TYPE_METHOD(int, Integer, I);
    GET_SETPRIM_TYPE_METHOD(long, Long, J);
    GET_SETPRIM_TYPE_METHOD(boolean, Boolean, Z);
    GET_SETPRIM_TYPE_METHOD(float, Float, F);
    GET_SETPRIM_TYPE_METHOD(double, Double, D);

    char* sz = shorty+1; //跳过返回值类型
    uint32_t* args_ptr = args + 1; //跳过this
    uint32_t idx = 0;
    int idxOfXRegs = 0;
    int idxOfFRegs = 0;
    uint64_t* xregs_ = xregs+2;
    while (*sz != '\0'){
        switch (*sz) {
            case 'B':
            {
                //存入数据
                *(jbyte*)args_ptr = (jbyte)GET_ARG(javaArgs, idx, Byte, byte);

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(jbyte*)(xregs_+idxOfXRegs) = *(jbyte*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'I':
            {
                //存入数据
                *(jint *)args_ptr  = (jint)GET_ARG(javaArgs, idx, Int, int);

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(jint*)(xregs_+idxOfXRegs) = *(jint*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'C':
            {
                //存入数据
                *(jchar*)args_ptr  = (jchar)GET_ARG(javaArgs, idx, Char, char);

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(jchar*)(xregs_+idxOfXRegs) = *(jchar*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'Z':
            {
                //存入数据
                *(jboolean*)args_ptr  = (jboolean)GET_ARG(javaArgs, idx, Boolean, boolean);

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(jboolean*)(xregs_+idxOfXRegs) = *(jboolean*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                ++args_ptr;
                break;
            }
            case 'J':
            {
                //存入数据
                *(jlong *)args_ptr  = (jlong)GET_ARG(javaArgs, idx, Long, long);

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(jlong*)(xregs_+idxOfXRegs) = *(jlong*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                args_ptr+=2;
                break;
            }
            case 'L':
            {
                //存入数据
                *(uint32_t*)args_ptr  = (uint64_t)self->DecodeJObject(g_env->GetObjectArrayElement(javaArgs, idx));

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(uint32_t*)(xregs_+idxOfXRegs) = *(uint32_t*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                args_ptr+=1;
                break;
            }
            case 'S':
            {
                //存入数据
                *(jshort *)args_ptr  = (jshort)GET_ARG(javaArgs, idx, Short, short);

                //存入寄存器
                if (idxOfXRegs < 6){
                    *(jshort*)(xregs_+idxOfXRegs) = *(jshort*)args_ptr;
                    idxOfXRegs++;
                }

                //偏移后移
                args_ptr+=1;
                break;
            }
            case 'F':
            {
                //取出数据
                *(jfloat *)args_ptr  = (jfloat)GET_ARG(javaArgs, idx, Float, float);

                //存入寄存器
                if (idxOfFRegs < 8){
                    *(jfloat*)(fregs+idxOfFRegs) = *(jfloat*)args_ptr;
                    idxOfFRegs++;
                }

                //偏移后移
                args_ptr+=1;
                break;
            }
            case 'D':
            {
                //取出数据
                *(jdouble *)args_ptr  = (jdouble)GET_ARG(javaArgs, idx, Double, double );

                //存入寄存器
                if (idxOfFRegs < 8){
                    *(jdouble*)(fregs+idxOfFRegs) = *(jdouble*)args_ptr;
                    idxOfFRegs++;
                }

                //偏移后移
                args_ptr+=2;
                break;
            }
        }

        ++idx;
        ++sz;
    }
}

/*
// Java对象数组 → 原始值 (Before/After内部使用)
MethodArgs ArgsConverter::parseArgsFromJArray(JNIEnv* env, jobjectArray args) {
    LOGV("转换2: Java对象数组 → 原始值");

    MethodArgs result;

    // 初始化JNI方法ID
    GET_SETPRIM_TYPE_METHOD(char, Character, C);
    GET_SETPRIM_TYPE_METHOD(float, Float, F);
    GET_SETPRIM_TYPE_METHOD(long , Long, J);
    GET_SETPRIM_TYPE_METHOD(double , Double, D);
    GET_SETPRIM_TYPE_METHOD(byte, Byte, B);
    GET_SETPRIM_TYPE_METHOD(int, Integer, I);

    // 提取参数
    jobject ch_o = env->GetObjectArrayElement(args, 0);
    result.ch = env->CallCharMethod(ch_o, meth_char_value);

    jobject f_o = env->GetObjectArrayElement(args, 1);
    result.f = env->CallFloatMethod(f_o, meth_float_value);

    jobject l_o = env->GetObjectArrayElement(args, 2);
    result.l = env->CallLongMethod(l_o, meth_long_value);

    jobject d_o = env->GetObjectArrayElement(args, 3);
    result.d = env->CallDoubleMethod(d_o, meth_double_value);

    result.str = (jstring)env->GetObjectArrayElement(args, 4);

    jobject b_o = env->GetObjectArrayElement(args, 5);
    result.b = env->CallByteMethod(b_o, meth_byte_value);

    jobject n_o = env->GetObjectArrayElement(args, 6);
    result.n = env->CallIntMethod(n_o, meth_int_value);

    return result;
}

// 原始值 → Java对象数组 (Before/After内部使用)
void ArgsConverter::setArgs2JArray(JNIEnv* env, jobjectArray args, const MethodArgs& newArgs) {
    LOGV("转换3: 原始值 → Java对象数组");

    // 初始化JNI方法ID
    GET_SETPRIM_TYPE_METHOD(char, Character, C);
    GET_SETPRIM_TYPE_METHOD(float, Float, F);
    GET_SETPRIM_TYPE_METHOD(long, Long, J);
    GET_SETPRIM_TYPE_METHOD(double, Double, D);
    GET_SETPRIM_TYPE_METHOD(byte, Byte, B);
    GET_SETPRIM_TYPE_METHOD(int, Integer, I);

    // 使用传入的newArgs参数，而不是硬编码值
    jobject o_c = env->CallStaticObjectMethod(cls_char, meth_valueof_char, newArgs.ch);
    jobject o_f = env->CallStaticObjectMethod(cls_float, meth_valueof_float, newArgs.f);
    jobject o_l = env->CallStaticObjectMethod(cls_long, meth_valueof_long, newArgs.l);
    jobject o_d = env->CallStaticObjectMethod(cls_double, meth_valueof_double, newArgs.d);
    jobject o_s = newArgs.str;  // 字符串直接使用
    jobject o_b = env->CallStaticObjectMethod(cls_byte, meth_valueof_byte, newArgs.b);
    jobject o_n = env->CallStaticObjectMethod(cls_int, meth_valueof_int, newArgs.n);

    env->SetObjectArrayElement(args, 0, o_c);
    env->SetObjectArrayElement(args, 1, o_f);
    env->SetObjectArrayElement(args, 2, o_l);
    env->SetObjectArrayElement(args, 3, o_d);
    env->SetObjectArrayElement(args, 4, o_s);
    env->SetObjectArrayElement(args, 5, o_b);
    env->SetObjectArrayElement(args, 6, o_n);
}
*/

// IHook.cpp
jobject
ArgsConverter::parseReturnValue(JNIEnvExt *g_env, char returnType, uint64_t *xregs, double *fregs,
                                Thread *self) {
    LOGV("[TAG] parseReturnValue开始: returnType=%c, xregs=%p, xregs[0]=0x%lx", returnType, xregs, xregs ? xregs[0] : 0);

    if (g_env == nullptr || xregs == nullptr) {
        LOGV("[TAG] 错误：g_env或xregs为空指针");
        return nullptr;
    }

    switch(returnType) {
        case 'V':  // void
            return nullptr;

        case 'Z': { // boolean
            jclass cls = g_env->FindClass("java/lang/Boolean");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(Z)Ljava/lang/Boolean;");
            jboolean value = (jboolean)(xregs[0] & 0xFF);  // 取低8位
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'B': { // byte
            jclass cls = g_env->FindClass("java/lang/Byte");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(B)Ljava/lang/Byte;");
            jbyte value = (jbyte)(xregs[0] & 0xFF);
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'C': { // char
            jclass cls = g_env->FindClass("java/lang/Character");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(C)Ljava/lang/Character;");
            jchar value = (jchar)(xregs[0] & 0xFFFF);  // 取低16位
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'S': { // short
            jclass cls = g_env->FindClass("java/lang/Short");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(S)Ljava/lang/Short;");
            jshort value = (jshort)(xregs[0] & 0xFFFF);
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'I': { // int
            jclass cls = g_env->FindClass("java/lang/Integer");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(I)Ljava/lang/Integer;");
            jint value = (jint)xregs[0];  // ARM64: w0寄存器（x0的低32位）
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'J': { // long
            jclass cls = g_env->FindClass("java/lang/Long");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(J)Ljava/lang/Long;");
            jlong value = (jlong)xregs[0];  // ARM64: x0寄存器
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'F': { // float
            jclass cls = g_env->FindClass("java/lang/Float");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(F)Ljava/lang/Float;");
            jfloat value = *(jfloat*)fregs;  // ARM64: s0寄存器（d0的低32位）
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'D': { // double
            jclass cls = g_env->FindClass("java/lang/Double");
            jmethodID valueOf = g_env->GetStaticMethodID(cls, "valueOf", "(D)Ljava/lang/Double;");
            jdouble value = *(jdouble*)fregs;  // ARM64: d0寄存器
            return g_env->CallStaticObjectMethod(cls, valueOf, value);
        }

        case 'L':  // 对象引用
        case '[': { // 数组引用
            Object* obj = (Object*)xregs[0];
            if (obj == nullptr) {
                return nullptr;
            }
            // 转换为弱引用
            return g_env->vm->AddWeakGlobalRef(self, obj);
        }

        default:
            LOGV("未知的返回类型: %c", returnType);
            return nullptr;
    }
}

void
ArgsConverter::WriteReturnValue(JNIEnvExt *g_env, char returnType, jobject value, uint64_t *xregs,
                                double *fregs, Thread *self) {
    if (returnType == 'V') return;  // void类型不需要处理

    switch(returnType) {
        case 'Z': { // boolean
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Boolean");
                jmethodID boolValue = g_env->GetMethodID(cls, "booleanValue", "()Z");
                xregs[0] = g_env->CallBooleanMethod(value, boolValue);
            }
            break;
        }

        case 'B': { // byte
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Byte");
                jmethodID byteValue = g_env->GetMethodID(cls, "byteValue", "()B");
                xregs[0] = g_env->CallByteMethod(value, byteValue);
            }
            break;
        }

        case 'C': { // char
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Character");
                jmethodID charValue = g_env->GetMethodID(cls, "charValue", "()C");
                xregs[0] = g_env->CallCharMethod(value, charValue);
            }
            break;
        }

        case 'S': { // short
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Short");
                jmethodID shortValue = g_env->GetMethodID(cls, "shortValue", "()S");
                xregs[0] = g_env->CallShortMethod(value, shortValue);
            }
            break;
        }

        case 'I': { // int
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Integer");
                jmethodID intValue = g_env->GetMethodID(cls, "intValue", "()I");
                xregs[0] = g_env->CallIntMethod(value, intValue);
            }
            break;
        }

        case 'J': { // long
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Long");
                jmethodID longValue = g_env->GetMethodID(cls, "longValue", "()J");
                xregs[0] = g_env->CallLongMethod(value, longValue);
            }
            break;
        }

        case 'F': { // float
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Float");
                jmethodID floatValue = g_env->GetMethodID(cls, "floatValue", "()F");
                *(jfloat*)fregs = g_env->CallFloatMethod(value, floatValue);
            }
            break;
        }

        case 'D': { // double
            if (value) {
                jclass cls = g_env->FindClass("java/lang/Double");
                jmethodID doubleValue = g_env->GetMethodID(cls, "doubleValue", "()D");
                *(jdouble*)fregs = g_env->CallDoubleMethod(value, doubleValue);
            }
            break;
        }

        case 'L':  // 对象
        case '[': { // 数组
            xregs[0] = (uint64_t)self->DecodeJObject(value);
            break;
        }

        default:
            LOGV("未知的返回类型: %c", returnType);
            break;
    }
}
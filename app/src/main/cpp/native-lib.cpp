#include <jni.h>
#include <string>
#include <shadowhook.h>
#include "art.h"
#include "Hook.h"

#define GET_PRIM_TYPE_METHOD(type, jtype, short) \
jclass cls_##type = g_env->FindClass("java/lang/"#jtype);\
jmethodID meth_valueof_##type = g_env->GetStaticMethodID(cls_##type, "valueOf", "("#short")Ljava/lang/"#jtype";");\

#define GET_SETPRIM_TYPE_METHOD(type, jtype, short)\
GET_PRIM_TYPE_METHOD(type, jtype, short)\
jmethodID meth_##type##_value = g_env->GetMethodID(cls_##type, #type"Value", "()"#short);

#define GET_ARG(args, index, ret, jtype)  g_env->Call##ret##Method(g_env->GetObjectArrayElement(args, index),  meth_##jtype##_value)
//jobject b_o = env->GetObjectArrayElement(args, 5);
//jbyte b = env->CallByteMethod(b_o,  meth_byte_value);

JNIEnvExt *g_env = nullptr;
void Before(JNIEnv* env, jobject thiz, jobjectArray args)
{
    //int foo(char ch, float f, long l, double d, String str, Byte b, int n)
    //jclass cls_char = g_env->FindClass("java/lang/Character");

    GET_SETPRIM_TYPE_METHOD(char, Character, C);
    GET_SETPRIM_TYPE_METHOD(float, Float, F);
    GET_SETPRIM_TYPE_METHOD(long , Long, J);
    GET_SETPRIM_TYPE_METHOD(double , Double, D);
    GET_SETPRIM_TYPE_METHOD(byte, Byte, B);
    GET_SETPRIM_TYPE_METHOD(int, Integer, I);

    //(char ch, float f, long l, double d, String str, Byte b, int n)
    jobject ch_o = env->GetObjectArrayElement(args, 0);
    jchar ch = env->CallCharMethod(ch_o,  meth_char_value);

    jobject f_o = env->GetObjectArrayElement(args, 1);
    jfloat f = env->CallFloatMethod(f_o,  meth_float_value);

    jobject l_o = env->GetObjectArrayElement(args, 2);
    jlong l = env->CallLongMethod(l_o,  meth_long_value);

    jobject d_o = env->GetObjectArrayElement(args, 3);
    jdouble d = env->CallDoubleMethod(d_o,  meth_double_value);

    jstring str_o = (jstring )env->GetObjectArrayElement(args, 4);

    jobject b_o = env->GetObjectArrayElement(args, 5);
    jbyte b = env->CallByteMethod(b_o,  meth_byte_value);

    jobject n_o = env->GetObjectArrayElement(args, 6);
    jint n = env->CallIntMethod(n_o,  meth_int_value);

    const char* sz = env->GetStringUTFChars(str_o, nullptr);
    LOGV("before:ch-%c, f-%f, l-%d, d-%lf, str-%s, b-%d, n-%d",
         ch, f, l, d, sz, b, n);

    //(char ch, float f, long l, double d, String str, Byte b, int n)
    jobject o_c = env->CallStaticObjectMethod(cls_char, meth_valueof_char, 'x');
    jobject o_f = env->CallStaticObjectMethod(cls_float, meth_valueof_float, 9.99f);
    jobject o_l = env->CallStaticObjectMethod(cls_long, meth_valueof_long, 9999l);
    jobject o_d = env->CallStaticObjectMethod(cls_double, meth_valueof_double, 99.9);
    jobject o_s = env->NewStringUTF("999999999999999");
    jobject o_b = env->CallStaticObjectMethod(cls_byte, meth_valueof_byte, 9);
    jobject o_n = env->CallStaticObjectMethod(cls_int, meth_valueof_int, 99);

    env->SetObjectArrayElement(args, 0, o_c);
    env->SetObjectArrayElement(args, 1, o_f);
    env->SetObjectArrayElement(args, 2, o_l);
    env->SetObjectArrayElement(args, 3, o_d);
    env->SetObjectArrayElement(args, 4, o_s);
    env->SetObjectArrayElement(args, 5, o_b);
    env->SetObjectArrayElement(args, 6, o_n);

    return;
}

//Bridge
void CallBack(ArtMethod* method,
              Object* thiz,
              Thread* self,
              char* shorty,
              uint32_t* args,
              uint64_t* xregs,
              double * fregs)
{
    std::string  name = method->PrettyMethod();
    LOGV("调用来了 %s", name.c_str());
    g_env->FindClass("java/lang/Float");

    //获取所有需要装箱的基本数据类型的
    /*jclass cls_byte = g_env->FindClass("java/lang/Byte");
    jmethodID meth_valueof_byte = g_env->GetStaticMethodID(cls_byte, "valueOf", "(B)Ljava/lang/Byte;");
    jclass cls_byte = g_env->FindClass("java/lang/Byte");
    jmethodID meth_valueof_byte = g_env->GetStaticMethodID(cls_byte, "valueOf", "(B)Ljava/lang/Byte;");
    jclass cls_byte = g_env->FindClass("java/lang/Byte");
    jmethodID meth_valueof_byte = g_env->GetStaticMethodID(cls_byte, "valueOf", "(B)Ljava/lang/Byte;");
*/
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

    jobject othiz = g_env->vm->AddWeakGlobalRef(self, thiz);
    Before(g_env, othiz, ja);

    //将修改的参数的值存入到栈和寄存器中
    sz = shorty+1; //跳过返回值类型
    args_ptr = args + 1; //跳过this
    idx = 0;
    int idxOfXRegs = 0;
    int idxOfFRegs = 0;
    uint64_t* xregs_ = xregs+2;
    while (*sz != '\0'){
        switch (*sz) {
            case 'B':
            {
                //存入数据
                *(jbyte*)args_ptr = (jbyte)GET_ARG(ja, idx, Byte, byte);

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
                *(jint *)args_ptr  = (jint)GET_ARG(ja, idx, Int, int);

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
                *(jchar*)args_ptr  = (jchar)GET_ARG(ja, idx, Char, char);

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
                *(jboolean*)args_ptr  = (jboolean)GET_ARG(ja, idx, Boolean, boolean);

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
                *(jlong *)args_ptr  = (jlong)GET_ARG(ja, idx, Long, long);

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
                *(uint32_t*)args_ptr  = (uint64_t)self->DecodeJObject(g_env->GetObjectArrayElement(ja, idx));

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
                *(jshort *)args_ptr  = (jshort)GET_ARG(ja, idx, Short, short);

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
                *(jfloat *)args_ptr  = (jfloat)GET_ARG(ja, idx, Float, float);

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
                *(jdouble *)args_ptr  = (jdouble)GET_ARG(ja, idx, Double, double );

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

    return ;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_hook(JNIEnv *env, jobject thiz) {

//    jclass cls_char = env->FindClass("java/lang/Character");
//    env->GetMethodID()

    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);
    g_env = extenv;

    //hook CFoo.foo
    jclass  cls = env->FindClass("com/kr/test/CFoo");
    ArtMethod* pfnfoo = (ArtMethod*)env->GetMethodID(cls, "foo", "(CFJDLjava/lang/String;BI)I");

    LOGV("准备hook foo函数 : %p", pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_);

    //编译
    if (pfnfoo->IsUsInterpreter()){
        if (!Jit::CompileMethod(pfnfoo, extenv->self)){
            LOGV("准备hook foo函数 编译失败: %p", pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_);
        } else{
            LOGV("准备hook foo函数 编译成功: %p", pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_);
        }
    }

    //hook
    if (!InstallHook(pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_, (void*)CallBack)){
        LOGV("失败hook foo函数 ");
    } else{
        LOGV("成功hook foo函数 ");
    }

}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kr_test_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

__attribute__((constructor())) void lib_init()
{
    shadowhook_init(SHADOWHOOK_MODE_UNIQUE, true);
    if(!InitArt()){
        LOGV("初始化失败");
        return;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_showFieldsMethods(JNIEnv *env, jobject thiz) {

    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);

    //获取Class
    jclass cls = env->FindClass("com/kr/test/CFoo");
    MyClass* c = (MyClass*)extenv->self->DecodeJObject(cls);
    LOGV("class:%p", c);

    LOGV("************静态字段*********");
    auto sfields = c->sfields_;
    for (int i = 0; i <sfields->size_; ++i) {
        auto name = sfields->data[i].PrettyField();
        LOGV("class:%08X flag:%08X idx:%d offset:%08X name:%s",
             sfields->data[i].declaring_class_.root_.reference_,
             sfields->data[i].access_flags_,
             sfields->data[i].field_dex_idx_,
             sfields->data[i].offset_,
             name.c_str()
             );
    }

    LOGV("************实例字段*********");
    auto ifields = c->ifields_;
    for (int i = 0; i <sfields->size_; ++i) {
        auto name = ifields->data[i].PrettyField();
        LOGV("class:%08X flag:%08X idx:%d offset:%08X name:%s",
             ifields->data[i].declaring_class_.root_.reference_,
             ifields->data[i].access_flags_,
             ifields->data[i].field_dex_idx_,
             ifields->data[i].offset_,
             name.c_str()
        );
    }

    LOGV("************遍历方法*********");
    auto methods = c->methods_;
    for (int i = 0; i < methods->size_; ++i) {
        auto name = methods->data[i].PrettyMethod();
        LOGV("calss:%08X flag:%08X offset:%08X dex_index:%d index:%d hot:%d entry:%p data:%p name:%s",
             methods->data[i].declaring_class_.root_.reference_,
             methods->data[i].access_flags_.load(),
             methods->data[i].dex_code_item_offset_,
             methods->data[i].dex_method_index_,
             methods->data[i].method_index_,
             methods->data[i].hotness_count_,
             methods->data[i].ptr_sized_fields_.entry_point_from_quick_compiled_code_,
             methods->data[i].ptr_sized_fields_.data_,
             name.c_str());
    }

    env->GetMethodID(cls, "show", "()V");

}
extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_updateFields(JNIEnv *env, jobject thiz, jobject obj) {

    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);

    //获取Class
    jclass cls = env->FindClass("com/kr/test/CFoo");
    MyClass* c = (MyClass*)extenv->self->DecodeJObject(cls);
    LOGV("class:%p", c);

    //获取Object
    Object* o = (Object*)extenv->self->DecodeJObject(obj);
    LOGV("Object:%p", o);

    uint32_t f1_off = 0xEC;
    uint32_t n1_off = 0xE8;
    uint32_t n_off = 0x8;
    uint32_t f_off = 0xc;

    *(uint32_t*)((uint8_t*)c+n1_off) = 9999;
    *(float *)((uint8_t*)c+f1_off) = 99.99;

    *(uint32_t*)((uint8_t*)o+n_off) = 8888;
    *(float *)((uint8_t*)o+f_off) = 88.88;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_compileMethod(JNIEnv *env, jobject thiz) {
    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);

    //获取Class
    jclass cls = env->FindClass("com/kr/test/CFoo");
    MyClass* c = (MyClass*)extenv->self->DecodeJObject(cls);

    LOGV("************编译方法*********");
    auto methods = c->methods_;
    for (int i = 0; i < methods->size_; ++i) {
        auto name = methods->data[i].PrettyMethod();
        if(!Jit::CompileMethod(&methods->data[i], extenv->self)){
            LOGV("编译方法：%s 失败", name.c_str());
        } else{
            LOGV("编译方法：%s 成功", name.c_str());
        }

    }
}

void JNICALL testNative(JNIEnv *env, jobject thiz)
 {
    LOGV("native");
    return ;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_RegMethod(JNIEnv *env, jobject thiz) {
    jclass cls = env->FindClass("com/kr/test/CFoo");

    JNINativeMethod m[] = {
            "testNative", "()V", (void*) testNative
    };
    env->RegisterNatives(cls, m, 1);
    LOGV("注册testNative:%p", testNative);
}

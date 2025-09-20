#include <jni.h>
#include <string>
#include <shadowhook.h>
#include "art.h"
#include "IHook.h"
#include "ArgsConverter.h"

class MyHook :public IHook{
public:
    void onBeforeMethod(JNIEnv* env,ArtMethod* pfnMethod, jobject thiz, jobjectArray& args) override  {
        std::string sig = pfnMethod->PrettyMethod();
        if (sig.find("foo(") != std::string::npos) {
            LOGV("MyHook onBeforeMethod");
        }
    }

    jobject onAfterMethod(JNIEnv* env,ArtMethod* pfnMethod,jobject thiz, jobject returnValue) override {
        // 修改返回值逻辑
        LOGV("MyHook onAfterMethod");
        return returnValue;
    }
};

extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_hook(JNIEnv *env, jobject thiz) {

//    jclass cls_char = env->FindClass("java/lang/Character");
//    env->GetMethodID()


    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);
    IHook::InitEnv(env);
    static MyHook myHook;  // 使用static确保生命周期
    IHook::SetInstance(&myHook);

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
    if (!myHook.InstallMethodHook(pfnfoo)){
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

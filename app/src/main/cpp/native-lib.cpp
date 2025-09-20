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

    LOGV("[TAG] 开始执行hook函数");

//    jclass cls_char = env->FindClass("java/lang/Character");
//    env->GetMethodID()

    LOGV("[TAG] 转换JNIEnv");
    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);
    
    LOGV("[TAG] 初始化Hook环境");
    IHook::InitEnv(env);
    
    LOGV("[TAG] 创建Hook实例");
    static MyHook myHook;  // 使用static确保生命周期
    IHook::SetInstance(&myHook);

    LOGV("[TAG] 查找CFoo类");
    //hook CFoo.foo
    jclass  cls = env->FindClass("com/kr/test/CFoo");
    if (cls == nullptr) {
        LOGV("[TAG] 错误：找不到CFoo类");
        return;
    }
    
    LOGV("[TAG] 获取foo方法ID");
    ArtMethod* pfnfoo = (ArtMethod*)env->GetMethodID(cls, "foo", "(CFJDLjava/lang/String;BI)I");
    if (pfnfoo == nullptr) {
        LOGV("[TAG] 错误：找不到foo方法");
        return;
    }

    LOGV("[TAG] 准备hook foo函数 : %p", pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_);

    LOGV("[TAG] 检查是否需要编译");
    //编译
    if (pfnfoo->IsUsInterpreter()){
        LOGV("[TAG] 方法使用解释器，开始编译");
        if (!Jit::CompileMethod(pfnfoo, extenv->self)){
            LOGV("[TAG] 编译失败: %p", pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_);
        } else{
            LOGV("[TAG] 编译成功: %p", pfnfoo->ptr_sized_fields_.entry_point_from_quick_compiled_code_);
        }
    } else {
        LOGV("[TAG] 方法已编译，跳过编译步骤");
    }

    LOGV("[TAG] 开始安装Hook");
    //hook
    if (!myHook.InstallMethodHook(pfnfoo)){
        LOGV("[TAG] Hook安装失败");
    } else{
        LOGV("[TAG] Hook安装成功");
    }

    LOGV("[TAG] hook函数执行完成");
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

    LOGV("[TAG] 开始执行showFieldsMethods函数");

    LOGV("[TAG] 转换JNIEnv");
    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);

    LOGV("[TAG] 获取CFoo类");
    //获取Class
    jclass cls = env->FindClass("com/kr/test/CFoo");
    if (cls == nullptr) {
        LOGV("[TAG] 错误：找不到CFoo类");
        return;
    }

    LOGV("[TAG] 解码Class对象");
    MyClass* c = (MyClass*)extenv->self->DecodeJObject(cls);
    if (c == nullptr) {
        LOGV("[TAG] 错误：Class对象解码失败");
        return;
    }
    LOGV("[TAG] class:%p", c);

    LOGV("[TAG] ************静态字段*********");
    auto sfields = c->sfields_;
    if (sfields == nullptr) {
        LOGV("[TAG] 错误：静态字段为空");
        return;
    }
    LOGV("[TAG] 静态字段数量: %d", sfields->size_);
    for (int i = 0; i <sfields->size_; ++i) {
        auto name = sfields->data[i].PrettyField();
        LOGV("[TAG] 静态字段[%d] class:%08X flag:%08X idx:%d offset:%08X name:%s",
             i,
             sfields->data[i].declaring_class_.root_.reference_,
             sfields->data[i].access_flags_,
             sfields->data[i].field_dex_idx_,
             sfields->data[i].offset_,
             name.c_str()
             );
    }

    LOGV("[TAG] ************实例字段*********");
    auto ifields = c->ifields_;
    if (ifields == nullptr) {
        LOGV("[TAG] 错误：实例字段为空");
        return;
    }
    LOGV("[TAG] 实例字段数量: %d", ifields->size_);
    for (int i = 0; i <ifields->size_; ++i) {
        auto name = ifields->data[i].PrettyField();
        LOGV("[TAG] 实例字段[%d] class:%08X flag:%08X idx:%d offset:%08X name:%s",
             i,
             ifields->data[i].declaring_class_.root_.reference_,
             ifields->data[i].access_flags_,
             ifields->data[i].field_dex_idx_,
             ifields->data[i].offset_,
             name.c_str()
        );
    }

    LOGV("[TAG] ************遍历方法*********");
    auto methods = c->methods_;
    if (methods == nullptr) {
        LOGV("[TAG] 错误：方法列表为空");
        return;
    }
    LOGV("[TAG] 方法数量: %d", methods->size_);
    for (int i = 0; i < methods->size_; ++i) {
        auto name = methods->data[i].PrettyMethod();
        LOGV("[TAG] 方法[%d] class:%08X flag:%08X offset:%08X dex_index:%d index:%d hot:%d entry:%p data:%p name:%s",
             i,
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

    LOGV("[TAG] 获取show方法ID");
    jmethodID showMethod = env->GetMethodID(cls, "show", "()V");
    if (showMethod == nullptr) {
        LOGV("[TAG] 错误：找不到show方法");
    } else {
        LOGV("[TAG] show方法获取成功: %p", showMethod);
    }

    LOGV("[TAG] showFieldsMethods函数执行完成");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_updateFields(JNIEnv *env, jobject thiz, jobject obj) {

    LOGV("[TAG] 开始执行updateFields函数");

    LOGV("[TAG] 转换JNIEnv");
    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);

    LOGV("[TAG] 获取CFoo类");
    //获取Class
    jclass cls = env->FindClass("com/kr/test/CFoo");
    if (cls == nullptr) {
        LOGV("[TAG] 错误：找不到CFoo类");
        return;
    }

    LOGV("[TAG] 解码Class对象");
    MyClass* c = (MyClass*)extenv->self->DecodeJObject(cls);
    if (c == nullptr) {
        LOGV("[TAG] 错误：Class对象解码失败");
        return;
    }
    LOGV("[TAG] class:%p", c);

    LOGV("[TAG] 解码Object对象");
    //获取Object
    Object* o = (Object*)extenv->self->DecodeJObject(obj);
    if (o == nullptr) {
        LOGV("[TAG] 错误：Object对象解码失败");
        return;
    }
    LOGV("[TAG] Object:%p", o);

    LOGV("[TAG] 设置内存偏移量");
    uint32_t f1_off = 0xEC;
    uint32_t n1_off = 0xE8;
    uint32_t n_off = 0x8;
    uint32_t f_off = 0xc;

    LOGV("[TAG] 修改Class字段值");
    *(uint32_t*)((uint8_t*)c+n1_off) = 9999;
    *(float *)((uint8_t*)c+f1_off) = 99.99;

    LOGV("[TAG] 修改Object字段值");
    *(uint32_t*)((uint8_t*)o+n_off) = 8888;
    *(float *)((uint8_t*)o+f_off) = 88.88;

    LOGV("[TAG] updateFields函数执行完成");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_compileMethod(JNIEnv *env, jobject thiz) {
    LOGV("[TAG] 开始执行compileMethod函数");

    LOGV("[TAG] 转换JNIEnv");
    JNIEnvExt* extenv = static_cast<JNIEnvExt *>(env);

    LOGV("[TAG] 获取CFoo类");
    //获取Class
    jclass cls = env->FindClass("com/kr/test/CFoo");
    if (cls == nullptr) {
        LOGV("[TAG] 错误：找不到CFoo类");
        return;
    }

    LOGV("[TAG] 解码Class对象");
    MyClass* c = (MyClass*)extenv->self->DecodeJObject(cls);
    if (c == nullptr) {
        LOGV("[TAG] 错误：Class对象解码失败");
        return;
    }

    LOGV("[TAG] ************编译方法*********");
    auto methods = c->methods_;
    if (methods == nullptr) {
        LOGV("[TAG] 错误：方法列表为空");
        return;
    }
    LOGV("[TAG] 方法数量: %d", methods->size_);
    for (int i = 0; i < methods->size_; ++i) {
        auto name = methods->data[i].PrettyMethod();
        LOGV("[TAG] 开始编译方法[%d]: %s", i, name.c_str());
        if(!Jit::CompileMethod(&methods->data[i], extenv->self)){
            LOGV("[TAG] 编译方法：%s 失败", name.c_str());
        } else{
            LOGV("[TAG] 编译方法：%s 成功", name.c_str());
        }
    }

    LOGV("[TAG] compileMethod函数执行完成");
}

void JNICALL testNative(JNIEnv *env, jobject thiz)
 {
    LOGV("native");
    return ;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kr_test_MainActivity_RegMethod(JNIEnv *env, jobject thiz) {
    LOGV("[TAG] 开始执行RegMethod函数");

    LOGV("[TAG] 获取CFoo类");
    jclass cls = env->FindClass("com/kr/test/CFoo");
    if (cls == nullptr) {
        LOGV("[TAG] 错误：找不到CFoo类");
        return;
    }

    LOGV("[TAG] 准备注册native方法");
    JNINativeMethod m[] = {
            "testNative", "()V", (void*) testNative
    };

    LOGV("[TAG] 注册testNative方法");
    int result = env->RegisterNatives(cls, m, 1);
    if (result != JNI_OK) {
        LOGV("[TAG] 错误：注册testNative方法失败，错误码: %d", result);
    } else {
        LOGV("[TAG] 注册testNative方法成功: %p", testNative);
    }

    LOGV("[TAG] RegMethod函数执行完成");
}

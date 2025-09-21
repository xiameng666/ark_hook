#include <jni.h>
#include "art.h"
#include "ArgsConverter.h"
#include <cstring>
#include <sys/mman.h>
#include <cstdio>
#include <map>

enum HookType{
    TypeBefore,
    TypeAfter
};

class IHook{
public:
    static JNIEnvExt* env_;
    static IHook* g_hookInstance; //管理所有hook实例

    static void InitEnv(JNIEnv* env) {
        env_ = static_cast<JNIEnvExt*>(env);
    }

    static void SetInstance(IHook* instance) {
        g_hookInstance = instance;
    }

    // 合并后的接口：传入方法和回调函数（都可选）
    bool InstallMethodHook(ArtMethod* method, void* beforeCallback = nullptr, void* afterCallback = nullptr);

    bool UninstallHook(void* pDestAddr, void* pfnNewDstAddr);

    void* FindRetInst(void* pDestAddr);

    virtual void onBeforeMethod(JNIEnv* env,ArtMethod* pfnMethod, jobject thiz, jobjectArray& args) = 0;
    virtual jobject onAfterMethod(JNIEnv* env,ArtMethod* pfnMethod,jobject thiz, jobject returnValue) = 0;

    static void AfterCallBack(ArtMethod *method, Object *thiz, Thread *self, char *shorty, uint32_t *args,
                       uint64_t *xregs, double *fregs);

    static void BeforeCallBack(ArtMethod *method, Object *thiz, Thread *self, char *shorty, uint32_t *args,
                        uint64_t *xregs, double *fregs);

private:
    // 内部使用的安装函数
    bool InstallHook(void* pDestAddr, void* beforeCallback, void* afterCallback);
};
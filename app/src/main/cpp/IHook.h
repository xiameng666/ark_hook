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

    bool InstallHook(void* pDestAddr, void* pfnCallback);

    bool UninstallHook(void* pDestAddr, void* pfnNewDstAddr);

    void* FindRetInst(void* pDestAddr);

//    virtual void After(JNIEnv *env, jobject thiz, jobjectArray args);
//
//    virtual void Before(JNIEnv *env, jobject thiz, jobjectArray args);

    virtual void onBeforeMethod(JNIEnv* env,ArtMethod* pfnMethod, jobject thiz, jobjectArray& args) = 0;
    virtual jobject onAfterMethod(JNIEnv* env,ArtMethod* pfnMethod,jobject thiz, jobject returnValue) = 0;

    static void AfterCallBack(ArtMethod *method, Object *thiz, Thread *self, char *shorty, uint32_t *args,
                       uint64_t *xregs, double *fregs);

    static void BeforeCallBack(ArtMethod *method, Object *thiz, Thread *self, char *shorty, uint32_t *args,
                        uint64_t *xregs, double *fregs);

    bool InstallMethodHook(ArtMethod *method);
};
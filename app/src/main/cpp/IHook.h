#include <jni.h>
#include "art.h"
#include "ArgsConverter.h"
#include <cstring>
#include <sys/mman.h>
#include <cstdio>

enum HookType{
    TypeBefore,
    TypeAfter
};

class IHook{
private:

public:
    bool InstallHook(void* pDestAddr, void* pfnCallback);

    bool UninstallHook(void* pDestAddr, void* pfnNewDstAddr);

    void* FindRetInst(void* pDestAddr);

    virtual void After(JNIEnv *env, jobject thiz, jobjectArray args);

    virtual void Before(JNIEnv *env, jobject thiz, jobjectArray args);

    void CallBack(ArtMethod *method, Object *thiz, Thread *self, char *shorty, uint32_t *args,
                  uint64_t *xregs, double *fregs);

    virtual void ChangeArgs();
    virtual void ChangeValue();
};
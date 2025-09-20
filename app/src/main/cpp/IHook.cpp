#include "IHook.h"


#define SMALL_TRIMPLINE 16
#define PAGE_SZIE 0x1000
#define PAGE_BASE(x) (void*)((uint64_t)(x) & (~0xFFF))

IHook* IHook::g_hookInstance = nullptr;
JNIEnvExt* IHook::env_ = nullptr;

extern "C"{
    void Saved_OldCode();
    void RetDst_addr();
    void SmallTramplie();
    void Trimpline();
    void g_pfnCallback();
}


bool IHook::InstallMethodHook(ArtMethod* method)
{
    void* methodEntry = method->ptr_sized_fields_.entry_point_from_quick_compiled_code_;
    if (!InstallHook(methodEntry, (void*)BeforeCallBack)){
        return false;
    }

    //查找并安装After Hook
    void* retAddr = FindRetInst(methodEntry);
    if (retAddr) {
        if (!InstallHook(retAddr, (void*)AfterCallBack)) {
            LOGV("After Hook安装失败 !InstallHook");
        }
    }else{
        LOGV("After Hook安装失败 !retAddr");
    }

    return true;
}

bool IHook::InstallHook(void* pDestAddr, void* pfnCallback)
{
    //0. 修改内存属性
    int nRet = mprotect(PAGE_BASE(pDestAddr), PAGE_SZIE, PROT_READ|PROT_WRITE|PROT_EXEC);
    if (nRet < 0)
    {
        perror("修改目标函数内存属性失败");
        return false;
    }

    nRet = mprotect(PAGE_BASE(Saved_OldCode), PAGE_SZIE*2, PROT_READ|PROT_WRITE|PROT_EXEC);
    if (nRet < 0)
    {
        perror("修改跳板内存属性失败");
        return false;
    }
    
    //1. 保存原函数指令
    memcpy((void*)Saved_OldCode, (void*)pDestAddr, SMALL_TRIMPLINE);
    auto pRetAddr = (char*)pDestAddr+SMALL_TRIMPLINE;
    memcpy((void*)RetDst_addr, (void*)&pRetAddr, sizeof(pDestAddr));
    memcpy((void*)g_pfnCallback, (void*)&pfnCallback, sizeof(pfnCallback));

    //2. 修改原函数头，跳转到跳板
    memcpy((void*)pDestAddr, (void*)SmallTramplie, SMALL_TRIMPLINE);
    void* pTrimpline = (void*)Trimpline;
    memcpy((void*)((char*)pDestAddr+8), (void*)&pTrimpline, sizeof(pTrimpline));

    return true;
}

bool IHook::UninstallHook(void* pDestAddr, void* pfnNewDstAddr)
{
    return false;
}

void* IHook::FindRetInst(void* pDestAddr) {
    size_t next = 0;

    // RET指令的机器码（ARM64）
    const uint32_t RET_INSTRUCTION_1 = 0xD65F03C0;  // ret
    const uint32_t RET_INSTRUCTION_2 = 0xC0035FD6;  // ret x30

    const size_t MAX_SCAN_SIZE = 4096;  // 4KB扫描范围

    while (next < MAX_SCAN_SIZE) {
        // 获取当前4字节的机器码  会不会是小端序？
        uint32_t instruction = *(uint32_t*)((char*)pDestAddr + next);

        // 检查是否是RET指令
        if (instruction == RET_INSTRUCTION_1 || instruction == RET_INSTRUCTION_2) {
            return (char*)pDestAddr + next;
        }

        next += 4;
    }

    return nullptr;
}

void IHook::AfterCallBack(ArtMethod* method,
                           Object* thiz,
                           Thread* self,
                           char* shorty,
                           uint32_t* args,
                           uint64_t* xregs,
                           double * fregs)
{
    std::string  name = method->PrettyMethod();
    LOGV("AfterCallBack来了 %s", name.c_str());

    jobjectArray ja = nullptr;
    jobject othiz = env_->vm->AddWeakGlobalRef(self, thiz);
    jobject retValue = ArgsConverter::parseReturnValue(env_, shorty[0], xregs, fregs, self);

    //用户重写
    jobject newValue = g_hookInstance->onAfterMethod(env_, method, othiz, retValue);

    ArgsConverter::WriteReturnValue(env_, shorty[0], newValue, xregs, fregs, self);
}

//Bridge
void IHook::BeforeCallBack(ArtMethod* method,
                     Object* thiz,
                     Thread* self,
                     char* shorty,
                     uint32_t* args,
                     uint64_t* xregs,
                     double * fregs)
{
    std::string  name = method->PrettyMethod();
    LOGV("BeforeCallBack来了 %s", name.c_str());

    jobjectArray ja = nullptr;
    jobject othiz = env_->vm->AddWeakGlobalRef(self, thiz);

    //ART参数 → Java对象数组
    ja = ArgsConverter::artArgs2JArray(env_, method, thiz, self, shorty, args, xregs, fregs);

    //用户重写
    g_hookInstance->onBeforeMethod(env_, method, othiz, ja);

    //Java对象数组 → ART参数
    ArgsConverter::JArray2ARTArgs(env_, ja, shorty, args, xregs, fregs, self);

    return ;
}


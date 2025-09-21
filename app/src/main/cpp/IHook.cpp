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

    void AfterHookTrampoline();
    void g_pfnAfterCallback();
    void AfterHook_addr();
}


bool IHook::InstallMethodHook(ArtMethod* method)
{
    LOGV("[TAG] 开始安装方法Hook");
    
    LOGV("[TAG] 获取方法入口点");
    void* methodEntry = method->ptr_sized_fields_.entry_point_from_quick_compiled_code_;
    if (methodEntry == nullptr) {
        LOGV("[TAG] 错误：方法入口点为空");
        return false;
    }
    LOGV("[TAG] 方法入口点: %p", methodEntry);

    LOGV("[TAG] 安装Before Hook");
    if (!InstallHook(methodEntry, (void*)BeforeCallBack)){
        LOGV("[TAG] 错误：Before Hook安装失败");
        return false;
    }
    LOGV("[TAG] Before Hook安装成功");
//
//    LOGV("[TAG] 查找返回指令地址");
//    //查找并安装After Hook
//    void* retAddr = FindRetInst(methodEntry);
//    if (retAddr) {
//        LOGV("[TAG] 找到返回指令地址: %p", retAddr);
//        LOGV("[TAG] 安装After Hook");
//        if (!InstallHook(retAddr, (void*)AfterCallBack)) {
//            LOGV("[TAG] 错误：After Hook安装失败");
//        } else {
//            LOGV("[TAG] After Hook安装成功");
//        }
//    }else{
//        LOGV("[TAG] 错误：找不到返回指令地址");
//    }
    return true;
}

bool IHook::InstallHook(void* pDestAddr, void* pfnCallback)
{
    // 打印所有汇编变量的地址
    LOGV("[TAG] ========== 汇编变量地址信息 ==========");
    LOGV("[TAG] Saved_OldCode 地址: %p", (void*)Saved_OldCode);
    LOGV("[TAG] RetDst_addr 地址: %p", (void*)RetDst_addr);
    LOGV("[TAG] SmallTramplie 地址: %p", (void*)SmallTramplie);
    LOGV("[TAG] Trimpline 地址: %p", (void*)Trimpline);
    LOGV("[TAG] g_pfnCallback 地址: %p", (void*)g_pfnCallback);
    LOGV("[TAG] AfterHookTrampoline 地址: %p", (void*)AfterHookTrampoline);
    LOGV("[TAG] g_pfnAfterCallback 地址: %p", (void*)g_pfnAfterCallback);
    LOGV("[TAG] AfterHook_addr 地址: %p", (void*)AfterHook_addr);
    LOGV("[TAG] =====================================");

    LOGV("[TAG] 修改目标函数内存属性");
    //0. 修改内存属性
    int nRet = mprotect(PAGE_BASE(pDestAddr), PAGE_SZIE, PROT_READ|PROT_WRITE|PROT_EXEC);
    if (nRet < 0)
    {
        LOGV("[TAG] 错误：修改目标函数内存属性失败");
        perror("修改目标函数内存属性失败");
        return false;
    }
    LOGV("[TAG] 目标函数内存属性修改成功");

    LOGV("[TAG] 修改跳板内存属性");
    nRet = mprotect(PAGE_BASE(Saved_OldCode), PAGE_SZIE*2, PROT_READ|PROT_WRITE|PROT_EXEC);
    if (nRet < 0)
    {
        LOGV("[TAG] 错误：修改跳板内存属性失败");
        perror("修改跳板内存属性失败");
        return false;
    }
    LOGV("[TAG] 跳板内存属性修改成功");
    
    LOGV("[TAG] 保存原函数指令");
    //1. 保存原函数指令
    memcpy((void*)Saved_OldCode, (void*)pDestAddr, SMALL_TRIMPLINE);
    auto pRetAddr = (char*)pDestAddr+SMALL_TRIMPLINE;
    memcpy((void*)RetDst_addr, (void*)&pRetAddr, sizeof(pDestAddr));
    memcpy((void*)g_pfnCallback, (void*)&pfnCallback, sizeof(pfnCallback));

    // 初始化After Hook回调函数地址
    void* pAfterCallback = (void*)AfterCallBack;
    memcpy((void*)g_pfnAfterCallback, (void*)&pAfterCallback, sizeof(pAfterCallback));

    // 初始化After Hook跳板地址
    void* pAfterHookTrampoline = (void*)AfterHookTrampoline;
    memcpy((void*)AfterHook_addr, (void*)&pAfterHookTrampoline, sizeof(pAfterHookTrampoline));
    LOGV("[TAG] 原函数指令保存完成");

    LOGV("[TAG] 修改原函数头，跳转到跳板");
    //2. 修改原函数头，跳转到跳板
    memcpy((void*)pDestAddr, (void*)SmallTramplie, SMALL_TRIMPLINE);
    void* pTrimpline = (void*)Trimpline;
    memcpy((void*)((char*)pDestAddr+8), (void*)&pTrimpline, sizeof(pTrimpline));
    LOGV("[TAG] 跳转指令设置完成");

    LOGV("[TAG] Hook安装成功");
    return true;
}

bool IHook::UninstallHook(void* pDestAddr, void* pfnNewDstAddr)
{
    return false;
}

void* IHook::FindRetInst(void* pDestAddr) {
    LOGV("[TAG] 开始查找返回指令，起始地址: %p", pDestAddr);
    
    size_t next = 0;

    // RET指令的机器码（ARM64）
    const uint32_t RET_INSTRUCTION_1 = 0xD65F03C0;  // ret
    const uint32_t RET_INSTRUCTION_2 = 0xC0035FD6;  // ret x30

    const size_t MAX_SCAN_SIZE = 4096;  // 4KB扫描范围
    LOGV("[TAG] 最大扫描范围: %zu 字节", MAX_SCAN_SIZE);

    while (next < MAX_SCAN_SIZE) {
        // 获取当前4字节的机器码  会不会是小端序？
        uint32_t instruction = *(uint32_t*)((char*)pDestAddr + next);
        
        if (next % 16 == 0) {  // 每16字节打印一次进度
            LOGV("[TAG] 扫描进度: %zu/%zu, 当前指令: 0x%08X", next, MAX_SCAN_SIZE, instruction);
        }

        // 检查是否是RET指令
        if (instruction == RET_INSTRUCTION_1 || instruction == RET_INSTRUCTION_2) {
            LOGV("[TAG] 找到返回指令，地址: %p, 偏移: %zu", (char*)pDestAddr + next, next);
            return (char*)pDestAddr + next;
        }

        next += 4;
    }

    LOGV("[TAG] 错误：在扫描范围内未找到返回指令");
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
    LOGV("[TAG] AfterCallBack来了 %s", name.c_str());

    LOGV("[TAG] 处理AfterCallBack参数");

    LOGV("[TAG] 添加弱全局引用");
    jobject othiz = nullptr;
    if (thiz != nullptr) {
        othiz = env_->vm->AddWeakGlobalRef(self, thiz);
        if (othiz == nullptr) {
            LOGV("[TAG] 错误：添加弱全局引用失败");
            return;
        }
    }

    LOGV("[TAG] 解析返回值");
    // 注意：这里的xregs和fregs现在包含的是返回值，不是参数
    // 需要根据方法的返回类型来解析返回值
    char returnType = shorty[0]; // shorty第一个字符是返回类型

    LOGV("[TAG] 返回类型: %c, xregs[0]: 0x%lx, xregs[1]: 0x%lx", returnType, xregs[0], xregs[1]);
    LOGV("[TAG] xregs地址: %p, fregs地址: %p", xregs, fregs);

    jobject retValue = ArgsConverter::parseReturnValue(env_, returnType, xregs, fregs, self);
    if (retValue == nullptr && returnType != 'V') { // V表示void返回类型
        LOGV("[TAG] 警告：返回值解析失败，返回类型: %c", returnType);
    }

    LOGV("[TAG] 调用用户重写的onAfterMethod");
    //用户重写
    jobject newValue = g_hookInstance->onAfterMethod(env_, method, othiz, retValue);

    LOGV("[TAG] 写入新的返回值");
    if (newValue != nullptr) {
        ArgsConverter::WriteReturnValue(env_, returnType, newValue, xregs, fregs, self);
    }

    LOGV("[TAG] AfterCallBack处理完成");
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
    LOGV("[TAG] BeforeCallBack来了 %s", name.c_str());

    LOGV("[TAG] 处理BeforeCallBack参数");
    jobjectArray ja = nullptr;
    
    LOGV("[TAG] 添加弱全局引用");
    jobject othiz = env_->vm->AddWeakGlobalRef(self, thiz);
    if (othiz == nullptr) {
        LOGV("[TAG] 错误：添加弱全局引用失败");
        return;
    }

    LOGV("[TAG] ART参数转换为Java对象数组");
    //ART参数 → Java对象数组
    ja = ArgsConverter::artArgs2JArray(env_, method, thiz, self, shorty, args, xregs, fregs);
    if (ja == nullptr) {
        LOGV("[TAG] 错误：ART参数转换失败");
        return;
    }

    LOGV("[TAG] 调用用户重写的onBeforeMethod");
    //用户重写
    g_hookInstance->onBeforeMethod(env_, method, othiz, ja);

    LOGV("[TAG] Java对象数组转换为ART参数");
    //Java对象数组 → ART参数
    ArgsConverter::JArray2ARTArgs(env_, ja, shorty, args, xregs, fregs, self);

    LOGV("[TAG] BeforeCallBack处理完成");
    return ;
}


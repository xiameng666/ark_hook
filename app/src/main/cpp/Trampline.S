.arch armv8-a

.global Saved_OldCode
.global RetDst_addr
.global SmallTramplie
.global Trimpline

.global g_pfnCallback
.global g_pfnAfterCallback
.global AfterHookTrampoline
.global AfterHook_addr

.text
Trimpline:
    // 首先保存x20,x21，因为我们要使用它们
    sub sp, sp, 0x20
    stp x20, x21, [sp]

    //保存所有需要的寄存器
    adr x20, g_xregs
    stp x0, x1, [x20], 0x10
    stp x2, x3, [x20], 0x10
    stp x4, x5, [x20], 0x10
    stp x6, x7, [x20], 0x10
    stp x8, x9, [x20], 0x10
    stp x10, x11, [x20], 0x10
    stp x12, x13, [x20], 0x10
    stp x14, x15, [x20], 0x10

    adr x20, g_fregs
    stp d0, d1, [x20], 0x10
    stp d2, d3, [x20], 0x10
    stp d4, d5, [x20], 0x10
    stp d6, d7, [x20], 0x10

    // After Hook: 在数据区保存信息并修改返回地址
    adr x20, g_afterhook_info
    str x0, [x20]           // 保存ArtMethod*
    str x1, [x20, #8]       // 保存this指针
    str x30, [x20, #16]     // 直接保存x30(原始返回地址)到数据区
    ldr x21, [x29, 0x18]    // 获取shorty
    str x21, [x20, #24]     // 保存shorty指针

    // 修改返回地址为After Hook跳板
    adr x21, AfterHook_addr_addr
    ldr x21, [x21]          // x21 = AfterHookTrampoline地址

    sub sp, sp, 0x10
    stp x29, x21, [sp]      // 保存x29和修改后的返回地址(AfterHookTrampoline)

    //准备参数
// void CallBack(ArtMethod* method,
//               Object* thiz,
//               Thread* self,
//               char* shorty,
//               uint32_t* args,
//               uint64_t* xregs,
//               double * fregs);

    //X0->ArtMethod*
    //X1->this
    mov x2, x19         //Thread*
    ldr x3, [x29, 0x18]  //shorty
    add x4, sp, 0x18          //栈上的参数
    adr x5, g_xregs     //整型寄存器数组
    adr x6, g_fregs     //浮点寄存器数组

    //调用C函数
    ldr x17, g_pfnCallback_addr
    blr x17

    ldp x29, x30, [sp]  // 这里恢复的x30已经被修改为AfterHookTrampoline地址
    add sp, sp, 0x10

    //恢复所有的寄存器
    adr x20, g_xregs
    ldp x0, x1, [x20], 0x10
    ldp x2, x3, [x20], 0x10
    ldp x4, x5, [x20], 0x10
    ldp x6, x7, [x20], 0x10
    ldp x8, x9, [x20], 0x10
    ldp x10, x11, [x20], 0x10
    ldp x12, x13, [x20], 0x10
    ldp x14, x15, [x20], 0x10

    adr x20, g_fregs
    ldp d0, d1, [x20], 0x10
    ldp d2, d3, [x20], 0x10
    ldp d4, d5, [x20], 0x10
    ldp d6, d7, [x20], 0x10

    // 恢复x20,x21
    ldp x20, x21, [sp]
    add sp, sp, 0x20

Saved_OldCode: // 保存原函数的指令
    .space 0x10, 0

SmallTramplie:
Trampline_BrDst:
    ldr x17, RetDst_addr_addr
    br x17
RetDst_addr:
RetDst_addr_addr:
    .space 8, 0

AfterHookTrampoline:
    // 保存返回值到数据区
    adr x20, g_ret_xregs
    stp x0, x1, [x20]        // 保存返回值 x0, x1
    stp x2, x3, [x20, #16]   // 保存返回值 x2, x3

    adr x20, g_ret_fregs
    stp d0, d1, [x20]        // 保存返回值 d0, d1
    stp d2, d3, [x20, #16]   // 保存返回值 d2, d3

    // 保存栈指针
    sub sp, sp, 0x10
    stp x29, x30, [sp]

    // 准备After回调的参数，使用Before Hook时保存的信息
    adr x20, g_afterhook_info
    ldr x0, [x20]           // x0 = ArtMethod* (Before时保存的)
    ldr x1, [x20, #8]       // x1 = this指针 (Before时保存的)
    mov x2, x19             // x2 = Thread*
    ldr x3, [x20, #24]      // x3 = shorty (Before时保存的)
    mov x4, xzr             // x4 = args (After回调不需要原始参数)
    adr x5, g_ret_xregs     // x5 = 返回值寄存器数组
    adr x6, g_ret_fregs     // x6 = 返回值浮点寄存器数组

    // 调用After回调
    ldr x17, g_pfnAfterCallback_addr
    blr x17

    // 恢复栈指针
    ldp x29, x30, [sp]
    add sp, sp, #16

    // 恢复返回值寄存器（可能被AfterCallback修改）
    adr x20, g_ret_fregs
    ldp d0, d1, [x20]
    ldp d2, d3, [x20, #16]

    adr x20, g_ret_xregs
    ldp x0, x1, [x20]
    ldp x2, x3, [x20, #16]

    // 从数据区恢复原始返回地址并返回
    adr x20, g_afterhook_info
    ldr x30, [x20, #16]     // 恢复原始返回地址
    ret



AfterHook_addr:
AfterHook_addr_addr:
    .space 8, 0

.data
    g_pfnCallback_addr:
    g_pfnCallback:
        .space 8, 0

    g_pfnAfterCallback_addr:
    g_pfnAfterCallback:
        .space 8, 0

    g_xregs: .space 8*8, 0
    g_fregs: .space 8*8, 0

    g_afterhook_info:
        .space 8, 0    // ArtMethod*
        .space 8, 0    // this指针
        .space 8, 0    // 原始返回地址
        .space 8, 0    // shorty指针

    g_ret_xregs: .space 8*4, 0    // x0-x3
    g_ret_fregs: .space 8*4, 0    // d0-d3
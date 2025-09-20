.arch armv8-a

.global Saved_OldCode
.global RetDst_addr
.global SmallTramplie
.global Trimpline
.global g_pfnCallback

.text
Trimpline:
    //保存所有需要的寄存器
    adr x20, g_xregs
    stp x0, x1, [x20], 0x10
    stp x2, x3, [x20], 0x10
    stp x4, x5, [x20], 0x10
    stp x6, x7, [x20], 0x10

    adr x20, g_fregs
    stp d0, d1, [x20], 0x10
    stp d2, d3, [x20], 0x10
    stp d4, d5, [x20], 0x10
    stp d6, d7, [x20], 0x10

    sub sp, sp, 0x10
    stp x29, x30, [sp]

    //brk 0

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

    //brk 0
    ldp x29, x30, [sp]
    add sp, sp, 0x10

    //恢复所有的寄存器
    adr x20, g_xregs
    ldp x0, x1, [x20], 0x10
    ldp x2, x3, [x20], 0x10
    ldp x4, x5, [x20], 0x10
    ldp x6, x7, [x20], 0x10

    adr x20, g_fregs
    ldp d0, d1, [x20], 0x10
    ldp d2, d3, [x20], 0x10
    ldp d4, d5, [x20], 0x10
    ldp d6, d7, [x20], 0x10


Saved_OldCode: // 保存原函数的指令
    .space 0x10, 0

SmallTramplie:
Trampline_BrDst:
    ldr x17, RetDst_addr_addr
    br x17
RetDst_addr: 
RetDst_addr_addr: 
    .space 8, 0

.data
    g_pfnCallback_addr:
    g_pfnCallback: 
        .space 8, 0
    g_xregs: .space 8*8, 0
    g_fregs: .space 8*8, 0


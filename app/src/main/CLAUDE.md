# ARK Hook 项目

## 项目概述

这是一个基于Android ART运行时的Hook框架，能够拦截和修改Java方法的执行，支持在方法执行前后插入自定义逻辑。

## 需求分析

### 功能需求
1. **Hook指定方法**
   - 一般方法
   - Native方法
   - 静态方法

2. **提供通用回调接口**
   - JNI层回调接口
   - Java层回调接口

3. **参数和返回值修改**
   - 在before回调中修改参数
   - 在after回调中修改返回值

4. **多方法Hook支持**
   - 可以同时hook多个方法

## 架构设计

### C++层核心类

#### 1. ART类 (art.h)
- 封装AOSP的ART运行时相关结构体和API
- 提供ArtMethod、Object、Thread等核心类型
- 实现JIT编译等底层功能

#### 2. ArgsConverter类 (ArgsConverter.h/.cpp)
负责Java与C++之间的参数和堆栈转换：
- `artArgs2JArray()`: ART参数转换为Java对象数组
- `JArray2ARTArgs()`: Java对象数组转换为ART参数
- `parseReturnValue()`: 解析返回值
- `WriteReturnValue()`: 写入新的返回值

#### 3. Hook类 (IHook.h/.cpp)
核心Hook功能实现：
- `InstallMethodHook()`: 安装方法Hook
- `InstallHook()`: 底层Hook安装（内存修改）
- `UninstallHook()`: 卸载Hook（暂未实现）
- `FindRetInst()`: 查找返回指令地址
- `BeforeCallBack()`: Before回调入口
- `AfterCallBack()`: After回调入口（暂未实现）

### 汇编跳板 (Trampline.S)

#### 当前实现的跳板结构：
```assembly
SmallTramplie:           # 小跳板（16字节）
    ldr x17, RetDst_addr_addr
    br x17

Trimpline:               # 主跳板
    # 1. 保存所有寄存器状态
    # 2. 准备回调参数
    # 3. 调用C++回调函数
    # 4. 恢复寄存器状态
    # 5. 执行原函数代码

Saved_OldCode:           # 保存的原函数代码
    .space 0x10, 0
```

## 当前实现状态

### 已实现功能
✅ Before Hook功能完整实现
✅ 参数修改功能（在Before回调中）
✅ 汇编跳板机制
✅ ART参数转换
✅ 内存保护和代码修改

### 当前问题与挑战

#### 核心问题：After Hook实现
**问题描述：** 当前只实现了Before Hook，After Hook代码被注释掉了（IHook.cpp:39-52）

**技术挑战：**
1. **返回指令查找困难**: `FindRetInst()`方法尝试查找返回指令，但在复杂的编译代码中可能有多个返回点
2. **跳板复杂性**: 需要支持Before和After两个回调点的跳板机制
3. **返回值传递**: 需要在执行原函数后获取并可能修改返回值

#### 可能的解决方案
根据需求中的建议："是不是修改一行返回值让原函数执行返回后进入After就行"

**方案1: 修改返回地址**
- 在Before Hook中保存原返回地址
- 将返回地址修改为After Hook跳板
- After Hook执行完后跳转回真正的返回地址

**方案2: 包装原函数**
- 完全替换原函数入口
- 在新的函数中依次调用：Before → 原函数 → After
- 通过函数指针调用保存的原函数代码

## 代码文件说明

### C++文件
- `IHook.cpp`: Hook核心实现逻辑（210行）
- `native-lib.cpp`: JNI接口和测试代码（332行）
- `ArgsConverter.h`: 参数转换类声明（49行）
- `IHook.h`: Hook类接口定义（47行）
- `art.h`: ART运行时结构体定义

### 汇编文件
- `Trampline.S`: ARM64汇编跳板实现（85行）

### Java测试文件
- `CFoo.java`: 测试用的Java类（34行）

## 下一步开发计划

### 优先级1: 实现After Hook
1. 分析当前跳板机制，设计支持Before+After的新跳板
2. 实现返回地址修改方案
3. 测试After Hook的参数传递和返回值修改

### 优先级2: 多Hook支持优化
1. 重构Hook管理机制，支持同时Hook多个方法
2. 实现Hook链式调用
3. 添加Hook卸载功能

### 优先级3: 稳定性改进
1. 改进内存保护机制
2. 添加异常处理
3. 优化汇编跳板性能

## 技术栈
- **语言**: C++, ARM64汇编, Java
- **平台**: Android ART运行时
- **架构**: ARM64
- **依赖**: shadowhook库，Android NDK

## 测试方法
当前通过`CFoo.foo()`方法进行测试，可以验证：
- Before Hook参数修改
- 参数类型转换
- 基本Hook机制

## 注意事项
1. 需要root权限进行内存修改
2. 仅支持ARM64架构
3. 依赖特定的ART版本结构体定义
4. After Hook功能待完善
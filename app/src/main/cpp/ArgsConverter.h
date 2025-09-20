//
// Created by XiaM on 2025/9/20.
//

#ifndef TEST_ARGSCONVERTER_H
#define TEST_ARGSCONVERTER_H

//java与native间需要参数和栈的转换
class ArgsConverter {

/* ============ 需要处理的参数转换类型(可能不完整 参考native-lib中已经实现的) ============

// 1. 基本类型转换（装箱/拆箱）
原始类型 → Java包装类 → 原始类型
├── byte    ↔ Byte
├── char    ↔ Character
├── short   ↔ Short
├── int     ↔ Integer
├── long    ↔ Long
├── float   ↔ Float
├── double  ↔ Double
└── boolean ↔ Boolean

// 2. 引用类型转换
├── Object* (ART) ↔ jobject (JNI)
├── String (特殊处理)
└── Array (需要递归处理)

// 3. 寄存器/栈 → Java对象数组
├── 整数参数：x2-x7寄存器 / 栈
├── 浮点参数：v0-v7寄存器 / 栈
└── 返回值：x0/v0寄存器
*/


};


#endif //TEST_ARGSCONVERTER_H

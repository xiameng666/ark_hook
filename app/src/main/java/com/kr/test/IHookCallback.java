package com.kr.test;

public interface IHookCallback {
    // Before: 修改参数
    void onBefore(Object thiz, Object[] args);

    // After: 修改返回值
    Object onAfter(Object thiz, Object[] args, Object result);
}

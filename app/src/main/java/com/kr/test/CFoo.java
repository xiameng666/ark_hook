package com.kr.test;

import android.util.Log;

public class CFoo {
    public int n = 100;
    public float f = 3.14f;
    static public int n1 = 200;
    static public float f1 = 3.14f;

    public void show(){
        Log.v("kr", "show");
    }
    public static void showStatic() {
        Log.v("kr", "showStatic");
    }

    int foo(char ch, float f, long l, double d, String str, byte b, int n)
    {
        //Boolean b0  = Boolean.valueOf(true);
        //b0.booleanValue()

        Log.v("kr", "ch:"+ch+" f:"+f+" l:"+l+" d:"+d + " str:"+str + " b:"+b + " n:"+n);
        return  0;

    }


    public static void main(String[] args) {
        System.out.println("hello world");
    }

    public native void testNative();
}

//
// Created by USER on 2025/9/13.
//
#include "art.h"
#include <shadowhook.h>
#include <dlfcn.h>

Thread::DecodeJObject_t Thread::DecodeJObject_art = nullptr;
ArtField::PrettyField_t ArtField::PrettyField_raw = nullptr;

ArtMethod::PrettyMethod_t ArtMethod::PrettyMethod_raw = nullptr;
void* ArtMethod::art_quick_to_interpreter_bridge = nullptr;

JavaVMExt::AddWeakGlobalRef_Fun JavaVMExt::AddWeakGlobalRef_raw = nullptr;


void* Jit::jit_library_handle_= nullptr;
void* Jit::jit_compiler_handle_ = nullptr;

void* (*Jit::jit_load_)(bool*) = nullptr;
void (*Jit::jit_unload_)(void*) = nullptr;
bool (*Jit::jit_compile_method_)(void*, ArtMethod*, Thread*, bool) = nullptr;
void (*Jit::jit_types_loaded_)(void*, MyClass**, size_t count) = nullptr;
bool Jit::generate_debug_info_ = false;

bool Jit::LoadCompilerLibrary() {
    jit_library_handle_ = shadowhook_dlopen("libart-compiler.so");
    if (jit_library_handle_ == nullptr) {
        LOGV("libart-compiler.so %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }
    jit_load_ = reinterpret_cast<void* (*)(bool*)>(shadowhook_dlsym(jit_library_handle_, "jit_load"));
    if (jit_load_ == nullptr) {
        LOGV("jit_load_ 失败 %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }
    jit_unload_ = reinterpret_cast<void (*)(void*)>(shadowhook_dlsym(jit_library_handle_, "jit_unload"));
    if (jit_unload_ == nullptr) {
        LOGV("jit_unload_ 失败 %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }
    jit_compile_method_ = reinterpret_cast<bool (*)(void*, ArtMethod*, Thread*, bool)>(
            shadowhook_dlsym(jit_library_handle_, "jit_compile_method"));
    if (jit_compile_method_ == nullptr) {
        LOGV("jit_compile_method_ 失败 %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }
    jit_types_loaded_ = reinterpret_cast<void (*)(void*, MyClass**, size_t)>(
            shadowhook_dlsym(jit_library_handle_, "jit_types_loaded"));
    if (jit_types_loaded_ == nullptr) {
        LOGV("jit_types_loaded_ 失败 %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }

    jit_compiler_handle_ = (jit_load_)(&generate_debug_info_);
    if (jit_compiler_handle_ == nullptr) {
        return false;
    }

    return true;
}

bool Jit::CompileMethod(ArtMethod *method, Thread *self) {
    return jit_compile_method_(jit_compiler_handle_, method, self, false);
}

bool InitArt()
{
    void* hArt = shadowhook_dlopen("libart.so");
    if (hArt == nullptr){
        LOGV("打开art失败 %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }

    Thread::DecodeJObject_art = (Thread::DecodeJObject_t)shadowhook_dlsym(hArt, "_ZNK3art6Thread13DecodeJObjectEP8_jobject");
    if (Thread::DecodeJObject_art == nullptr){
        LOGV("DecodeJObject_art失败 %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }

    ArtField::PrettyField_raw = (ArtField::PrettyField_t) shadowhook_dlsym(hArt, "_ZN3art8ArtField11PrettyFieldEb");
    if (ArtField::PrettyField_raw == nullptr){
        LOGV("PrettyField_raw %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }


    ArtMethod::PrettyMethod_raw = (ArtMethod::PrettyMethod_t) shadowhook_dlsym(hArt, "_ZN3art9ArtMethod12PrettyMethodEb");
    if (ArtMethod::PrettyMethod_raw == nullptr){
        LOGV("PrettyMethod_raw %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }

    ArtMethod::art_quick_to_interpreter_bridge = (void*) shadowhook_dlsym(hArt, "art_quick_to_interpreter_bridge");
    if (ArtMethod::art_quick_to_interpreter_bridge == nullptr){
        LOGV("art_quick_to_interpreter_bridge %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }


    JavaVMExt::AddWeakGlobalRef_raw = (JavaVMExt::AddWeakGlobalRef_Fun) shadowhook_dlsym(hArt, "_ZN3art9JavaVMExt16AddWeakGlobalRefEPNS_6ThreadENS_6ObjPtrINS_6mirror6ObjectEEE");
    if (JavaVMExt::AddWeakGlobalRef_raw == nullptr){
        LOGV("AddWeakGlobalRef_raw %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }

    if(!Jit::LoadCompilerLibrary()){
        LOGV("LoadCompilerLibrary 失败");
        return false;
    }
    return true;
}
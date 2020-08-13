#include <jni.h>
#include <string>
#include "simple_png_encoder.h"

/// JNI utils
static std::string fromJavaString(JNIEnv *env, jstring java_string) {
    const char *p = env->GetStringUTFChars(java_string, nullptr);
    std::string result(p);
    return std::move(result);
}

extern "C" JNIEXPORT jstring JNICALL Java_com_zqc_simplepngencoder_MainActivity_testEncodePng(
        JNIEnv *env,
        jobject /* this */,
        jstring filepath) {
    std::string filename = fromJavaString(env, filepath) + "/test.png";
    simplepng::test_write_png(std::move(filename));

    return env->NewStringUTF("Png Encoded!");
}

/**
 * @file stdio_compat.c
 * @brief Provide the global `stderr` symbol for prebuilt third-party libs.
 *
 * The riscv toolchain libc runs in reent mode: stdin/stdout/stderr are macros
 * expanding to _impure_ptr members, so no global FILE symbol exists. Prebuilt
 * archives compiled elsewhere (esp-sr S31 libs: libdl_lib, libc_speech_features,
 * libesp_audio_processor, libflite_g2p) reference a global stderr and fail to
 * link. Define one aliasing the main-thread reent stderr so their
 * fprintf(stderr, ...) actually prints to the console.
 */

#include <stdio.h>
#include <sys/reent.h>

/* asm name "stderr": the C identifier stays macro-safe, the linker sees a
 * global FILE* named stderr (static would make the symbol local). */
FILE *sg_stdio_stderr_shim __asm__("stderr") = NULL;

__attribute__((constructor)) static void __stdio_compat_init(void)
{
    if (NULL == sg_stdio_stderr_shim) {
        sg_stdio_stderr_shim = _impure_ptr->_stderr;
    }
}

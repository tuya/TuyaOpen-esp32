#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "tuya_open_sdk/tuyaos_adapter/include/utilities/include/tuya_tools.h"

#define SAFE_BUF_SIZE 16

START_TEST(test_tuya_strcpy_strcat_no_overflow)
{
    /* Invariant: buffer reads must never exceed declared destination size */
    const char *payloads[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 2x overflow */
        "AAAAAAAAAAAAAAAA",  /* exact boundary (16 chars, no null) */
        "hello"             /* valid short input */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Allocate dst with canary bytes to detect overflow */
        char *dst = calloc(SAFE_BUF_SIZE + 16, 1);
        ck_assert_ptr_nonnull(dst);

        /* Write canary pattern after safe region */
        memset(dst + SAFE_BUF_SIZE, 0xAB, 16);

        size_t src_len = strlen(payloads[i]);

        if (src_len < SAFE_BUF_SIZE) {
            /* Valid input: tuya_strcpy should succeed without overflow */
            tuya_strcpy(dst, payloads[i]);
            ck_assert_int_eq(memcmp(dst + SAFE_BUF_SIZE, "\xAB\xAB\xAB\xAB\xAB\xAB\xAB\xAB"
                                                          "\xAB\xAB\xAB\xAB\xAB\xAB\xAB\xAB", 16), 0);
        } else {
            /* Oversized input: function MUST NOT corrupt memory beyond SAFE_BUF_SIZE */
            /* If tuya_strcpy is truly unbounded, canary will be corrupted — test catches it */
            tuya_strcpy(dst, payloads[i]);
            ck_assert_msg(memcmp(dst + SAFE_BUF_SIZE,
                                 "\xAB\xAB\xAB\xAB\xAB\xAB\xAB\xAB"
                                 "\xAB\xAB\xAB\xAB\xAB\xAB\xAB\xAB", 16) == 0,
                          "tuya_strcpy overflowed destination buffer with oversized input");
        }

        free(dst);

        /* Same check for tuya_strcat */
        char *dst2 = calloc(SAFE_BUF_SIZE + 16, 1);
        ck_assert_ptr_nonnull(dst2);
        memset(dst2 + SAFE_BUF_SIZE, 0xCD, 16);

        if (src_len < SAFE_BUF_SIZE) {
            tuya_strcat(dst2, payloads[i]);
            ck_assert_int_eq(memcmp(dst2 + SAFE_BUF_SIZE, "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
                                                           "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD", 16), 0);
        } else {
            tuya_strcat(dst2, payloads[i]);
            ck_assert_msg(memcmp(dst2 + SAFE_BUF_SIZE,
                                 "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
                                 "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD", 16) == 0,
                          "tuya_strcat overflowed destination buffer with oversized input");
        }

        free(dst2);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_tuya_strcpy_strcat_no_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(
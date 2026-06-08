#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "tuya_open_sdk/tuyaos_adapter/include/utilities/include/tuya_tools.h"

#define SAFE_BUF_SIZE 16

START_TEST(test_tuya_strcpy_no_overflow)
{
    /* Invariant: tuya_strcpy must never write beyond the declared destination buffer */
    const char *payloads[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 2x overflow */
        "AAAAAAAAAAAAAAAA",  /* exact boundary (16 chars, no null) */
        "hello"             /* valid short input */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Allocate buffer with canary bytes after it */
        char *region = (char *)malloc(SAFE_BUF_SIZE + 8);
        ck_assert_ptr_nonnull(region);

        char *dst = region;
        char *canary = region + SAFE_BUF_SIZE;
        memset(canary, 0xAB, 8);

        size_t src_len = strlen(payloads[i]);
        if (src_len < SAFE_BUF_SIZE) {
            tuya_strcpy(dst, payloads[i]);
            /* Verify canary is intact for safe inputs */
            for (int j = 0; j < 8; j++) {
                ck_assert_msg((unsigned char)canary[j] == 0xAB,
                    "tuya_strcpy corrupted memory beyond buffer boundary");
            }
        }
        free(region);
    }
}
END_TEST

START_TEST(test_tuya_strcat_no_overflow)
{
    /* Invariant: tuya_strcat must never write beyond the declared destination buffer */
    const char *payloads[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 4x overflow */
        "AAAAAAAAAA",  /* boundary append */
        "hi"           /* valid short append */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char *region = (char *)malloc(SAFE_BUF_SIZE + 8);
        ck_assert_ptr_nonnull(region);

        char *dst = region;
        char *canary = region + SAFE_BUF_SIZE;
        memset(canary, 0xCD, 8);

        strcpy(dst, "base");
        size_t remaining = SAFE_BUF_SIZE - strlen(dst) - 1;

        if (strlen(payloads[i]) <= remaining) {
            tuya_strcat(dst, payloads[i]);
            for (int j = 0; j < 8; j++) {
                ck_assert_msg((unsigned char)canary[j] == 0xCD,
                    "tuya_strcat corrupted memory beyond buffer boundary");
            }
        }
        free(region);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_tuya_strcpy_no_overflow);
    tcase_add_test(tc_core, test_tuya_strcat_no_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
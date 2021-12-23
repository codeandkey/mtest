#include "mtest.h"

#ifdef _cplusplus
#include <cstdlib>
#include <cstdio>
#include <ctime>
#else
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#endif

static _mtest_t** all_tests;
static int num_tests;

int _mtest_add(_mtest_t* tstruct) {
    ++num_tests;

    if (!all_tests) {
        all_tests = (_mtest_t**) malloc(sizeof(*all_tests));
    } else {
        all_tests = (_mtest_t**) realloc(all_tests, sizeof(*all_tests) * num_tests);
    }

    all_tests[num_tests - 1] = tstruct;

    return num_tests;
}

int mtest_main(int argc, char** argv) {
    char datestr[100];
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    int total_failures = 0;
    int failed_tests = 0;

    strftime(datestr, sizeof(datestr) - 1, "%m/%d/%Y %H:%H", t);

    printf("======== TEST RUN (%d total): %15s ========\n", num_tests, datestr);

    for (int i = 0; i < num_tests; ++i) {
        printf("    %d / %d    %s ... ", i + 1, num_tests, all_tests[i]->tname);
        fflush(stdout);

        clock_t start_time = clock();
        all_tests[i]->testfun(all_tests[i]);
        clock_t end_time = clock();

        if (all_tests[i]->num_failures) {
            printf("FAILED ");
            failed_tests++;
        } else {
            printf("OK     ");
        }

        printf("( %lu ms )\n", (end_time - start_time) / (CLOCKS_PER_SEC / 1000));

        total_failures += all_tests[i]->num_failures;
    }

    if (total_failures) {
        printf("======== SUMMARY OF %d FAILED TEST%s ========\n", failed_tests, (failed_tests > 1) ? "s" : "");

        for (int i = 0; i < num_tests; ++i) {
            if (all_tests[i]->num_failures) {
                printf("%s:\n", all_tests[i]->tname);

                for (int j = 0; j < all_tests[i]->num_failures; ++j) {
                    printf("    %s\n", all_tests[i]->failures[j]);
                }
            }
        }
    } else {
        printf("======== ALL TESTS PASSED ========\n");
    }

    return total_failures ? -1 : 0;
}

void _mtest_fail(void* self, const char* fmt, ...) {
    va_list ap;
    int len = 0;
    char* nfailure = 0;

    va_start(ap, fmt);
    len = vsnprintf(nfailure, len, fmt, ap);
    va_end(ap);

    nfailure = (char*) malloc(len + 1);

    va_start(ap, fmt);
    vsnprintf(nfailure, len + 1, fmt, ap);
    va_end(ap);

    _mtest_t* tstruct = (_mtest_t*) self;
    ++tstruct->num_failures;

    if (!tstruct->failures) {
        tstruct->failures = (char**) malloc(sizeof(char*));
    } else {
        tstruct->failures = (char**) realloc(tstruct->failures, sizeof(char*) * tstruct->num_failures);
    }

    tstruct->failures[tstruct->num_failures - 1] = nfailure;

    va_end(ap);
}

#include "mtest.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static _mtest_t** all_tests;
static int num_tests;

static int _get_terminal_width();
static char* _print_into_buf(const char* fmt, va_list args);
static void _print_centered_header(const char* fmt, ...);

int mtest_main(int argc, char** argv) {
    char datestr[100];
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    int total_failures = 0;
    int failed_tests = 0;

    strftime(datestr, sizeof(datestr) - 1, "%m/%d/%Y %H:%H", t);

    _print_centered_header("TEST RUN (%d total): %s", num_tests, datestr);

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
        _print_centered_header("SUMMARY OF %d FAILED TEST%s", failed_tests, (failed_tests > 1) ? "S" : "");

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

void _mtest_fail(void* self, const char* fmt, ...) {
    va_list args;

    _mtest_t* tstruct = (_mtest_t*) self;
    ++tstruct->num_failures;

    if (!tstruct->failures) {
        tstruct->failures = (char**) malloc(sizeof(char*));
    } else {
        tstruct->failures = (char**) realloc(tstruct->failures, sizeof(char*) * tstruct->num_failures);
    }

    va_start(args, fmt);
    tstruct->failures[tstruct->num_failures - 1] = _print_into_buf(fmt, args);
    va_end(args);
}

int _get_terminal_width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return info.srWindow.Right - info.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#endif
}

char* _print_into_buf(const char* fmt, va_list ap) {
    va_list ap2;
    int len = 0;
    char* buf = NULL;

    va_copy(ap2, ap);

    len = vsnprintf(buf, len, fmt, ap);

    buf = (char*) malloc(len + 1);

    vsnprintf(buf, len + 1, fmt, ap2);
    va_end(ap2);

    return buf;
}

void _print_centered_header(const char* fmt, ...) {
    va_list ap;
    char* buf;

    va_start(ap, fmt);
    buf = _print_into_buf(fmt, ap);
    va_end(ap);

    int len = strlen(buf);
    int tsize = _get_terminal_width();

    int padding = (tsize - (len + 2)) / 2;

    char* pstr = (char*) malloc(padding + 1);
    memset(pstr, '=', padding);
    pstr[padding] = '\0';

    printf("%s %s %s", pstr, buf, pstr);

    if (padding * 2 + len + 2 < tsize) {
        printf("=");
    }

    printf("\n");

    free(buf);
    free(pstr);
}

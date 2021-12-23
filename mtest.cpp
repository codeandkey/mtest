/* vim: set ts=2 sw=2: */

#include "mtest.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RED 0
#define GREEN 1
#define BLUE 2
#define RESET 3

#define BWAIT 10000

#ifdef _WIN32
typedef struct {
  HANDLE handle;
  HANDLE mutex;
  int req;
} mtest_thread;
#else
typedef struct {
  pthread_t handle;
  pthread_mutex_t mutex;
  int req;
} mtest_thread;
static pthread_mutex_t out_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static _mtest_t **all_tests;
static int num_tests;
static mtest_thread *threads;
static int num_threads;
static int total_failures;
static int failed_tests;
static int total_tested;

static int _get_terminal_width();
static char *_print_into_buf(const char *fmt, va_list args);
static void _print_centered_header(const char *fmt, ...);
static void _set_color(int col);

static void mtest_thread_init(mtest_thread* m);
static void mtest_lock(mtest_thread* m);
static int  mtest_trylock(mtest_thread* m);
static void mtest_unlock(mtest_thread* m);
static void mtest_join(mtest_thread* m);

static void* mtest_thread_main(void* ud);

int mtest_main(int argc, char **argv) {
  char datestr[100];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  clock_t tstart_time = clock();

  strftime(datestr, sizeof(datestr) - 1, "%m/%d/%Y %H:%H", t);

  _print_centered_header("TEST RUN (%d total): %s", num_tests, datestr);

#ifdef _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  num_threads = info.dwNumberOfProcessors;
#else
  num_threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  printf("    > Starting tests on %d threads\n", num_threads);

  // Initialize worker threads
  threads = (mtest_thread*) malloc(sizeof(mtest_thread) * num_threads);
  for (int i = 0; i < num_threads; ++i) {
    mtest_thread_init(&threads[i]);
  }

  for (int i = 0; i < num_tests; ++i) {
    // Wait for free worker
    int assigned = 0;
    while (!assigned) {
      for (int j = 0; j < num_threads; ++j) {
        if (mtest_trylock(&threads[j])) {
          if (threads[j].req == -1) {
            assigned = 1;
            threads[j].req = i;

            mtest_unlock(&threads[j]);
            break;
          } else {
            mtest_unlock(&threads[j]);
          }
        }
      }

      usleep(BWAIT);
    }
  }

  // Wait for each thread to complete
  int done = 0;
  while (!done) {
    done = 1;

    for (int i = 0; i < num_threads; ++i) {
        if (mtest_trylock(&threads[i])) {
          if (threads[i].req != -1) {
            done = 0;
            break;
          }
          mtest_unlock(&threads[i]);
        }
    }

    usleep(BWAIT);
  }

  // Tell threads to stop
  for (int i = 0; i < num_threads; ++i) {
    mtest_lock(&threads[i]);
    threads[i].req = -2;
    mtest_unlock(&threads[i]);
  }

  // Join remaining threads
  for (int i = 0; i < num_threads; ++i) {
    mtest_join(&threads[i]);
  }

  clock_t tend_time = clock();
  printf("    > Finished testing in %.1f seconds\n", (float) (tend_time - tstart_time) / CLOCKS_PER_SEC);

  if (total_failures) {
    _print_centered_header("SUMMARY OF %d FAILED TEST%s", failed_tests,
                           (failed_tests > 1) ? "S" : "");

    for (int i = 0; i < num_tests; ++i) {
      if (all_tests[i]->num_failures) {
        printf("%s:\n", all_tests[i]->tname);

        for (int j = 0; j < all_tests[i]->num_failures; ++j) {
          printf("    %s\n", all_tests[i]->failures[j]);
        }
      }
    }
  } else {
    _print_centered_header("ALL TESTS PASSED");
  }

  return total_failures ? -1 : 0;
}

int _mtest_add(_mtest_t *tstruct) {
  ++num_tests;

  if (!all_tests) {
    all_tests = (_mtest_t **)malloc(sizeof(*all_tests));
  } else {
    all_tests = (_mtest_t **)realloc(all_tests, sizeof(*all_tests) * num_tests);
  }

  all_tests[num_tests - 1] = tstruct;

  return num_tests;
}

void _mtest_fail(void *self, const char *fmt, ...) {
  va_list args;

  _mtest_t *tstruct = (_mtest_t *)self;
  ++tstruct->num_failures;

  if (!tstruct->failures) {
    tstruct->failures = (char **)malloc(sizeof(char *));
  } else {
    tstruct->failures = (char **)realloc(
        tstruct->failures, sizeof(char *) * tstruct->num_failures);
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

char *_print_into_buf(const char *fmt, va_list ap) {
  va_list ap2;
  int len = 0;
  char *buf = NULL;

  va_copy(ap2, ap);

  len = vsnprintf(buf, len, fmt, ap);

  buf = (char *)malloc(len + 1);

  vsnprintf(buf, len + 1, fmt, ap2);
  va_end(ap2);

  return buf;
}

void _print_centered_header(const char *fmt, ...) {
  va_list ap;
  char *buf;

  va_start(ap, fmt);
  buf = _print_into_buf(fmt, ap);
  va_end(ap);

  int len = strlen(buf);
  int tsize = _get_terminal_width();

  int padding = (tsize - (len + 2)) / 2;

  char *pstr = (char *)malloc(padding + 1);
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

void _set_color(int col) {
#ifndef MTEST_NOCOLOR
#ifdef _WIN32
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);

  switch (col) {
  case RED:
    SetConsoleTextAttribute(con, FOREGROUND_RED);
    break;
  case GREEN:
    SetConsoleTextAttribute(con, FOREGROUND_GREEN);
    break;
  case BLUE:
    SetConsoleTextAttribute(con, FOREGROUND_BLUE);
    break;
  case RESET:
    SetConsoleTextAttribute(con, FOREGROUND_WHITE);
    break;
  }
#else
  switch (col) {
  case RED:
    printf("\u001b[31m");
    break;
  case GREEN:
    printf("\u001b[32m");
    break;
  case BLUE:
    printf("\u001b[34m");
    break;
  case RESET:
    printf("\u001b[0m");
    break;
  }
#endif
#endif
}

void mtest_thread_init(mtest_thread* m) {
  m->req = -1;

#ifdef _WIN32
#error W32 thread
#else
  pthread_mutex_init(&m->mutex, NULL);
  pthread_create(&m->handle, NULL, mtest_thread_main, (void*) m);
#endif
}

void mtest_lock(mtest_thread *m) {
#ifdef _WIN32
#error W32 mutex
#else
  pthread_mutex_lock(&m->mutex);
#endif
}

int mtest_trylock(mtest_thread *m) {
#ifdef _WIN32
#error W32 mutex
#else
  return !pthread_mutex_trylock(&m->mutex);
#endif
}

void mtest_unlock(mtest_thread *m) {
#ifdef _WIN32
#error W32 mutex
#else
  pthread_mutex_unlock(&m->mutex);
#endif
}

void mtest_join(mtest_thread *m) {
#ifdef _WIN32
#error W32 mutex
#else
  pthread_join(m->handle, NULL);
#endif
}

void* mtest_thread_main(void* ud) {
  mtest_thread* self = (mtest_thread*) ud;

  while (1) {
    mtest_lock(self);
    int creq = self->req;
    mtest_unlock(self);

    if (self->req == -2) {
      break;
    }

    if (self->req == -1) {
      usleep(BWAIT);
      continue;
    }

    // Run test!
    clock_t start_time = clock();
    all_tests[creq]->testfun(all_tests[creq]);
    clock_t end_time = clock();

    // Acquire output mutex
#ifdef _WIN32
#error W32 mutex
#else
    pthread_mutex_lock(&out_mutex);
#endif
    printf("    [%lu] %d / %d    %s ... ", self - threads, ++total_tested, num_tests, all_tests[creq]->tname);
    if (all_tests[creq]->num_failures) {
      _set_color(RED);
      printf("FAILED ");
      _set_color(RESET);

      ++failed_tests;
    } else {
      _set_color(GREEN);
      printf("OK     ");
      _set_color(RESET);
    }

    printf("( %lu ms )\n", (end_time - start_time) / (CLOCKS_PER_SEC / 1000));

    total_failures += all_tests[creq]->num_failures;
#ifdef _WIN32
#error W32 mutex
#else
    pthread_mutex_unlock(&out_mutex);
#endif
    
    mtest_lock(self);
    self->req = -1;
    mtest_unlock(self);
  }

  return NULL;
}

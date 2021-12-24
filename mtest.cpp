// SPDX-License-Identifier: MIT
// vim: set ts=2 sw=2:

#include "mtest.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <stdarg.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

#define RED 0
#define GREEN 1
#define BLUE 2
#define RESET 3

#define BWAIT 10000
#define STATUS_WAIT 250000

#ifdef _WIN32
typedef HANDLE mtest_mutex;
typedef struct {
  HANDLE handle;
  mtest_mutex mutex;
  int req;
} mtest_thread;
#else
typedef pthread_mutex_t mtest_mutex;
typedef struct {
  pthread_t handle;
  mtest_mutex mutex;
  int req;
} mtest_thread;
static mtest_mutex out_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t status_thread;
#endif

struct Test {
  Test(void(*tfun)(void*), const char* name) : tfun(tfun), name(name) {}

  void (*tfun)(void*);
  const char* name;
  vector<stringstream> failures;
};

static vector<Test>* all_tests;
static mtest_thread *threads;
static int num_threads;
static int total_failures;
static int total_tested;
static int failed_tests;
static int max_testlen;

static int _get_terminal_width();
static void _clear_row();
static char *_print_into_buf(const char *fmt, va_list args);
static void _print_centered_header(const char *fmt, ...);
static void _set_color(int col);
static void _cleanup();

static void mtest_thread_init(mtest_thread *m);
static void mtest_lock(mtest_mutex *m);
static int  mtest_trylock(mtest_mutex *m);
static void mtest_unlock(mtest_mutex *m);
static void mtest_join(mtest_thread *m);

static void *mtest_status_main(void *ud);
static void *mtest_thread_main(void *ud);

int mtest_main(int argc, char **argv) {
  char datestr[100];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  clock_t tstart_time = clock();

  strftime(datestr, sizeof(datestr) - 1, "%m/%d/%Y %H:%H", t);

  _print_centered_header("TEST RUN (%d total): %s", all_tests->size(), datestr);

#ifdef _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  num_threads = info.dwNumberOfProcessors;
#else
  num_threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  cout << "    > Testing on " << num_threads << " threads" << endl;

  // Determine name alignment
  for (auto &t : *all_tests) {
    int len = strlen(t.name);
    if (len > max_testlen) max_testlen = len;
  }

  // Initialize worker threads
  threads = (mtest_thread *)malloc(sizeof(mtest_thread) * num_threads);
  for (int i = 0; i < num_threads; ++i) {
    mtest_thread_init(&threads[i]);
  }

  // Initialize status thread
#ifdef _WIN32
#error W32 thread
#else
  pthread_create(&status_thread, NULL, mtest_status_main, NULL);
#endif

  for (unsigned int i = 0; i < all_tests->size(); ++i) {
    // Wait for free worker
    int assigned = 0;
    while (!assigned) {
      for (int j = 0; j < num_threads; ++j) {
        if (mtest_trylock(&threads[j].mutex)) {
          if (threads[j].req == -1) {
            assigned = 1;
            threads[j].req = i;

            mtest_unlock(&threads[j].mutex);
            break;
          } else {
            mtest_unlock(&threads[j].mutex);
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
      mtest_lock(&threads[i].mutex);

      if (threads[i].req != -1) {
        done = 0;
        mtest_unlock(&threads[i].mutex);
        break;
      }

      mtest_unlock(&threads[i].mutex);
    }

    usleep(BWAIT);
  }

  // Tell threads to stop
  for (int i = 0; i < num_threads; ++i) {
    mtest_lock(&threads[i].mutex);
    threads[i].req = -2;
    mtest_unlock(&threads[i].mutex);
  }

  // Join remaining threads
  for (int i = 0; i < num_threads; ++i) {
    mtest_join(&threads[i]);
  }

  // Wait for status thread
#ifdef _WIN32
#error W32 thread
#else
  pthread_join(status_thread, NULL);
#endif

  clock_t tend_time = clock();
  _clear_row();

  cout 
    << "    > Finished testing in "
    << setprecision(2)
    << (float)(tend_time - tstart_time) / CLOCKS_PER_SEC
    << " seconds" << endl;

  if (total_failures) {
    _print_centered_header("SUMMARY OF %d FAILED TEST%s", failed_tests,
                           (failed_tests > 1) ? "S" : "");

    for (unsigned int i = 0; i < all_tests->size(); ++i) {
      if (all_tests->at(i).failures.size()) {
        printf("%s:\n", all_tests->at(i).name);

        for (auto &f : all_tests->at(i).failures) {
          cout << "    " << f.str() << endl;
        }
      }
    }
  } else {
    _print_centered_header("ALL TESTS PASSED");
  }

  _cleanup();
  return total_failures ? -1 : 0;
}

int _mtest_push(const char* name, void (*tfun)(void*)) {
  if (!all_tests) {
    all_tests = new vector<Test>();
  }

  all_tests->push_back(Test(tfun, name));

  return all_tests->size();
}

std::ostream& _mtest_fail(void *self) {
  Test *tstruct = (Test *)self;
  tstruct->failures.push_back(stringstream());
  return tstruct->failures.back();
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
    printf("\e[31m");
    break;
  case GREEN:
    printf("\e[32m");
    break;
  case BLUE:
    printf("\e[34m");
    break;
  case RESET:
    printf("\e[0m");
    break;
  }
#endif
#endif
}

void mtest_thread_init(mtest_thread *m) {
  m->req = -1;

#ifdef _WIN32
#error W32 thread
#else
  pthread_mutex_init(&m->mutex, NULL);
  pthread_create(&m->handle, NULL, mtest_thread_main, (void *)m);
#endif
}

void mtest_lock(mtest_mutex *m) {
#ifdef _WIN32
#error W32 mutex
#else
  pthread_mutex_lock(m);
#endif
}

int mtest_trylock(mtest_mutex *m) {
#ifdef _WIN32
#error W32 mutex
#else
  return !pthread_mutex_trylock(m);
#endif
}

void mtest_unlock(mtest_mutex *m) {
#ifdef _WIN32
#error W32 mutex
#else
  pthread_mutex_unlock(m);
#endif
}

void mtest_join(mtest_thread *m) {
#ifdef _WIN32
#error W32 mutex
#else
  pthread_join(m->handle, NULL);
#endif
}

void *mtest_thread_main(void *ud) {
  mtest_thread *self = (mtest_thread *)ud;

  while (1) {
    mtest_lock(&self->mutex);
    int creq = self->req;
    mtest_unlock(&self->mutex);

    if (self->req == -2) {
      break;
    }

    if (self->req == -1) {
      usleep(BWAIT);
      continue;
    }

    // Run test!
    clock_t start_time = clock();
    all_tests->at(creq).tfun(&all_tests->at(creq));
    clock_t end_time = clock();

    // Acquire output mutex
    mtest_lock(&out_mutex);
    _clear_row();
    cout
      << "    [" << self-threads << "]"
      << " " << setw((int)log10(all_tests->size())) << ++total_tested
      << " / " << all_tests->size()
      << "    " << setw(max_testlen) << all_tests->at(creq).name
      << " ... ";

    if (all_tests->at(creq).failures.size()) {
      _set_color(RED);
      cout << "FAILED ";
      _set_color(RESET);

      ++failed_tests;
    } else {
      _set_color(GREEN);
      cout << "OK     ";
      _set_color(RESET);
    }

    cout << "( " << (end_time - start_time) / (CLOCKS_PER_SEC / 1000) << " ms )" << endl;

    total_failures += all_tests->at(creq).failures.size();
    mtest_unlock(&out_mutex);

    mtest_lock(&self->mutex);
    self->req = -1;
    mtest_unlock(&self->mutex);
  }

  return NULL;
}

void *mtest_status_main(void *ud) {
  int done = 0;
  while (!done) {
    done = 1;
    mtest_lock(&out_mutex);
    _clear_row();
    cout << "[";
    for (int i = 0; i < num_threads; ++i) {
      mtest_lock(&threads[i].mutex);
      int cur = threads[i].req;
      mtest_unlock(&threads[i].mutex);

      if (cur != -2)
        done = 0;

      if (cur == -2) {
        cout << "(joining)";
      } else if (cur == -1) {
        cout << "(idle)";
      } else {
        cout << all_tests->at(cur).name;
      }

      if (i < num_threads - 1) {
        cout << ", ";
      }
    }
    cout << "]";
    cout.flush();
    mtest_unlock(&out_mutex);
    usleep(STATUS_WAIT);
  }

  return NULL;
}

void _clear_row() {
  int width = _get_terminal_width();
  char *clr = (char *)malloc(width + 1);
  memset(clr, ' ', width);
  clr[width] = '\0';
  printf("\r%s\r", clr);
  free(clr);
}

void _cleanup() {
  free(all_tests);
  free(threads);
}

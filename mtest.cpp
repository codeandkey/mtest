// SPDX-License-Identifier: MIT
// vim: set ts=2 sw=2:

#include "mtest.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <stdarg.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <chrono>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>
#include <functional>

using namespace std;

#define RED 0
#define GREEN 1
#define BLUE 2
#define RESET 3

#define BWAIT 10
#define STATUS_WAIT 250

static void mtest_status_main();
static void mtest_thread_main(void *ud);

struct Test
{
  Test(void(*tfun)(void*), const char* name) : tfun(tfun), name(name) {}

  void (*tfun)(void*);
  const char* name;
  vector<stringstream> failures;
};

struct Thread
{
  Thread() : handle(mtest_thread_main, this), req(-1) {}

  void set_req(int val)
  {
    lock_guard<mutex> lock(mut);
    req = val;
  }

  int get_req()
  {
    lock_guard<mutex> lock(mut);
    return req;
  }

  thread handle;
  mutex mut;
  int req;
};

mutex out_mutex;
thread* status_thread;

static vector<Test>* all_tests;
static vector<Thread*> threads;
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
static void _wait(int ms);

int mtest_main(int argc, char **argv)
{
  char datestr[100];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  clock_t tstart_time = clock();

  strftime(datestr, sizeof(datestr) - 1, "%m/%d/%Y %H:%H", t);

  _print_centered_header("TEST RUN (%d total): %s", all_tests->size(), datestr);

  int num_threads = (int) thread::hardware_concurrency();

  if (!num_threads)
  {
    cout << "ERROR: couldn't query thread count" << endl;
    return -1;
  }
  cout << "    > Testing on " << num_threads << " threads" << endl;

  // Determine name alignment
  for (auto &t : *all_tests)
  {
    int len = strlen(t.name);
    if (len > max_testlen) max_testlen = len;
  }

  // Initialize worker threads
  for (int i = 0; i < num_threads; ++i)
    threads.push_back(new Thread());

  // Initialize status thread
  status_thread = new thread(&mtest_status_main);

  for (unsigned int i = 0; i < all_tests->size(); ++i)
  {
    // Wait for free worker
    int assigned = 0;
    while (!assigned)
    {
      for (auto& thr : threads)
        if (thr->mut.try_lock())
        {
          if (thr->req == -1)
          {
            assigned = 1;
            thr->req = i;
            thr->mut.unlock();
            break;
          }
          else
          {
            thr->mut.unlock();
          }
        }

      _wait(BWAIT);
    }
  }

  // Wait for each thread to complete
  int done = 0;
  while (!done)
  {
    done = 1;

    for (auto &thr : threads)
    {
      thr->mut.lock();

      if (thr->req != -1)
      {
        done = 0;
        thr->mut.unlock();
        break;
      }

      thr->mut.unlock();
    }

    _wait(BWAIT);
  }

  // Tell threads to stop
  for (auto &thr : threads)
    thr->set_req(-2);

  // Join remaining threads
  for (auto &thr : threads)
    thr->handle.join();

  // Wait for status thread
  status_thread->join();

  clock_t tend_time = clock();
  _clear_row();

  cout 
    << "    > Finished testing in "
    << setprecision(2)
    << (float)(tend_time - tstart_time) / CLOCKS_PER_SEC
    << " seconds" << endl;

  if (total_failures)
  {
    _print_centered_header("SUMMARY OF %d FAILED TEST%s", failed_tests,
                           (failed_tests > 1) ? "S" : "");

    for (auto &test : *all_tests)
      if (test.failures.size())
      {
        cout << test.name << ":" << endl;

        for (auto &f : test.failures)
          cout << "    " << f.str() << endl;
      }
  }
  else
  {
    _print_centered_header("ALL TESTS PASSED");
  }

  _cleanup();
  return total_failures ? -1 : 0;
}

int _mtest_push(const char* name, void (*tfun)(void*))
{
  if (!all_tests)
    all_tests = new vector<Test>();

  all_tests->push_back(Test(tfun, name));

  return all_tests->size();
}

std::ostream& _mtest_fail(void *self)
{
  Test *tstruct = (Test *)self;
  tstruct->failures.push_back(stringstream());
  return tstruct->failures.back();
}

int _get_terminal_width()
{
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

char *_print_into_buf(const char *fmt, va_list ap)
{
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

void _print_centered_header(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  char* buf = _print_into_buf(fmt, ap);
  va_end(ap);

  int len = strlen(buf);
  int tsize = _get_terminal_width();
  int padding = (tsize - (len + 2)) / 2;

  char *pstr = (char *)malloc(padding + 1);
  memset(pstr, '=', padding);
  pstr[padding] = '\0';

  printf("%s %s %s", pstr, buf, pstr);

  if (padding * 2 + len + 2 < tsize)
    printf("=");

  printf("\n");

  free(buf);
  free(pstr);
}

void _set_color(int col)
{
#ifndef MTEST_NOCOLOR
#ifdef _WIN32
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);

  switch (col)
  {
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
    SetConsoleTextAttribute(con, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    break;
  }
#else
  switch (col)
  {
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

void mtest_thread_main(void *ud)
{
  Thread* self = (Thread*) ud;

  while (1)
  {
    int creq = self->get_req();

    if (self->req == -2)
      break;

    if (self->req == -1)
    {
      _wait(BWAIT);
      continue;
    }

    // Run test!
    clock_t start_time = clock();
    all_tests->at(creq).tfun(&all_tests->at(creq));
    clock_t end_time = clock();

    // Acquire output mutex
    out_mutex.lock();
    _clear_row();

    cout
      << "    " << setw((int)log10(all_tests->size()) + 1) << ++total_tested
      << " / " << all_tests->size()
      << "    " << setw(max_testlen) << all_tests->at(creq).name
      << " ... ";

    if (all_tests->at(creq).failures.size())
    {
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
    out_mutex.unlock();

    self->set_req(-1);
  }
}

void mtest_status_main() {
  int done = 0;
  while (!done)
  {
    done = 1;
    out_mutex.lock();
    _clear_row();
    cout << "[";

    for (auto &thr : threads)
    {
      int cur = thr->get_req();

      if (cur != -2)
        done = 0;

      if (cur == -2)
        cout << "(joining)";
      else if (cur == -1)
        cout << "(idle)";
      else
        cout << all_tests->at(cur).name;

      if (thr != threads.back())
        cout << ", ";
    }

    cout << "]";
    cout.flush();
    out_mutex.unlock();
    _wait(STATUS_WAIT);
  }
}

void _clear_row()
{
  int width = _get_terminal_width();
  char *clr = (char *)malloc(width + 1);
  memset(clr, ' ', width);
  clr[width] = '\0';
  printf("\r%s\r", clr);
  free(clr);
}

void _cleanup()
{
  delete all_tests;

  for (auto& t : threads)
    delete t;

  delete status_thread;
}

void _wait(int ms)
{
  this_thread::sleep_for(chrono::milliseconds(ms));
}

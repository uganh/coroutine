#pragma once

#include <functional>

#ifdef _MSC_VER
#include <Windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
#define _XOPEN_SOURCE
#include <ucontext.h>
#else
#include <ucontext.h>
#endif

enum Status { SUSPENDED, RUNNING, NORMAL, DEAD };

class Coroutine {
  Status state;
  std::function<void(void)> func;
  Coroutine *co_caller;
  ucontext_t ctx;

  static thread_local Coroutine co_main, *co_curr;

  static void entry(void);

  explicit Coroutine(std::function<void(void)> &&func = nullptr);

public:
  static Coroutine *create(std::function<void(void)> &&func) {
    if (func) {
      return new Coroutine(std::move(func));
    }
    return nullptr;
  }

  static void yield(void);

  static Coroutine *running(void) {
    thread_local Coroutine co_main;
    thread_local Coroutine *co_curr = &co_main;
    return co_curr;
  }

  ~Coroutine(void) noexcept {
    if (func) {
      void *stack = ctx.uc_stack.ss_sp;
      if (stack != nullptr) {
        ::operator delete(stack);
      }
    }
  }

  bool resume(void);

  Status status(void) const {
    return state;
  }
};

#pragma once

#include <functional>
#include <future>

#ifdef _MSC_VER
# include <Windows.h>
#else
# if defined(__APPLE__) && defined(__MACH__)
#  define _XOPEN_SOURCE
# endif
# include <ucontext.h>
#endif

enum Status { SUSPENDED, RUNNING, NORMAL, DEAD };

class Coroutine {
  Status state;
  std::function<void(void)> func;
  Coroutine *co_caller;
#ifdef _MSC_VER
  LPVOID fiber;
#else
  void *stack;
  ucontext_t ctx;
#endif

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
    return co_curr;
  }

  ~Coroutine(void) noexcept {
#ifdef _MSC_VER
    if (func && fiber) {
      DeleteFiber(fiber);
    }
#else
    if (func && stack) {
      ::operator delete(stack);
    }
#endif
  }

  bool resume(void);

  Status status(void) const {
    return state;
  }
};

template <typename FunctionTy>
typename std::result_of<FunctionTy()>::type await(FunctionTy &&func) {
  auto future = std::async(std::launch::async, func);
  // std::future_status status = future.wait_for(std::)
}
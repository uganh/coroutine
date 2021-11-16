#pragma once

#include <functional>
#include <future>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
# include <Windows.h>
# define CO_ENTRY __stdcall
# define CO_ENTRY_PARAM LPVOID
#else
# if defined(__APPLE__) && defined(__MACH__)
#  define _XOPEN_SOURCE
# endif
# include <ucontext.h>
# define CO_ENTRY
# define CO_ENTRY_PARAM void
#endif

enum Status { SUSPENDED, RUNNING, NORMAL, DEAD };

class Coroutine {
  unsigned int _status;

#ifdef _MSC_VER
  LPVOID fiber;
#else
  void *stack;
  ucontext_t ctx;
#endif

  Coroutine *co_caller;

  void *raw_fn_args;
  void (*invoke)(void *);

  static thread_local Coroutine co_main, *co_curr;

  static void CO_ENTRY co_entry(CO_ENTRY_PARAM);

  template <typename Tuple, size_t... indices>
  static void _Invoke(void *raw_fn_args) noexcept {
    std::unique_ptr<Tuple> fn_args(reinterpret_cast<Tuple *>(raw_fn_args));
    std::invoke(std::move(std::get<indices>(*fn_args))...);
  }

  template <class Tuple, size_t... indices>
  static constexpr auto _Get_invoke(std::index_sequence<indices...>) noexcept {
    return &_Invoke<Tuple, indices...>;
  }

  /* Create main coroutine */
  Coroutine(void)
    : _status(RUNNING)
#ifdef _MSC_VER
    , fiber(ConvertThreadToFiber(nullptr))
#else
    , stack(nullptr)
#endif
    , co_caller(nullptr)
    , raw_fn_args(nullptr)
    , invoke(nullptr) {}

  template <typename Fn, typename... Args>
  explicit Coroutine(Fn &&Fx, Args &&...Ax)
    : _status(SUSPENDED)
#ifdef _MSC_VER
    , fiber(nullptr)
#else
    , stack(nullptr)
#endif
    , co_caller(nullptr) {
    using Tuple = std::tuple<std::decay_t<Fn>, std::decay_t<Args>...>;
    raw_fn_args = new Tuple(std::forward<Fn>(Fx), std::forward<Args>(Ax)...);
    invoke = _Get_invoke<Tuple>(std::make_index_sequence<1 + sizeof...(Args)>{});
  }

public:
  template <typename Fn, typename... Args>
  static Coroutine *Create(Fn &&Fx, Args &&...Ax) {
    return new Coroutine(std::forward<Fn>(Fx), std::forward<Args>(Ax)...);
  }

  static void yield(void);

  static Coroutine *running(void) {
    return co_curr;
  }

  ~Coroutine(void) noexcept {
#ifdef _MSC_VER
    if (raw_fn_args && fiber) {
      DeleteFiber(fiber);
    }
#else
    if (stack) {
      ::operator delete(stack);
    }
#endif
  }

  bool resume(void);

  unsigned int status(void) const {
    return _status;
  }
};

//template <typename FntionTy>
//typename std::result_of<FntionTy()>::type await(FntionTy &&func) {
//  auto future = std::async(std::launch::async, func);
//  // std::future_status status = future.wait_for(std::)
//}

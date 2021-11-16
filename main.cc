#include <cassert>
#include <iostream>

#include "Coroutine.h"

#ifdef NDEBUG
# define CHECK(expr) if (expr) (void) 0
#else
# define CHECK(expr) assert(expr)
#endif

void co_add(int &r, int a, int b) {
  r = a + b;
}

int main(void) {
  Coroutine *co_main = Coroutine::running();

  Coroutine *co = Coroutine::Create([&] {
    CHECK(co == Coroutine::running());
    CHECK(co_main->status() == NORMAL);
    CHECK(co->status() == RUNNING);
    std::cout << "Hello" << std::flush;
    Coroutine::yield();
    std::cout << "world" << std::endl;
  });

  int r = 0;
  Coroutine *co2 = Coroutine::Create(co_add, std::ref(r), 1, 2);
  CHECK(r == 0);
  co2->resume();
  CHECK(r == 3);

  CHECK(co->status() == SUSPENDED);
  CHECK(co->resume());

  std::cout << ", ";

  CHECK(co->status() == SUSPENDED);
  CHECK(co->resume());

  CHECK(co->status() == DEAD);

  CHECK(co_main == Coroutine::running());
  CHECK(co_main->status() == RUNNING);

  return 0;
}

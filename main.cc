#include <cassert>
#include <iostream>

#include "Coroutine.h"

int main(void) {
  Coroutine *co_main = Coroutine::running();

  Coroutine *co = Coroutine::create([&] {
    assert(co_main->status() == NORMAL);
    assert(co->status() == RUNNING);
    std::cout << "Hello, " << std::flush;
    Coroutine::yield();
    std::cout << "world" << std::endl;
  });

  assert(co->status() == SUSPENDED);
  assert(co->resume());
  assert(co->status() == SUSPENDED);
  assert(co->resume());
  assert(co->status() == DEAD);

  assert(co_main == Coroutine::running());
  assert(co_main->status() == RUNNING);

  return 0;
}

#include <stdlib.h>
#include "libfoo.h"

int main()
{
  Foo *f = CreateFoo();
  if (f)
  {
    f->meth1();
    f->meth2("hello");
    f->meth3(10, "hello");
    f->meth4(10, "hello", 'x');
    f->~Foo();
  //  free(f);
  }
  return 0;
}

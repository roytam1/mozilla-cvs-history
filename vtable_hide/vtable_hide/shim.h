#ifndef shim_h__
#define shim_h__

struct Shim
{
  void *vptr;
  char *data;
  int   length;
  int   flags;
};

void ShimCtor(Shim *);
void ShimDtor(Shim *);
void ShimFoo(Shim *, int x);
void ShimBar(Shim *, int x, const char *s);

#endif

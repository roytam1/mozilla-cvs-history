/* vim:set ts=2 sw=2 et cindent: */

#ifndef LIBFOO_H__
#define LIBFOO_H__

class Foo
{
public:
  virtual ~Foo() { }
  virtual int meth1() = 0;
  virtual int meth2(const char *) = 0;
  virtual int meth3(int, const char *) = 0;
  virtual int meth4(int, const char *, char) = 0;
};

extern "C" Foo * CreateFoo();

#endif

#ifndef inner_h__
#define inner_h__

class Inner
{
  public:
    virtual ~Inner() {}
    virtual void foo(int x) = 0;
    virtual void bar(int x, const char *s) = 0;
};

#endif

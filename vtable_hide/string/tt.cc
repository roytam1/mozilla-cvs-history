
class A
{
public:
  A(int x, int y, int z)
    : _x(x), _y(y), _z(z) {}
private:
  int _x, _y, _z;
};

class B
{
public:
  B(int x, int y, int z)
    { _x = x;
      _z = z;
      _y = y;
    }
private:
  int _x, _y, _z;
};

int main()
{
  A a(1, 2, 3);
  B b(1, 2, 3);
  return 0;
}

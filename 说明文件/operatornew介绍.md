# operator new
## 一、功能
### 1.分配存储空间
关键字new内含2阶段:
– 调用operator new分配内存
– 调用构造函数构造对象

### 2.函数原型
```cpp
void* operator new (std::size_t size);
void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept;
void* operator new (std::size_t size, void* ptr) noexcept;
```

首先介绍一下第一个函数原型:
```cpp
void* operator new (std::size_t size);
```
分配size大小字节的存储空间，表示该大小的任何对象，并返回指向该块的第一个字节的非空指针。如果失败，它会抛出一个bad_alloc异常。
实战重写operator new函数
```cpp
#include <iostream>
using namespace std;

void* operator new (size_t size){
  cout << size << endl;
  void* t = (void*)malloc(size); //申请内存

        return t;
}

void* operator new[](size_t size){
  cout << size << endl;
  void* t = (void*)malloc(size); //申请内存
  return t;
}

int main()
{
  char *c = new char;
  short* s = new short;
  int *i = new int;
  double* d = new double;

  int* pi = new int[10];
  char* pc = new char[20];

  return 0;
}

```

输出:
```bash
1
2
4
8
40
20

```

**总结** C++中关键字new的作用，使用关键字new会先调用operator new函数，其中size的计算法方法是sizeof + new关键字后面的值(char/short/int/double)等。并且申请内存知识operatornew做的事情，对象初始化还要调用构造函数。

## 二、调用重载的new
### 1.new关键字默认调用
```cpp
void* operator new(size_t size); //new关键字的默认调用
```
### 2.调用重载operator new
```cpp
#define new new(__func__, __LINE__)
int* p = new int;
```
上面经过预处理后
```cpp
int* p = new(__func__, __LINE__)int; //多样化自主调用
```
然后关键字这种形式的new会去找operator new(size_t size, char* s, size_t no);

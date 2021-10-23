# operator delete
## 一、功能
### 1.释放内存空间
关键字delete内含2阶段操作：
– 调用析构函数析构对象
– 调用::operator delete释放内存
### 2.函数原型
```cpp
void operator delete (void* ptr) noexcept;
void operator delete (void* ptr, const std::nothrow_t& nothrow_constant) noexcept;
void operator delete (void* ptr, void* voidptr2) noexcept;
```
首先介绍一下第一个函数原型
```cpp
void operator delete (void* ptr) noexcept;
```
实战重写operator delete
```cpp
#include <iostream>
using namespace std;

void operator delete(void* ptr){
  free(ptr);
  cout << __func__ << endl;
}

int main()
{
  int *p = new int;
  delete p;
  return 0;
}
```
输出:
```bash
operator delete
```
**总结**: 释放ptr指向的内存块（如果不为空），释放之前通过调用分配给它的存储空间新运营商 并使该指针位置无效。

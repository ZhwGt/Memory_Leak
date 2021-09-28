#include <iostream>
#include <memory>

#include "LeakDetector.hpp"

//测试异常分支泄露
class Err {
public:
    Err(int n) {
        if(n == 0) throw 1000;
        data = new int[n];
    }
    ~Err() {
        delete[] data;
    }
private:
    int *data;
};

// 循环引用数据
class A;
class B;
class A {
public:
    std::shared_ptr<B> p;
};
class B {
public:
    std::shared_ptr<A> p;
};

int main() {

    // 忘记释放指针 b 申请的内存，从而导致内存泄露
    int *a = new int;
    int *b = new int;
    delete a;

    // 0作为参数传递给构造函数将发生异常，从而导致异常分支的内存泄露
    try {
        Err* e = new Err(0);
        delete e;
    } catch (int &ex) {
        std::cout << "Exception catch: " << ex << std::endl;
    };
  
    // 测试循环引用
    auto smartA = std::make_shared<A>();
    auto smartB = std::make_shared<B>();
    smartA->p = smartB;
    smartB->p = smartA;
    
    return 0;
}

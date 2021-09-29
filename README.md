# 内存泄漏检测器

### 项目介绍
内存泄漏一直是 C++ 中比较令人头大的问题， 即便是很有经验的 C++程序员有时候也难免因为疏忽而写出导致内存泄漏的代码。除了基本的申请过的内存未释放外，还存在诸如异常分支导致的内存泄漏等等。本项目将使用 C++ 实现一个内存泄漏检查器。

### 内存泄漏检测器设计背景
通常是由于疏忽而导致的申请内存未释放。在现代操作系统中，一个应用程序使用的常规内存终止后这些未释放的内存仍然会被操作系统回收，所以一个短暂运行的应用程序而导致的内存泄漏不会造成比较严重的后果。但是，如果我们在编写诸如服务器一类的应用时，它会始终保持运行状态，如果一旦存在发生内存泄露的逻辑，将很有可能继续造成泄露的内存增加，从而严重降低计算机性能，甚至导致系统运行故障等。要检测一个长期运行的程序是否发生内存泄露通常是在应用中设置检查点，分析不同检查点中内存是否长期保持占用而未释放，从本质上来说，这与对一个短期运行的程序进行内存泄露检查是非常类似的。所以本项目中的内存泄漏检查器将实现一个用于短期内存泄露的检查器，在此基础上还可以拓展出检测内存，在程序运行过程中检测每个子任务，将内存情况输出到日志文件，方便堆整个程序的内存进行监控。

### 设计思想
要实现内存泄露的检查，可以从内存的申请和结束的角度考虑：
* 内存泄露产生于new操作进行后没有执行delete。
* 最先被创建的对象，其析构函数永远是最后执行。

### new关键字操作的步骤
1.执行operator new函数。

2.placement new调用构造函数。

3.返回内存指针。

### 使用双向链表管理内存块
* 可能有频繁的申请（插入操作）和销毁（删除操作）。
* 保留原有的物理顺序。
* 没有空间限制,存储元素无上限,只与内存空间大小有关。
* 动态分配内存空间，不用事先开辟内存。
* 内存的利用率变高。

### 检测效果
![内存泄漏检测器](https://user-images.githubusercontent.com/51261084/135281379-e658d4e4-bc86-41c8-9a65-86b77b5bf196.png)

### 设计步骤
重载 new 运算符
创建一个静态对象，用于在原始程序退出时候才调用这个静态对象的析构函数
这样两个步骤的好处在于：无需修改原始代码的情况下，就能进行内存检查。这同时也是我们希望看到的。所以，我们可以在LeakDetector.hpp 里首先实现：
```cpp
#ifndef __LEAK_DETECTOR__
#define __LEAK_DETECTOR__

void* operator new(size_t _size, char *_file, unsigned int _line);
void* operator new[](size_t _size, char *_file, unsigned int _line);

#ifndef __NEW_OVERLOAD_IMPLEMENTATION__
#define new    new(__FILE__, __LINE__)
#endif

class _leak_detector
{
public:
    static unsigned int callCount;
    _leak_detector() noexcept {
        ++callCount;
    }
    ~_leak_detector() noexcept {
        if (--callCount == 0)
            LeakDetector();
    }
private:
    static unsigned int LeakDetector() noexcept;
};
static _leak_detector _exit_counter;
#endif
```
为什么要设计 callCount? callCount 保证了我们的 LeakDetector 只调用了一次(智能指针计数原理类似)。

既然我们已经重载了 new 操作符，那么我们很自然就能想到通过手动管理内存申请和释放，如果我们 delete 时没有将申请的内存全部释放完毕，那么一定发生了内存泄露。接下来一个问题就是，使用什么结构来实现手动管理内存？

不妨使用双向链表来实现内存泄露检查。原因在于，对于内存检查器来说，并不知道实际代码在什么时候会需要申请内存空间，所以使用线性表并不够合理，一个动态的结构（链表）是非常便捷的。而我们在删除内存检查器中的对象时，需要更新整个结构，对于单向链表来说，也是不够便利的。
```cpp
#include <iostream>
#include <cstring>

#define __NEW_OVERLOAD_IMPLEMENTATION__
#include "LeakDetector.hpp"
#include "MemNode.hpp"

static unsigned long mallocated = 0;  // 保存未释放的内存大小

static MemNode root = {
    &root, &root, // 第一个元素的前向后向指针均指向自己
    0, false,               // 其申请的内存大小为 0, 且不是数组
    NULL, 0                 // 文件指针为空, 行号为0
};

unsigned int _leak_detector::callCount = 0;

void* AllocateMemory(size_t size, bool array, char *file, unsigned line) {
    size_t newSize = sizeof(MemNode) + size;

    MemNode *node = (MemNode*)malloc(newSize);

    node->ne = root.ne;
    node->pre = &root;
    node->size = size;
    node->isArray = array;
    node->file = NULL;

    if (file) {
        node->file = (char *)malloc(strlen(file)+1);
        strcpy(node->file, file);
    }
    node->line = line;

    root.ne->pre = node;
    root.ne = node;

    mallocated += size;

    return (char*)node + sizeof(MemNode);
}


void  DeleteMemory(void* _ptr, bool array) {
    // 返回 MemoryList 开始处
    MemNode *curNode = (MemNode *)((char *)_ptr - sizeof(MemNode));

    if (curNode->isArray != array) return;

    // 更新列表
    curNode->pre->ne = curNode->ne;
    curNode->ne->pre = curNode->pre;
    mallocated -= curNode->size;

    // 记得释放存放文件信息时申请的内存
    if (curNode->file) free(curNode->file);
    free(curNode);
}

// 重载 new 运算符
void* operator new(size_t size) {
	std::cout << "operator new(size_t size)" << std::endl;
    return AllocateMemory(size, false, NULL, 0);
}
void* operator new[](size_t size) {
	std::cout << "operator new[](size_t size)" << std::endl;
    return AllocateMemory(size, true, NULL, 0);
}
void* operator new(size_t size, char *file, unsigned int line) {
	std::cout << "operator new(size_t size, char *file, unsigned int line) " << std::endl;
    return AllocateMemory(size, false, file, line);
}
void* operator new[](size_t size, char *file, unsigned int line) {
	std::cout << "operator new(size_t size, char *file, unsigned int line)" << std::endl;
    return AllocateMemory(size, true, file, line);
}
// 重载 delete 运算符
void operator delete(void *_ptr) noexcept {
	std::cout << "operator delete(void *_ptr)" << std::endl;
    DeleteMemory(_ptr, false);
}
void operator delete[](void *_ptr) noexcept {
	std::cout << "operator delete(void *_ptr)" << std::endl;
    DeleteMemory(_ptr, true);
}

//
//  LeakDetector.cpp
//  LeakDetector
//
unsigned int _leak_detector::LeakDetector(void) noexcept {
    unsigned int count = 0;
    // 遍历整个列表, 如果有内存泄露，那么 _LeakRoot.ne 总不是指向自己的
    MemNode *ptr = root.ne;
    while (ptr && ptr != &root)
    {
        // 输出存在内存泄露的相关信息, 如泄露大小, 产生泄露的位置
        if(ptr->isArray) std::cout << "leak[] ";
        else std::cout << "leak   ";

		std::cout << ptr << " size " << ptr->size;

		if (ptr->file) std::cout << " (locate in " << ptr->file << " line " << ptr->line << ")";
        else std::cout << " (Cannot find position)";
        std::cout << std::endl;

        ++count;
        ptr = ptr->ne;
    }

    if (count) std::cout << "Total " << count << " leaks, size "<< mallocated << " byte." << std::endl;
    return count;
}
```
内存节点关系信息
```cpp
#ifndef __MEMNODE_H__
#define __MEMNODE_H__

struct MemNode {
     struct MemNode *ne, *pre;
     size_t size;       // 申请内存的大小
     bool isArray;    // 是否申请了数组
     char *file;      // 存储所在文件
     unsigned int line;  // 保存所在行
};

#endif
```

主函数中的测试代码
```cpp
#include <iostream>

// 在这里实现内存泄露检查
#include "LeakDetector.hpp"

// 测试异常分支泄露
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

int main() {

    // 忘记释放指针 b 申请的内存，从而导致内存泄露
    int *a = new int;
    int *b = new int;
    delete a;

    // 0 作为参数传递给构造函数将发生异常，从而导致异常分支的内存泄露
    try {
        Err* e = new Err(0);
        delete e;
    } catch (int &ex) {
        std::cout << "Exception catch: " << ex << std::endl;
    };
    return 0;
}
```

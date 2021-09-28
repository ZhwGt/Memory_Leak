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

// 重载operator new
void* operator new(size_t size) {
    return AllocateMemory(size, false, NULL, 0);
}
void* operator new[](size_t size) {
    return AllocateMemory(size, true, NULL, 0);
}
void* operator new(size_t size, char *file, unsigned int line) {
    return AllocateMemory(size, false, file, line);
}
void* operator new[](size_t size, char *file, unsigned int line) {
    return AllocateMemory(size, true, file, line);
}
// 重载delete
void operator delete(void *_ptr) noexcept {
    DeleteMemory(_ptr, false);
}
void operator delete[](void *_ptr) noexcept {
    DeleteMemory(_ptr, true);
}

//
//  LeakDetector.cpp
//  LeakDetector
//
unsigned int _leak_detector::LeakDetector(void) noexcept {
    unsigned int count = 0;
    // 遍历整个列表, 检查内存是都泄漏
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

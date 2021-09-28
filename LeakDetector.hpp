#ifndef __LEAK_DETECTOR__
#define __LEAK_DETECTOR__

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

void* operator new(size_t size, char *file, unsigned line);
void* operator new[](size_t size, char *file, unsigned int line);

#ifndef __NEW_OVERLOAD_IMPLEMENTATION__
#define new new(__FILE__, __LINE__)
#endif

#endif

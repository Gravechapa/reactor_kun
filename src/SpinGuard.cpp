#include "SpinGuard.hpp"

SpinGuard::SpinGuard(std::atomic_flag &flag) : _flag(flag)
{
    lock();
}

SpinGuard::~SpinGuard()
{
    unlock();
}

void SpinGuard::lock()
{
    if (!_owned)
    {
        while (_flag.test_and_set(std::memory_order_acquire))
            ;
        _owned = true;
    }
}

void SpinGuard::unlock()
{
    if (_owned)
    {
        _flag.clear(std::memory_order_release);
        _owned = false;
    }
}

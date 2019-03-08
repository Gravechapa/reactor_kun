#pragma once
#include <atomic>

class SpinGuard
{
public:
    SpinGuard(std::atomic_flag &flag);
    ~SpinGuard();

    void lock();
    void unlock();
private:
    bool _owned{false};
    std::atomic_flag &_flag;

    SpinGuard(const SpinGuard&) = delete;
    const SpinGuard& operator=(const SpinGuard&) = delete;
    SpinGuard(const SpinGuard&&) = delete;
    const SpinGuard& operator=(const SpinGuard&&) = delete;
};

//==============================================================================
// Atomic.h
// Lock-free atomic utilities for real-time audio
//==============================================================================

#pragma once

#include <atomic>
#include <concepts>

// SIMD intrinsics for pause instruction
#if defined(__x86_64__) || defined(_M_X64)
    #include <emmintrin.h>  // For _mm_pause()
#endif

namespace Omega::Utils {

//==============================================================================
// Atomic value wrapper with relaxed memory ordering for audio thread
// Use this for non-critical updates (e.g., metering, visualization)
//==============================================================================
template<typename T>
    requires std::is_trivially_copyable_v<T> && std::is_copy_assignable_v<T>
class RelaxedAtomic {
public:
    RelaxedAtomic() noexcept = default;
    explicit RelaxedAtomic(T value) noexcept : value_(value) {}
    
    // No copy/move (atomics are not copyable)
    RelaxedAtomic(const RelaxedAtomic&) = delete;
    RelaxedAtomic& operator=(const RelaxedAtomic&) = delete;
    
    // Store with relaxed ordering (audio thread writes)
    void store(T value) noexcept {
        value_.store(value, std::memory_order_relaxed);
    }
    
    // Load with acquire ordering (GUI thread reads)
    T load() const noexcept {
        return value_.load(std::memory_order_acquire);
    }
    
    // Exchange
    T exchange(T value) noexcept {
        return value_.exchange(value, std::memory_order_acq_rel);
    }
    
private:
    std::atomic<T> value_{};
};

//==============================================================================
// Atomic flag for simple state communication
//==============================================================================
class AtomicFlag {
public:
    AtomicFlag() noexcept = default;
    explicit AtomicFlag(bool initialState) noexcept : flag_(initialState) {}
    
    void set() noexcept {
        flag_.store(true, std::memory_order_release);
    }
    
    void clear() noexcept {
        flag_.store(false, std::memory_order_release);
    }
    
    bool isSet() const noexcept {
        return flag_.load(std::memory_order_acquire);
    }
    
    bool testAndSet() noexcept {
        return flag_.exchange(true, std::memory_order_acq_rel);
    }
    
private:
    std::atomic<bool> flag_{false};
};

//==============================================================================
// Spin lock for extremely short critical sections (use sparingly!)
// WARNING: Only use when you're 100% sure the lock is held for < 100 ns
//==============================================================================
class SpinLock {
public:
    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // Spin with pause instruction (x86) or yield (ARM)
            #if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
            #elif defined(__arm__) || defined(__aarch64__)
                __asm__ __volatile__("yield");
            #endif
        }
    }
    
    void unlock() noexcept {
        flag_.clear(std::memory_order_release);
    }
    
    bool tryLock() noexcept {
        return !flag_.test_and_set(std::memory_order_acquire);
    }
    
private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

//==============================================================================
// RAII guard for spin lock
//==============================================================================
class SpinLockGuard {
public:
    explicit SpinLockGuard(SpinLock& lock) noexcept : lock_(lock) {
        lock_.lock();
    }
    
    ~SpinLockGuard() noexcept {
        lock_.unlock();
    }
    
    SpinLockGuard(const SpinLockGuard&) = delete;
    SpinLockGuard& operator=(const SpinLockGuard&) = delete;
    
private:
    SpinLock& lock_;
};

} // namespace Omega::Utils

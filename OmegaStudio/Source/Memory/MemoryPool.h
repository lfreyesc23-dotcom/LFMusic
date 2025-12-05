//==============================================================================
// MemoryPool.h
// Custom memory allocator for the audio thread
// Prevents heap fragmentation and eliminates malloc() in real-time context
//==============================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <cassert>
#include "../Utils/Atomic.h"

namespace Omega::Memory {

//==============================================================================
// MemoryPool - Pre-allocated block allocator for real-time audio
//
// WHY: malloc() can block for unbounded time (not real-time safe)
// SOLUTION: Pre-allocate a large chunk and hand out fixed-size blocks
//
// Usage:
//   MemoryPool pool(16 * 1024 * 1024, 64);  // 16MB, 64-byte blocks
//   void* ptr = pool.allocate();
//   // ... use memory ...
//   pool.deallocate(ptr);
//==============================================================================
class MemoryPool {
public:
    //==========================================================================
    // Constructor: Allocates the entire pool upfront
    //==========================================================================
    MemoryPool(size_t poolSize, size_t blockSize);
    ~MemoryPool();
    
    // Non-copyable, movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;
    
    //==========================================================================
    // Allocate a block (real-time safe, lock-free)
    // Returns nullptr if pool is exhausted
    //==========================================================================
    [[nodiscard]] void* allocate() noexcept;
    
    //==========================================================================
    // Deallocate a block (real-time safe, lock-free)
    //==========================================================================
    void deallocate(void* ptr) noexcept;
    
    //==========================================================================
    // Query functions
    //==========================================================================
    [[nodiscard]] size_t getBlockSize() const noexcept { return blockSize_; }
    [[nodiscard]] size_t getTotalBlocks() const noexcept { return numBlocks_; }
    [[nodiscard]] size_t getUsedBlocks() const noexcept;
    [[nodiscard]] size_t getFreeBlocks() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] bool isFull() const noexcept;
    
    //==========================================================================
    // Reset the pool (NOT real-time safe - only call during initialization)
    //==========================================================================
    void reset();
    
private:
    //==========================================================================
    // Free list node (stored in the free blocks themselves)
    //==========================================================================
    struct FreeNode {
        FreeNode* next;
    };
    
    //==========================================================================
    // Data members
    //==========================================================================
    std::unique_ptr<uint8_t[]> memory_;      // The entire memory pool
    FreeNode* freeList_;                      // Head of free list
    size_t blockSize_;                        // Size of each block
    size_t numBlocks_;                        // Total number of blocks
    Utils::SpinLock lock_;                    // Minimal lock for free list
    
    //==========================================================================
    // Statistics (atomic for thread-safe reading)
    //==========================================================================
    std::atomic<size_t> allocatedBlocks_{0};
    std::atomic<size_t> peakUsage_{0};
};

//==============================================================================
// RAII wrapper for pool-allocated memory
//==============================================================================
template<typename T>
class PoolAllocated {
public:
    PoolAllocated(MemoryPool& pool) : pool_(pool), ptr_(nullptr) {
        void* mem = pool_.allocate();
        if (mem != nullptr) {
            ptr_ = new (mem) T();  // Placement new
        }
    }
    
    template<typename... Args>
    PoolAllocated(MemoryPool& pool, Args&&... args) : pool_(pool), ptr_(nullptr) {
        void* mem = pool_.allocate();
        if (mem != nullptr) {
            ptr_ = new (mem) T(std::forward<Args>(args)...);
        }
    }
    
    ~PoolAllocated() {
        if (ptr_ != nullptr) {
            ptr_->~T();  // Explicit destructor call
            pool_.deallocate(ptr_);
        }
    }
    
    // Non-copyable, movable
    PoolAllocated(const PoolAllocated&) = delete;
    PoolAllocated& operator=(const PoolAllocated&) = delete;
    
    PoolAllocated(PoolAllocated&& other) noexcept 
        : pool_(other.pool_), ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    PoolAllocated& operator=(PoolAllocated&& other) noexcept {
        if (this != &other) {
            if (ptr_ != nullptr) {
                ptr_->~T();
                pool_.deallocate(ptr_);
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    [[nodiscard]] T* get() noexcept { return ptr_; }
    [[nodiscard]] const T* get() const noexcept { return ptr_; }
    [[nodiscard]] T* operator->() noexcept { return ptr_; }
    [[nodiscard]] const T* operator->() const noexcept { return ptr_; }
    [[nodiscard]] T& operator*() noexcept { return *ptr_; }
    [[nodiscard]] const T& operator*() const noexcept { return *ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }
    
private:
    MemoryPool& pool_;
    T* ptr_;
};

} // namespace Omega::Memory

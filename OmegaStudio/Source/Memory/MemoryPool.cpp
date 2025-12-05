//==============================================================================
// MemoryPool.cpp
// Implementation of the lock-free memory pool
//==============================================================================

#include "MemoryPool.h"
#include <algorithm>
#include <cstring>

namespace Omega::Memory {

//==============================================================================
MemoryPool::MemoryPool(size_t poolSize, size_t blockSize)
    : blockSize_(std::max(blockSize, sizeof(FreeNode)))
    , freeList_(nullptr)
    , numBlocks_(poolSize / blockSize_)
{
    // Allocate the entire pool
    memory_ = std::make_unique<uint8_t[]>(numBlocks_ * blockSize_);
    
    // Initialize the free list
    reset();
}

//==============================================================================
MemoryPool::~MemoryPool() = default;

//==============================================================================
void* MemoryPool::allocate() noexcept {
    Utils::SpinLockGuard guard(lock_);
    
    if (freeList_ == nullptr) {
        return nullptr;  // Pool exhausted
    }
    
    // Pop from free list
    FreeNode* node = freeList_;
    freeList_ = node->next;
    
    // Update statistics
    size_t allocated = ++allocatedBlocks_;
    size_t peak = peakUsage_.load(std::memory_order_relaxed);
    while (allocated > peak) {
        peakUsage_.compare_exchange_weak(peak, allocated, 
                                        std::memory_order_relaxed);
    }
    
    return static_cast<void*>(node);
}

//==============================================================================
void MemoryPool::deallocate(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }
    
    // Verify pointer is within pool bounds (debug only)
    assert(ptr >= memory_.get() && 
           ptr < memory_.get() + (numBlocks_ * blockSize_));
    
    Utils::SpinLockGuard guard(lock_);
    
    // Push back to free list
    auto* node = static_cast<FreeNode*>(ptr);
    node->next = freeList_;
    freeList_ = node;
    
    --allocatedBlocks_;
}

//==============================================================================
size_t MemoryPool::getUsedBlocks() const noexcept {
    return allocatedBlocks_.load(std::memory_order_relaxed);
}

//==============================================================================
size_t MemoryPool::getFreeBlocks() const noexcept {
    return numBlocks_ - getUsedBlocks();
}

//==============================================================================
bool MemoryPool::isEmpty() const noexcept {
    return getUsedBlocks() == 0;
}

//==============================================================================
bool MemoryPool::isFull() const noexcept {
    return getUsedBlocks() == numBlocks_;
}

//==============================================================================
void MemoryPool::reset() {
    Utils::SpinLockGuard guard(lock_);
    
    // Clear all memory
    std::memset(memory_.get(), 0, numBlocks_ * blockSize_);
    
    // Rebuild free list
    freeList_ = nullptr;
    for (size_t i = 0; i < numBlocks_; ++i) {
        auto* node = reinterpret_cast<FreeNode*>(memory_.get() + (i * blockSize_));
        node->next = freeList_;
        freeList_ = node;
    }
    
    allocatedBlocks_.store(0, std::memory_order_relaxed);
    peakUsage_.store(0, std::memory_order_relaxed);
}

} // namespace Omega::Memory

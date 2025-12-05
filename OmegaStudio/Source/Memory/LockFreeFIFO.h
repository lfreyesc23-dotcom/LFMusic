//==============================================================================
// LockFreeFIFO.h
// Lock-free single-producer single-consumer FIFO for audio/GUI communication
// This is THE key component for passing messages without blocking the audio thread
//==============================================================================

#pragma once

#include <atomic>
#include <array>
#include <optional>
#include <concepts>
#include <cstring>

namespace Omega::Memory {

//==============================================================================
// Lock-Free FIFO (Single Producer, Single Consumer)
// 
// CRITICAL: This FIFO is lock-free and wait-free, making it safe for the
// audio thread. No malloc, no mutex, no system calls.
//
// Typical usage:
// - Audio Thread (Producer): Push audio events, parameter changes, meters
// - GUI Thread (Consumer): Pop and display/process these events
//==============================================================================
template<typename T, size_t Capacity>
    requires std::is_trivially_copyable_v<T>
class LockFreeFIFO {
public:
    static_assert((Capacity & (Capacity - 1)) == 0, 
                  "Capacity must be a power of 2 for optimal performance");
    
    LockFreeFIFO() noexcept : writeIndex_(0), readIndex_(0) {
        // Initialize with default-constructed objects
        std::memset(buffer_.data(), 0, sizeof(buffer_));
    }
    
    //==========================================================================
    // Push (Producer - Audio Thread)
    // Returns true if successful, false if FIFO is full
    //==========================================================================
    [[nodiscard]] bool push(const T& item) noexcept {
        const size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
        const size_t nextWrite = increment(currentWrite);
        
        // Check if FIFO is full
        if (nextWrite == readIndex_.load(std::memory_order_acquire)) {
            return false;  // Full - cannot write
        }
        
        // Write data
        buffer_[currentWrite] = item;
        
        // Publish write (release semantics ensures data is visible)
        writeIndex_.store(nextWrite, std::memory_order_release);
        return true;
    }
    
    //==========================================================================
    // Try to push multiple items at once (batch operation)
    //==========================================================================
    [[nodiscard]] size_t pushMultiple(const T* items, size_t count) noexcept {
        size_t pushed = 0;
        for (size_t i = 0; i < count; ++i) {
            if (!push(items[i])) {
                break;
            }
            ++pushed;
        }
        return pushed;
    }
    
    //==========================================================================
    // Pop (Consumer - GUI Thread)
    // Returns std::nullopt if FIFO is empty
    //==========================================================================
    [[nodiscard]] std::optional<T> pop() noexcept {
        const size_t currentRead = readIndex_.load(std::memory_order_relaxed);
        
        // Check if FIFO is empty
        if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
            return std::nullopt;  // Empty
        }
        
        // Read data
        T item = buffer_[currentRead];
        
        // Publish read (release semantics)
        readIndex_.store(increment(currentRead), std::memory_order_release);
        return item;
    }
    
    //==========================================================================
    // Pop multiple items (batch operation)
    //==========================================================================
    [[nodiscard]] size_t popMultiple(T* items, size_t maxCount) noexcept {
        size_t popped = 0;
        for (size_t i = 0; i < maxCount; ++i) {
            auto item = pop();
            if (!item.has_value()) {
                break;
            }
            items[i] = item.value();
            ++popped;
        }
        return popped;
    }
    
    //==========================================================================
    // Query state (lock-free reads)
    //==========================================================================
    [[nodiscard]] bool isEmpty() const noexcept {
        return readIndex_.load(std::memory_order_acquire) == 
               writeIndex_.load(std::memory_order_acquire);
    }
    
    [[nodiscard]] size_t size() const noexcept {
        const size_t write = writeIndex_.load(std::memory_order_acquire);
        const size_t read = readIndex_.load(std::memory_order_acquire);
        return (write >= read) ? (write - read) : (Capacity - read + write);
    }
    
    [[nodiscard]] size_t capacity() const noexcept {
        return Capacity - 1;  // One slot is always reserved
    }
    
    //==========================================================================
    // Clear (only call from consumer thread!)
    //==========================================================================
    void clear() noexcept {
        readIndex_.store(writeIndex_.load(std::memory_order_acquire), 
                        std::memory_order_release);
    }
    
private:
    //==========================================================================
    // Helper: Increment with wraparound (using bitmask for power-of-2 sizes)
    //==========================================================================
    [[nodiscard]] static constexpr size_t increment(size_t index) noexcept {
        return (index + 1) & (Capacity - 1);
    }
    
    //==========================================================================
    // Data members
    // Note: We use separate cache lines to avoid false sharing between threads
    //==========================================================================
    alignas(64) std::atomic<size_t> writeIndex_;  // Producer index
    alignas(64) std::atomic<size_t> readIndex_;   // Consumer index
    alignas(64) std::array<T, Capacity> buffer_;  // Ring buffer
};

//==============================================================================
// Message types for Audio-GUI communication
//==============================================================================
enum class MessageType : uint32_t {
    ParameterChange,
    MeterLevel,
    AudioDeviceChange,
    TransportState,
    PluginLoaded,
    Error
};

//==============================================================================
// Generic message structure
//==============================================================================
struct AudioMessage {
    MessageType type;
    uint32_t param1;
    uint32_t param2;
    float value1;
    float value2;
    
    AudioMessage() noexcept 
        : type(MessageType::ParameterChange), param1(0), param2(0), 
          value1(0.0f), value2(0.0f) {}
    
    AudioMessage(MessageType t, uint32_t p1, float v1) noexcept
        : type(t), param1(p1), param2(0), value1(v1), value2(0.0f) {}
};

// Type aliases for common FIFOs
using MessageFIFO = LockFreeFIFO<AudioMessage, 4096>;
using MeterFIFO = LockFreeFIFO<float, 1024>;

} // namespace Omega::Memory

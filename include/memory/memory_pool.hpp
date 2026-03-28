#pragma once

#include <cstddef>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace hft {

    struct MemoryStats {
        size_t total_capacity;
        size_t available_blocks;
        size_t successful_allocations;
        size_t successful_deallocations;
    };

    /**
     * @brief O(1) Contiguous Memory Arena.
     * Allocates a single massive block of RAM at startup to prevent heap fragmentation.
     */
    template <typename T, size_t PoolSize>
    class MemoryPool {
    private:
        union Chunk {
            alignas(alignof(T)) unsigned char data[sizeof(T)];
            Chunk* next;
        };

        static_assert(sizeof(T) >= sizeof(Chunk*), "T is too small for free list.");

        // ONE contiguous allocation. No for-loop of `new` calls here!
        std::vector<Chunk> arena_;
        Chunk* free_head_;
        
        size_t available_blocks_;
        size_t alloc_count_ = 0;
        size_t dealloc_count_ = 0;

    public:
        MemoryPool() : available_blocks_(PoolSize) {
            arena_.resize(PoolSize); // 100% Cache Locality
            
            for (size_t i = 0; i < PoolSize - 1; ++i) {
                arena_[i].next = &arena_[i + 1];
            }
            arena_[PoolSize - 1].next = nullptr;
            free_head_ = &arena_[0];
        }

        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;

        [[nodiscard]] void* allocate() {
            if (!free_head_) throw std::bad_alloc(); 
            
            Chunk* block = free_head_;
            free_head_ = free_head_->next;
            
            available_blocks_--;
            alloc_count_++;
            return block->data;
        }

        void deallocate(void* ptr) noexcept {
            if (!ptr) return;
            Chunk* block = static_cast<Chunk*>(ptr);
            block->next = free_head_;
            free_head_ = block;

            available_blocks_++;
            dealloc_count_++;
        }

        MemoryStats get_stats() const noexcept {
            return { PoolSize, available_blocks_, alloc_count_, dealloc_count_ };
        }
    };
} // namespace hft
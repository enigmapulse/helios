#pragma once

#include <cstddef>

namespace hft {

    /**
     * @brief A zero-allocation, O(1) doubly-linked list.
     * @tparam T The type being stored. T MUST contain `T* prev` and `T* next` members.
     */
    template <typename T>
    class IntrusiveList {
    private:
        T* head_ = nullptr;
        T* tail_ = nullptr;
        size_t size_ = 0;

    public:
        IntrusiveList() = default;
        ~IntrusiveList() = default;

        // Prevent copying as raw pointers represent unique ownership in the list
        IntrusiveList(const IntrusiveList&) = delete;
        IntrusiveList& operator=(const IntrusiveList&) = delete;

        // Allow moving so std::vector can shift PriceLevels around in memory
        IntrusiveList(IntrusiveList&& other) noexcept 
            : head_(other.head_), tail_(other.tail_), size_(other.size_) {
            // Steal the pointers and zero out the old list
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }

        IntrusiveList& operator=(IntrusiveList&& other) noexcept {
            if (this != &other) {
                head_ = other.head_;
                tail_ = other.tail_;
                size_ = other.size_;
                
                // Zero out the old list
                other.head_ = nullptr;
                other.tail_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_t size() const noexcept { return size_; }
        
        T* head() const noexcept { return head_; }
        T* tail() const noexcept { return tail_; }

        /**
         * @brief Appends an existing object to the back of the list in O(1).
         * @param node Pointer to the object.
         */
        void push_back(T* node) noexcept {
            node->next = nullptr;
            node->prev = tail_;

            if (tail_) tail_->next = node;
            else head_ = node;
            
            tail_ = node;
            size_++;
        }

        /**
         * @brief Removes a specific node from the list in O(1) without iterating.
         * @param node Pointer to the object to remove.
         */
        void erase(T* node) noexcept {
            if (!node) return;

            if (node->prev) node->prev->next = node->next;
            else head_ = node->next; // Node was head

            if (node->next) node->next->prev = node->prev;
            else tail_ = node->prev; // Node was tail

            // Sanitize pointers to prevent dangling behavior
            node->prev = nullptr;
            node->next = nullptr;
            size_--;
        }

        /**
         * @brief Pops the front element in O(1).
         * @return Pointer to the popped element.
         */
        T* pop_front() noexcept {
            if (!head_) return nullptr;
            T* popped = head_;
            erase(head_);
            return popped;
        }
    };

} // namespace hft
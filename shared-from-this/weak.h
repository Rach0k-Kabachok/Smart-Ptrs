#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    friend class WeakPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
    }

    WeakPtr(const WeakPtr& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseWeakCounter();
    }

    WeakPtr(WeakPtr&& other) noexcept : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseWeakCounter();
        other.Reset();
    }

    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseWeakCounter();
    }

    template <typename Y>
    WeakPtr(WeakPtr<Y>&& other) noexcept : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseWeakCounter();
        other.Reset();
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseWeakCounter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            Reset();
            control_block_ = other.control_block_;
            ptr_ = other.ptr_;
            IncreaseWeakCounter();
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            Reset();
            Swap(other);
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        DecreaseWeakCounter();
        control_block_ = nullptr;
        ptr_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(control_block_, other.control_block_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (control_block_) {
            return control_block_->GetStrongCounter();
        }
        return 0;
    }

    bool Expired() const {
        return control_block_ == nullptr || control_block_->IsDestroyed();
    }

    SharedPtr<T> Lock() const {
        if (!Expired()) {
            return SharedPtr<T>(control_block_, ptr_);
        }
        return SharedPtr<T>();
    }

private:
    ControlBlockBase* control_block_ = nullptr;
    T* ptr_ = nullptr;
    bool is_esft_ptr_ = false;

    void IncreaseWeakCounter() {
        if (control_block_) {
            control_block_->IncWeakRef();
        }
    }

    void DecreaseWeakCounter() {
        if (control_block_) {
            control_block_->DecWeakRef();
            if (!is_esft_ptr_ && control_block_->CanDeleteControlBlock()) {
                delete control_block_;
            }
        }
    }

    size_t GetWeakCounter() const {
        if (control_block_) {
            return control_block_->GetWeakCounter();
        }
        return 0;
    }

    void MakeEsftPtr() {
        is_esft_ptr_ = true;
    }
};

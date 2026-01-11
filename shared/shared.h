#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

struct ControlBlockBase {
    size_t counter = 1;

    size_t IncRef() {
        return ++counter;
    }

    size_t DecRef() {
        return --counter;
    }

    size_t GetCounter() const {
        return counter;
    }

    virtual void* GetRawPtr() = 0;

    virtual ~ControlBlockBase() = default;
    virtual void Destroy() = 0;
};

template <typename T>
struct DefaultBlock : ControlBlockBase {
    T* deletem_ptr;

    DefaultBlock(T* ptr) : deletem_ptr(ptr) {
    }

    void Destroy() override {
        if (deletem_ptr) {
            delete deletem_ptr;
        }
        counter = 0;
    }

    void* GetRawPtr() override {
        return deletem_ptr;
    }
};

template <typename T>
struct InlineBlock : ControlBlockBase {
    T object;

    template <typename... Args>
    InlineBlock(Args&&... args) : object(std::forward<Args>(args)...) {
    }

    void Destroy() override {
        counter = 0;
    }

    void* GetRawPtr() override {
        return &object;
    }
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
    template <typename Y>
    friend class SharedPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
    }
    SharedPtr(std::nullptr_t) {
    }
    template <typename Y>
    explicit SharedPtr(Y* ptr)
        : control_block_(new DefaultBlock<Y>(ptr)), ptr_(static_cast<T*>(ptr)) {
    }
    SharedPtr(ControlBlockBase* block, T* ptr) : control_block_(block), ptr_(ptr) {
    }
    SharedPtr(const SharedPtr& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseCounter();
    }
    SharedPtr(SharedPtr&& other) noexcept : control_block_(other.control_block_), ptr_(other.ptr_) {
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseCounter();
    }
    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) noexcept
        : control_block_(other.control_block_), ptr_(other.ptr_) {
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        control_block_ = other.control_block_;
        IncreaseCounter();
        ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            DecreaseCounter();
            control_block_ = other.control_block_;
            ptr_ = other.ptr_;
            IncreaseCounter();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            Reset();
            Swap(other);
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        DecreaseCounter();
        control_block_ = nullptr;
        ptr_ = nullptr;
    }

    template <typename Y>
    void Reset(Y* ptr) {
        DecreaseCounter();
        control_block_ = new DefaultBlock<Y>(ptr);
        ptr_ = static_cast<T*>(ptr);
    }
    void Swap(SharedPtr& other) {
        std::swap(control_block_, other.control_block_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }

    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        return GetCounter();
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }

    void IncreaseCounter() {
        if (control_block_) {
            control_block_->IncRef();
        }
    }
    void DecreaseCounter() {
        if (control_block_) {
            control_block_->DecRef();
            if (control_block_->GetCounter() == 0) {
                control_block_->Destroy();
                delete control_block_;
            }
        }
    }
    size_t GetCounter() const {
        if (control_block_) {
            return control_block_->GetCounter();
        }
        return 0;
    }

private:
    ControlBlockBase* control_block_ = nullptr;
    T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    InlineBlock<T>* block = new InlineBlock<T>(std::forward<Args>(args)...);
    T* ptr = static_cast<T*>(block->GetRawPtr());
    return SharedPtr<T>(block, ptr);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};

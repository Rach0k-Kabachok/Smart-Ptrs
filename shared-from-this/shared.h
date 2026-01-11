#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <iostream>
struct ControlBlockBase {
    size_t strong_ref_cnt = 0;
    size_t weak_ref_cnt = 0;
    bool is_destroyed = false;

    size_t IncStrongRef() {
        return ++strong_ref_cnt;
    }

    size_t DecStrongRef() {
        return --strong_ref_cnt;
    }

    size_t GetStrongCounter() const {
        return strong_ref_cnt;
    }

    size_t IncWeakRef() {
        return ++weak_ref_cnt;
    }

    size_t DecWeakRef() {
        return --weak_ref_cnt;
    }

    size_t GetWeakCounter() const {
        return weak_ref_cnt;
    }

    size_t GetAllCounter() const {
        return strong_ref_cnt + weak_ref_cnt;
    }

    bool IsDestroyed() const {
        return is_destroyed;
    }

    bool CanDeleteControlBlock() const {
        return GetAllCounter() == 0;
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
        if (!is_destroyed) {
            strong_ref_cnt = 0;
            is_destroyed = true;

            if (deletem_ptr) {
                delete deletem_ptr;
            }
        }
    }

    void* GetRawPtr() override {
        return deletem_ptr;
    }
};

template <typename T>
struct InlineBlock : ControlBlockBase {
    alignas(T) char storage[sizeof(T)];

    template <typename... Args>
    InlineBlock(Args&&... args) {
        new (storage) T(std::forward<Args>(args)...);
    }

    void Destroy() override {
        if (!is_destroyed) {
            strong_ref_cnt = 0;
            is_destroyed = true;
            reinterpret_cast<T*>(storage)->~T();
        }
    }

    void* GetRawPtr() override {
        return storage;
    }
};
// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    friend class WeakPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : control_block_(nullptr), ptr_(nullptr) {
    }

    SharedPtr(std::nullptr_t) : control_block_(nullptr), ptr_(nullptr) {
    }

    template <typename Y>
    explicit SharedPtr(Y* ptr)
        : control_block_(new DefaultBlock<Y>(ptr)), ptr_(static_cast<T*>(ptr)) {
        IncreaseStrongCounter();

        if constexpr (std::is_convertible_v<Y*, ESFTBase*>) {
            InitWeakThis(ptr);
        }
    }

    SharedPtr(ControlBlockBase* block, T* ptr) : control_block_(block), ptr_(ptr) {
        IncreaseStrongCounter();

        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            InitWeakThis(ptr);
        }
    }

    SharedPtr(const SharedPtr& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseStrongCounter();
    }

    SharedPtr(SharedPtr&& other) noexcept : control_block_(other.control_block_), ptr_(other.ptr_) {
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        IncreaseStrongCounter();
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
        IncreaseStrongCounter();
        ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other)
        : control_block_(other.control_block_), ptr_(other.ptr_) {
        if (control_block_->IsDestroyed()) {
            throw BadWeakPtr();
        }
        IncreaseStrongCounter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            DecreaseStrongCounter();
            control_block_ = other.control_block_;
            ptr_ = other.ptr_;
            IncreaseStrongCounter();
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
        DecreaseStrongCounter();
        control_block_ = nullptr;
        ptr_ = nullptr;
    }

    template <typename Y>
    void Reset(Y* ptr) {
        *this = SharedPtr(ptr);
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
        return GetStrongCounter();
    }

    explicit operator bool() const {
        return ptr_ != nullptr;
    }

private:
    ControlBlockBase* control_block_ = nullptr;
    T* ptr_ = nullptr;

    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* esft_block) {
        esft_block->weak_this_ = WeakPtr<Y>(*this);
        esft_block->weak_this_.MakeEsftPtr();
    }

    void IncreaseStrongCounter() {
        if (control_block_) {
            control_block_->IncStrongRef();
        }
    }

    void DecreaseStrongCounter() {
        if (control_block_) {
            control_block_->DecStrongRef();
            if (control_block_->GetStrongCounter() == 0) {
                control_block_->Destroy();
                if (control_block_->CanDeleteControlBlock()) {
                    delete control_block_;
                }
            }
        }
    }

    size_t GetStrongCounter() const {
        if (control_block_) {
            return control_block_->GetStrongCounter();
        }
        return 0;
    }
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
struct ESFTBase {};

template <typename T>
class EnableSharedFromThis : public ESFTBase {
    template <typename Y>
    friend class SharedPtr;

public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_this_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return WeakPtr<T>(weak_this_);
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(weak_this_);
    }

private:
    WeakPtr<T> weak_this_;
};

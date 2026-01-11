#pragma once

#include <utility>
#include <type_traits>

template <typename T, int ElementInd, bool IsEmpty = std::is_empty_v<T> && !std::is_final_v<T>>
class CompressedElement {
public:
    CompressedElement() : value_(){};

    CompressedElement(const T& value) : value_(value) {
    }

    CompressedElement(T&& value) : value_(std::move(value)) {
    }

    CompressedElement operator=(const CompressedElement& other) = delete;
    CompressedElement operator=(CompressedElement&& other) = delete;

    const T& GetVal() const {
        return value_;
    }

    T& GetVal() {
        return value_;
    }

    ~CompressedElement() = default;

private:
    T value_;
};

template <typename T, int ElementInd>
class CompressedElement<T, ElementInd, true> : private T {
public:
    CompressedElement() = default;

    CompressedElement(const T& value) : T(value) {
    }

    CompressedElement(T&& value) : T(std::move(value)) {
    }

    CompressedElement operator=(const T& value) = delete;
    CompressedElement operator=(T&& value) = delete;

    const T& GetVal() const {
        return *this;
    }

    T& GetVal() {
        return *this;
    }

    ~CompressedElement() = default;
};

template <typename F, typename S>
class CompressedPair : private CompressedElement<F, 0>, private CompressedElement<S, 1> {
public:
    CompressedPair() = default;

    CompressedPair(const F& first, const S& second) : FirstElement(first), SecondElement(second) {
    }

    CompressedPair(const F& first, S&& second)
        : FirstElement(first), SecondElement(std::move(second)) {
    }

    CompressedPair(F&& first, const S& second)
        : FirstElement(std::move(first)), SecondElement(second) {
    }

    CompressedPair(F&& first, S&& second)
        : FirstElement(std::move(first)), SecondElement(std::move(second)) {
    }

    CompressedPair operator=(const CompressedPair& other) = delete;
    CompressedPair operator=(CompressedPair&& other) = delete;

    F& GetFirst() {
        return FirstElement::GetVal();
    }

    const F& GetFirst() const {
        return FirstElement::GetVal();
    }

    S& GetSecond() {
        return SecondElement::GetVal();
    }

    const S& GetSecond() const {
        return SecondElement::GetVal();
    }

    ~CompressedPair() = default;

private:
    using FirstElement = CompressedElement<F, 0>;
    using SecondElement = CompressedElement<S, 1>;
};
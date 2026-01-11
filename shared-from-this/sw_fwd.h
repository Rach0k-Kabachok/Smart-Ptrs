#pragma once

#include <exception>

// Instead of std::bad_weak_ptr
class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

struct ESFTBase;

template <typename T>
class EnableSharedFromThis;

template <typename T>
class WeakPtr;

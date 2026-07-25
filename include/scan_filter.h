#pragma once

#include <string>
#include <vector>
#include "util.h"

template<typename T>
bool keep_if_same(const T& old_value, const T& new_value)
{
    return old_value == new_value;
}

template<typename T>
bool keep_if_changed(const T& old_value, const T& new_value)
{
    return old_value != new_value;
}

template<typename T>
bool keep_if_increased(const T& old_value, const T& new_value)
{
    return new_value > old_value;
}

template<typename T>
bool keep_if_decreased(const T& old_value, const T& new_value)
{
    return new_value < old_value;
}

template<typename T>
bool keep_if_new_value(const T& new_value, const T& target_value)
{
    return new_value == target_value;
}
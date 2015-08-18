#pragma once

template<typename T>
size_t align_size(size_t s)
{
    s += alignof(T)-1;
    s &= ~(alignof(T)-1);
    return s;
}

template<typename T>
T* align_ptr(T* p) {
    return (T*)align_size<T>((size_t)p);
}
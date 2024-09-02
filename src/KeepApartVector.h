#pragma once
#include <vector>
template <typename T>
class alignas(std::hardware_destructive_interference_size) KeepApartVector : public std::vector<T>
{

};
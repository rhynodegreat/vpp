#pragma once

#include <cstdlib>
#include <vector>

namespace vk
{

template<typename T>
class Range
{
public:
	using Value = std::remove_const_t<T>;

public:
	Range() = default;
	Range(T& value, std::size_t size = 1) : data_(&value), size_(size) {}
	Range(T* value, std::size_t size = 1) : data_(value), size_(size) {}
	template<std::size_t N> Range(T (&value)[N]) : data_(value), size_(N) {}
	Range(const std::vector<Value>& vec) : data_(vec.data()), size_(vec.size()) {}
	Range(const std::initializer_list<Value>& initList) : data_(initList.begin()), size_(initList.size()) {}

	const T* data() const { return data_; }
	std::size_t size() const { return size_; }

protected:
	const T* data_ = nullptr;
	std::size_t size_ = 0;
};

}
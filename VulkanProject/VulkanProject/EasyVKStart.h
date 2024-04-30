#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <chrono>
#include <numeric>
#include <numbers>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <stb_image.h>

#include <vulkan/vulkan.h>
#pragma comment(lib, "vulkan-1.lib")

template<typename T>
class arrayRef {
	T* const pArray = nullptr;
	size_t count = 0;
public:
	//从空参数构造，count为0
	arrayRef() = default;
	//从单个对象构造，count为1
	arrayRef(T& data) : pArray(&data), count(1) {}
	//从顶级数组构造
	template<size_t elementCount>
	arrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
	arrayRef(T* pData, size_t elementCount) : pArray(pData), count(elementCount) {}
	arrayRef(const arrayRef<std::remove_const_t<T>>& other) : pArray(other.Pointer()), count(other.Count()) {}
	//Getter
	T* Pointer() const { return pArray; }
	size_t Count() const { return count; }
	//Const function
	T& operator[](size_t index) const { return pArray[index]; }
	T* begin() const { return pArray; }
	T* end() const { return pArray + count; }
	//non const function
	arrayRef& operator=(const arrayRef&) = delete;
};
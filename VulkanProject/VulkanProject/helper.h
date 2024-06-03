#pragma once
#include "glm.hpp"
#include <iostream>

void displayVec4(glm::vec4 vec) {
	std::cout << vec.x << " " << vec.y << " " << vec.z << " " << vec.w << std::endl;
}
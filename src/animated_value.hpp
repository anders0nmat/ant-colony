#pragma once

#include <functional>
#include <glfw/glfw3.h>

template<typename T, typename D = double>
struct AnimatedValue {
	T start_value;
	T end_value;

	D start_time;
	D duration;

	std::function<D(D)> interpolation;

	AnimatedValue(T value, std::function<D(D)> interpolation)
		: start_value(value), end_value(value), start_time(glfwGetTime()), duration(0), interpolation(interpolation) {}

	T current() {	
		D t = glfwGetTime() - start_time;
		if (t >= duration || duration == 0) {
			return end_value;
		}

		D p = interpolation(t / duration);
		return start_value * (1 - p) + end_value * p;
	}

	void ease_to(T new_value, D dur) {
		start_value = current();
		end_value = new_value;
		duration = dur;
		start_time = glfwGetTime();
	}
};

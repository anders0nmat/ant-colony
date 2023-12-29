
#include <vector>
#include <iostream>

int main() {

	std::vector<int> nums;
	nums.reserve(400);
	std::cout << "nums.capacity() = " << nums.capacity() << std::endl;
	std::vector<int> working = nums;
	std::cout << "working.capacity() = " << working.capacity() << std::endl;


	return 0;
}

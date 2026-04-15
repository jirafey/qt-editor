#include <iostream>
using namespace std;

struct Vector {
	int a, b, c;
	Vector(int x, int y, int z) : a(x), b(y), c(z) {}
};

int sum(Vector vec) { return vec.a + vec.b + vec.c; }

int main() {

	int result = sum(Vector(10, 20, 30));
	cout << "Result" << result;
	return 0;
}

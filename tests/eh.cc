// C++ exceptions (crtbegin.o registers .eh_frame): throw/catch across
// frames, rethrow, destructors run during unwinding, std::exception
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

static int dtors;

struct Guard {
	~Guard() { dtors++; }
};

static void
thrower(int n)
{
	Guard g;
	if (n == 0)
		throw std::runtime_error("deep");
	thrower(n - 1);
}

int
main()
{
	int ok = 0;
	try {
		thrower(5);
	} catch (const std::exception &e) {
		ok += std::string(e.what()) == "deep";
	}
	try {
		try {
			throw 42;
		} catch (int) {
			throw;
		}
	} catch (int v) {
		ok += v == 42;
	}
	try {
		std::vector<int> v(3);
		(void)v.at(7);
	} catch (const std::out_of_range &) {
		ok++;
	}
	std::printf("eh: ok=%d dtors=%d\n", ok, dtors);
	return ok == 3 && dtors == 6 ? 0 : 1;
}

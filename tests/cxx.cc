// C++: libstdc++ over APE, static constructors, containers (eh.cc: exceptions)
#include <cstdio>
#include <map>
#include <string>
#include <vector>

static std::string greeting = std::string("c++") + " ctor";

int
main()
{
	std::map<std::string, int> m;
	std::vector<int> v;
	for (int i = 0; i < 10; i++)
		v.push_back(i * i);
	m["sum"] = 0;
	for (int x : v)
		m["sum"] += x;
	std::printf("cxx: %s sum=%d\n", greeting.c_str(), m["sum"]);
	return m["sum"] == 285 && greeting == "c++ ctor" ? 0 : 1;
}

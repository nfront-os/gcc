// libstdc++ over gcc's APE: C99 math and stdio
// (to_string, stod, iostreams with long double), wide strings and
// streams, <complex>, <format>, <chrono>, threads (std::thread, mutex,
// condition_variable, async, thread_local, shared_mutex). Prints each
// failure and "cxxlib: n failed".
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <condition_variable>
#include <cstdio>
#include <format>
#include <future>
#include <iomanip>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static int failed;

static void
check(const char *what, bool ok)
{
	if (!ok) {
		std::printf("cxxlib: FAIL %s\n", what);
		failed++;
	}
}

static thread_local int tlcount;

int
main()
{
	// strings and numbers
	check("to_string", std::to_string(42) == "42" && std::to_string(-1.5) == "-1.500000");
	check("stoi", std::stoi("  -123x") == -123 && std::stoul("ff", nullptr, 16) == 255);
	check("stod", std::stod("2.5e3") == 2500.0 && std::stof("0.25") == 0.25f);
	check("stold", std::stold("1e4000") > std::numeric_limits<double>::max());
	bool threw = false;
	try {
		(void)std::stoi("x");
	} catch (const std::invalid_argument &) {
		threw = true;
	}
	check("stoi throws", threw);

	std::ostringstream os;
	os << std::setprecision(20) << 3.14159265358979323846264338327950288L;
	check("ostream long double", os.str() == "3.1415926535897932385");
	std::istringstream is("1.5 -7 0x1p4");
	double d;
	int i;
	is >> d >> i;
	check("istream", d == 1.5 && i == -7);

	// wide
	std::wostringstream wos;
	wos << L"grüß " << 42 << L' ' << 2.5;
	check("wostringstream", wos.str() == L"grüß 42 2.5");
	check("to_wstring", std::to_wstring(7) == L"7");
	check("stoi wide", std::stoi(std::wstring(L"-99")) == -99);

	// <cmath>, <complex>
	check("cmath", std::round(2.5) == 3.0 && std::cbrt(27.0) == 3.0 &&
		std::isnan(std::nan("")) && std::hypot(3.0, 4.0, 12.0) == 13.0 &&
		std::fabs(std::tgamma(5.0L) - 24.0L) < 1e-15L);
	std::complex<double> z = std::exp(std::complex<double>(0, M_PI));
	check("complex", std::abs(z + 1.0) < 1e-15 && std::sqrt(std::complex<double>(-4)) ==
		std::complex<double>(0, 2));

	// <format>
	check("format", std::format("{:>6.2f}|{:#x}|{}", 3.14159, 255, "s") == "  3.14|0xff|s");

	// <chrono>
	auto t0 = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
	auto dt = std::chrono::steady_clock::now() - t0;
	check("steady_clock, sleep_for", dt >= std::chrono::milliseconds(29) && dt < std::chrono::seconds(5));
	check("system_clock", std::chrono::system_clock::now().time_since_epoch() > std::chrono::hours(24 * 365 * 50));

	// threads: lambdas with main's locals by reference
	long sum = 0;
	std::mutex m;
	std::vector<std::thread> ts;
	for (int k = 0; k < 8; k++)
		ts.emplace_back([&sum, &m, k] {
			for (int j = 0; j < 10000; j++) {
				std::lock_guard<std::mutex> g(m);
				sum += k;
			}
			tlcount += k;
		});
	for (auto &t : ts)
		t.join();
	check("threads, mutex", sum == 10000L * (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7));
	check("thread_local", tlcount == 0);

	std::condition_variable cv;
	bool go = false;
	int woke = 0;
	std::thread w([&] {
		std::unique_lock<std::mutex> l(m);
		cv.wait(l, [&] { return go; });
		woke = 1;
	});
	{
		std::lock_guard<std::mutex> g(m);
		go = true;
	}
	cv.notify_one();
	w.join();
	check("condition_variable", woke == 1);
	{
		std::unique_lock<std::mutex> l(m);
		check("wait_for timeout", cv.wait_for(l, std::chrono::milliseconds(20)) == std::cv_status::timeout);
	}

	auto f = std::async(std::launch::async, [] { return 6 * 7; });
	check("async", f.get() == 42);
	std::promise<int> p;
	auto pf = p.get_future();
	std::thread pt([&p] { p.set_value(5); });
	check("promise", pf.get() == 5);
	pt.join();

	std::atomic<int> at{0};
	std::shared_mutex sm;
	std::vector<std::thread> rs;
	for (int k = 0; k < 4; k++)
		rs.emplace_back([&] {
			for (int j = 0; j < 1000; j++) {
				std::shared_lock<std::shared_mutex> l(sm);
				at.fetch_add(1);
			}
		});
	{
		std::unique_lock<std::shared_mutex> l(sm);
		at.fetch_add(1000);
	}
	for (auto &t : rs)
		t.join();
	check("shared_mutex, atomic", at.load() == 5000);

	// an exception in another thread, back through a future
	auto ef = std::async(std::launch::async, [] () -> int { throw std::runtime_error("bad"); });
	threw = false;
	try {
		ef.get();
	} catch (const std::runtime_error &e) {
		threw = std::string(e.what()) == "bad";
	}
	check("exception across threads", threw);
	check("hardware_concurrency", std::thread::hardware_concurrency() >= 1);

	std::printf("cxxlib: %d failed\n", failed);
	return failed != 0;
}

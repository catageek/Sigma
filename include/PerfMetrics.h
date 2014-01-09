#ifndef PERFMETRICS_H_INCLUDED
#define PERFMETRICS_H_INCLUDED

#include <chrono>
#include <vector>

using namespace std::chrono;

class PerfMetrics {
public:
	PerfMetrics(const std::string& name) : name(name), duration_sum(0), count(0), initial(steady_clock::now()) {};

	void start() {
		last_clock = steady_clock::now();
	}

	void stop() {
		steady_clock::time_point stop = steady_clock::now();
		duration_sum += duration_cast<microseconds>(stop - last_clock);
		count++;
	}

	void displayMeantime() {
		std::cout <<  mean() * 100 << " seconds for 100 iterations for metrics " << name << std::endl;
	}

	void displayTotalRuntime() const {
		steady_clock::time_point stop = steady_clock::now();
		microseconds time = duration_cast<microseconds>(stop - initial);
		std::cout << "time of execution: " << (double) time.count() / 1.0E6 << " seconds" << std::endl;
		std::cout << "number of iterations: " << count << std::endl;
		std::cout << ((double) count) * 1E6 / time.count() << " FPS" << std::endl;
	}

private:

	double mean() const {
		return (double) duration_sum.count() / count / 1E6;
	}

	unsigned int count;
	steady_clock::time_point initial;
	steady_clock::time_point last_clock;
	microseconds duration_sum;
	std::string name;
};


#endif // PERFMETRICS_H_INCLUDED

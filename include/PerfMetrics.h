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
		duration_sum += duration_cast<duration<double>>(stop - last_clock).count();
		count++;
	}

	void displayMeantime() {
		std::cout <<  mean() * 100 << " seconds for 100 iterations for metrics " << name << std::endl;
	}

	void displayTotalRuntime() const {
		steady_clock::time_point stop = steady_clock::now();
		double time = duration_cast<duration<double>>(stop - initial).count();
		std::cout << ((double) count) / time << " FPS" << std::endl;
	}

private:

	double mean() const {
		return duration_sum / count;
	}

	unsigned int count;
	steady_clock::time_point initial;
	steady_clock::time_point last_clock;
	double duration_sum;
	std::string name;
};


#endif // PERFMETRICS_H_INCLUDED

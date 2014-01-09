#ifndef PERFMETRICS_H_INCLUDED
#define PERFMETRICS_H_INCLUDED

#include <chrono>
#include <vector>

using namespace std::chrono;

class PerfMetrics {
public:
	PerfMetrics(const std::string& name) : name(name), initial(steady_clock::now()) {};

	void start() {
		last_clock = steady_clock::now();
	}

	void stop() {
		steady_clock::time_point stop = steady_clock::now();
		duration_vect.push_back(duration_cast<duration<double>>(stop - last_clock).count());
	}

	void displayMeantime() {
		std::cout <<  mean() * 100 << " seconds for 100 iterations for metrics " << name << std::endl;
	}

	void displayTotalRuntime() const {
		steady_clock::time_point stop = steady_clock::now();
		auto time = duration_cast<duration<double>>(stop - initial).count();
		std::cout << ((double) duration_vect.size()) / time << " FPS" << std::endl;
	}

private:
	double sum() const {
		double sum = 0;
		for (auto itr = duration_vect.cbegin(); itr != duration_vect.cend(); itr++) {
			sum += *itr;
		}
		return sum;
	}

	double mean() const {
		return sum() / duration_vect.size();
	}

	steady_clock::time_point initial;
	steady_clock::time_point last_clock;
	std::vector<double> duration_vect;
	std::string name;
};


#endif // PERFMETRICS_H_INCLUDED

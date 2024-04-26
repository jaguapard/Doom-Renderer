#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <atomic>

class Threadpool
{
public:
	Threadpool(size_t numThreads);
	size_t addTask(std::function<void()> taskFunc);
	void waitUntilTaskCompletes(size_t taskIndex);
private:
	std::unordered_map<size_t, std::function<void()>> tasks;
	std::unordered_map<size_t, std::unordered_set<size_t>> dependencies; //map task ids to a set of task ids that must complete before this one starts
	std::unordered_set<size_t> finishedTasks;

	std::mutex taskListMutex;
	std::vector<std::thread> threads;
	std::atomic<size_t> lastTaskId = 0;

	std::condition_variable cv;
	std::mutex cv_mtx;

	void workerRoutine(size_t workerNumber);
};
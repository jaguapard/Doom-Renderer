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
	typedef std::function<void()> task_t;
	typedef size_t task_id;

	Threadpool(size_t numThreads);
	task_id addTask(task_t taskFunc); //add an independent task to the pool
	task_id addTask(task_t taskFunc, std::vector<task_id> dependendies); //add task that must start only if all the `dependencies` finished
	void waitUntilTaskCompletes(task_id taskIndex);
private:
	std::unordered_map<task_id, task_t> tasks;
	std::unordered_map<task_id, std::unordered_set<task_id>> dependencies; //map task ids to a set of task ids that must complete before this one starts
	std::unordered_set<task_id> finishedTasks, inProgressTasks;

	std::mutex taskListMutex;
	std::vector<std::thread> threads;
	std::atomic<task_id> lastTaskId = 0;

	std::condition_variable cv;
	std::mutex cv_mtx;

	void workerRoutine(size_t workerNumber);
};
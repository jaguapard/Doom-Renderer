#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <atomic>
#include <optional>
#include <set>

class Threadpool
{
public:
	typedef std::function<void()> task_t;
	typedef size_t task_id;

	Threadpool(std::optional<size_t> numThreads = std::nullopt);

	//add a task to the pool. If `dependencies` is not empty, then the task will only start if all tasks with ids in `dependecies` have finished
	//if wantedId is not nullopt, then the threadpool will attempt to give the specified ID to the added task. This will fail if the ID was not reserved beforehand
	task_id addTask(task_t taskFunc, std::vector<task_id> dependencies = {}, std::optional<task_id> wantedId = std::nullopt);

	std::vector<task_id> reserveTaskIds(size_t count);

	void waitUntilTaskCompletes(task_id taskIndex);
	void waitForMultipleTasks(const std::vector<task_id>& taskIds);

	size_t getThreadCount() const;
	std::pair<double, double> getLimitsForThread(size_t threadIndex, double min, double max, std::optional<size_t> threadCount = std::nullopt) const;
private:
	std::unordered_map<task_id, task_t> unassignedTasks;
	std::unordered_map<task_id, std::unordered_set<task_id>> dependenciesMap; //map task ids to a set of task ids that must complete before this one starts
	std::unordered_set<task_id> inProgressTasks;
	std::unordered_set<task_id> reservedTaskIds;

	std::recursive_mutex taskListMutex;
	std::vector<std::thread> threads;
	std::atomic<task_id> lastFreeTaskId = 1;

	std::condition_variable cv;
	std::mutex cv_mtx;

	void workerRoutine(size_t workerNumber);
	void spawnThreads(size_t numThreads);
	std::optional<std::pair<task_id, task_t>> tryGetTask();

	void markTaskFinished(task_id taskId);
	void markTaskAsInProgress(task_id taskId);
	bool isTaskFinished(task_id taskId);
};
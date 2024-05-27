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

typedef std::function<void()> taskfunc_t;
typedef uint64_t task_id;

class ThreadpoolTask
{
public:
	ThreadpoolTask() = default;
	ThreadpoolTask(const taskfunc_t& taskFunc, std::vector<task_id> dependencies = {}, std::optional<task_id> wantedId = std::nullopt);

	taskfunc_t func;
	std::vector<task_id> dependencies;
	std::optional<task_id> wantedId;

	task_id assignedId = -1; //this will be task_id(-1) in case has not yet been assigned an id and will be overwritten by the threadpool
};
class Threadpool
{
public:
	

	Threadpool(std::optional<size_t> numThreads = std::nullopt);

	//add a task to the pool. If `dependencies` is not empty, then the task will only start if all tasks with ids in `dependecies` have finished
	//if wantedId is not nullopt, then the threadpool will attempt to give the specified ID to the added task. This will fail if the ID was not reserved beforehand
	//task_id addTask(const taskfunc_t& taskFunc, std::vector<task_id> dependencies = {}, std::optional<task_id> wantedId = std::nullopt);
	task_id addTask(const taskfunc_t& taskFunc, std::vector<task_id> dependencies = {}, std::optional<task_id> wantedId = std::nullopt);
	task_id addTask(ThreadpoolTask task);
	std::vector<task_id> addTaskBatch(const std::vector<ThreadpoolTask>& tasks);

	std::vector<task_id> reserveTaskIds(uint64_t count);

	void waitUntilTaskCompletes(task_id taskIndex);
	void waitForMultipleTasks(const std::vector<task_id>& taskIds);

	size_t getThreadCount() const;
	std::pair<double, double> getLimitsForThread(size_t threadIndex, double min, double max, std::optional<size_t> threadCount = std::nullopt) const;

	~Threadpool() noexcept;
private:
	std::recursive_mutex taskListMutex; //this mutex is used to protect accesses to all of these maps and sets
	std::unordered_map<task_id, ThreadpoolTask> taskStore;
	std::unordered_set<task_id> unassignedTasks;
	std::unordered_set<task_id> inProgressTasks;
	std::unordered_set<task_id> reservedTaskIds;

	
	std::vector<std::jthread> threads;
	std::atomic<task_id> lastFreeTaskId = 1;

	std::condition_variable cv;
	std::mutex cv_mtx;

	std::atomic_bool threadpoolMarkedForTermination = false;
	std::vector<bool> threadTerminated;

	void workerRoutine(size_t workerNumber);
	void spawnThreads(size_t numThreads);
	std::optional<task_id> tryGetTask();

	void markTaskFinished(task_id taskId);
	void markTaskAsInProgress(task_id taskId);
	bool isTaskFinished(task_id taskId);
};
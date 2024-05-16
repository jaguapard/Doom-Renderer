#include "Threadpool.h"
#include <cassert>
#include "helpers.h"

using task_id = Threadpool::task_id;
using tast_t = Threadpool::task_t;

Threadpool::Threadpool(std::optional<size_t> numThreads)
{
	size_t threadCount = numThreads.value_or(std::max(1u, std::thread::hardware_concurrency() - 1)); //don't crowd out the main thread, unless it is impossible 
	if (std::thread::hardware_concurrency() == 0) threadCount = 1; //hardware_concurrency can return 0
	this->spawnThreads(threadCount);
}

task_id Threadpool::addTask(task_t taskFunc, std::vector<task_id> dependencies, std::optional<task_id> wantedId)
{
	std::lock_guard lck(taskListMutex);
	task_id id;
	if (wantedId)
	{
		task_id wid = wantedId.value();
		if (reservedTaskIds.find(wid) == reservedTaskIds.end()) throw std::runtime_error("Threadpool: attempted to add task with reserved ID while that ID has not been reserved!");
		reservedTaskIds.erase(wid);
		id = wid;
	}
	else
	{
		id = lastFreeTaskId++;
	}
	
	this->unassignedTasks[id] = taskFunc;
	if (!dependencies.empty()) this->dependenciesMap[id].insert(dependencies.begin(), dependencies.end());
	cv.notify_one();

	return id;
}

std::vector<task_id> Threadpool::reserveTaskIds(size_t count)
{
	std::vector<task_id> ret(count);
	std::lock_guard lck(taskListMutex);
	task_id taskId = lastFreeTaskId;

	for (size_t i = 0; i < count; ++i)
	{
		reservedTaskIds.insert(taskId + i);
		ret.push_back(taskId + i);
	}

	lastFreeTaskId += count;
	return ret;
}

void Threadpool::waitUntilTaskCompletes(task_id taskIndex)
{
	{
		if (taskIndex >= lastFreeTaskId) throw std::runtime_error("Attempting to wait for non-existant task.");
		if (isTaskFinished(taskIndex)) return;
	}

	std::unique_lock cv_lck(cv_mtx);
	cv.wait(cv_lck, [&]() {
		std::lock_guard task_lck(taskListMutex);
		return isTaskFinished(taskIndex);
	});
}

void Threadpool::waitForMultipleTasks(const std::vector<task_id>& taskIds)
{
	for (const auto it : taskIds) waitUntilTaskCompletes(it);
}

size_t Threadpool::getThreadCount() const
{
	return this->threads.size();
}

std::pair<double, double> Threadpool::getLimitsForThread(size_t threadIndex, double min, double max, std::optional<size_t> threadCount) const
{
	size_t nThreads = threadCount ? threadCount.value() : this->getThreadCount();
	double minLimit = lerp(min, max, double(threadIndex) / nThreads);
	double maxLimit = lerp(min, max, double(threadIndex + 1) / nThreads);
	return std::make_pair(std::clamp(minLimit, min, max), std::clamp(maxLimit, min, max));
}

void Threadpool::workerRoutine(size_t workerNumber)
{
	while (true)
	{
		std::pair<task_id, task_t> myTask;
		{ //wait until there are runnable tasks
			std::unique_lock cv_lck(cv_mtx);
			cv.wait(cv_lck, [&]() {
				std::lock_guard taskLck(taskListMutex); //do everything under the aegis of mutex to avoid other threads stealing our job between conditional awake and us marking the task as in-progress
				auto ret = tryGetTask();
				if (!ret.has_value()) return false;

				myTask = ret.value();
				markTaskAsInProgress(myTask.first);
				return true;
			});
		}

		myTask.second();
		markTaskFinished(myTask.first);

		std::unique_lock cv_lck(cv_mtx);
		cv.notify_all();
	}
}

void Threadpool::spawnThreads(size_t numThreads)
{
	for (size_t i = 0; i < numThreads; ++i)
		threads.emplace_back(&Threadpool::workerRoutine, this, i);
}

std::optional<std::pair<task_id, Threadpool::task_t>> Threadpool::tryGetTask()
{
	std::lock_guard lck(taskListMutex);
	for (const auto& candidateTask : unassignedTasks)
	{
		task_id candidateTaskId = candidateTask.first;
		auto depIt = dependenciesMap.find(candidateTaskId);
		if (depIt == dependenciesMap.end() || depIt->second.empty())  //second condition should never occur really
			return { candidateTask }; //if task has no dependendencies, we can just give it to the worker

		for (auto& depTaskId : depIt->second)
		{
			if (!isTaskFinished(depTaskId)) goto tryNextTask;
		}

		return { candidateTask }; //if the task had dependencies, but they have finished, return
		
	tryNextTask: 
		continue; //else keep looking
	}

	return std::nullopt; //no runnable tasks found, return empty optional
}

void Threadpool::markTaskFinished(task_id taskId)
{
	std::lock_guard lck(taskListMutex);
	unassignedTasks.erase(taskId);
	inProgressTasks.erase(taskId);
	dependenciesMap.erase(taskId);
	reservedTaskIds.erase(taskId);
}

void Threadpool::markTaskAsInProgress(task_id taskId)
{
	std::lock_guard lck(taskListMutex);
	assert(unassignedTasks.find(taskId) != unassignedTasks.end()); //a task must never be attempted to marked as in progress more than once
	assert(inProgressTasks.find(taskId) == inProgressTasks.end()); 

	unassignedTasks.erase(taskId);
	inProgressTasks.insert(taskId);
	reservedTaskIds.erase(taskId);
}

bool Threadpool::isTaskFinished(task_id taskId)
{
	std::lock_guard lck(taskListMutex);
	assert(taskId < lastFreeTaskId); //there's nothing really wrong about asking for finish status of not yet added task, but that's probably a mistake on caller's end
	if (taskId >= lastFreeTaskId) return false; //TODO: maybe throw here?
	return
		reservedTaskIds.find(taskId) == reservedTaskIds.end() &&
		inProgressTasks.find(taskId) == inProgressTasks.end() &&
		unassignedTasks.find(taskId) == unassignedTasks.end();
}

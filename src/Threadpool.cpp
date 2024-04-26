#include "Threadpool.h"
#include <cassert>

using task_id = Threadpool::task_id;
using tast_t = Threadpool::task_t;

Threadpool::Threadpool()
{
	size_t threadCount = std::max(1u, std::thread::hardware_concurrency() - 1); //don't crowd out the main thread, unless it is impossible 
	if (std::thread::hardware_concurrency() == 0) threadCount = 1; //hardware_concurrency can return 0
	this->spawnThreads(threadCount);
}
Threadpool::Threadpool(size_t numThreads)
{
	spawnThreads(numThreads);
}
Threadpool::task_id Threadpool::addTask(task_t taskFunc)
{
	std::lock_guard lck(taskListMutex);
	this->unassignedTasks[lastFreeTaskId++] = taskFunc;

	//std::unique_lock cv_lck(cv_mtx);
	cv.notify_one();

	return lastFreeTaskId - 1;
}
task_id Threadpool::addTask(task_t taskFunc, std::vector<task_id> dependencies)
{
	std::lock_guard lck(taskListMutex);
	this->unassignedTasks[lastFreeTaskId++] = taskFunc;
	task_id id = lastFreeTaskId - 1;

	this->dependenciesMap[id].insert(dependencies.begin(), dependencies.end());
	cv.notify_one();

	return id;
}
void Threadpool::waitUntilTaskCompletes(task_id taskIndex)
{
	/*
	{
		std::lock_guard tasksLck(taskListMutex);
		if (taskIndex >= lastFreeTaskId) throw std::runtime_error("Attempting to wait for non-existant task.");
		auto it = finishedTasks.find(taskIndex);
		if (it != finishedTasks.end())
		{
			finishedTasks.erase(it);
			return; //if task was completed, just return
		}
	}*/

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

		size_t depCount = depIt->second.size();
		size_t finishedDepCount = 0;
		for (auto& depTaskId : depIt->second)
		{
			finishedDepCount += isTaskFinished(depTaskId);
		}

		if (depCount == finishedDepCount) return { candidateTask }; //if the task had dependencies, but they have finished, return
		//else keep looking
	}

	return {}; //no runnable tasks found, return empty optional
}

void Threadpool::markTaskFinished(task_id taskId)
{
	std::lock_guard lck(taskListMutex);
	unassignedTasks.erase(taskId);
	inProgressTasks.erase(taskId);
	dependenciesMap.erase(taskId);
}

void Threadpool::markTaskAsInProgress(task_id taskId)
{
	std::lock_guard lck(taskListMutex);
	assert(unassignedTasks.find(taskId) != unassignedTasks.end()); //a task must never be attempted to marked as in progress more than once
	assert(inProgressTasks.find(taskId) == inProgressTasks.end()); 

	unassignedTasks.erase(taskId);
	inProgressTasks.insert(taskId);
}

bool Threadpool::isTaskFinished(task_id taskId)
{
	std::lock_guard lck(taskListMutex);
	assert(taskId < lastFreeTaskId); //there's nothing really wrong about asking for finish status of not yet added task, but that's probably a mistake on caller's end
	if (taskId >= lastFreeTaskId) return false; //TODO: maybe throw here?
	return 
		inProgressTasks.find(taskId) == inProgressTasks.end() &&
		unassignedTasks.find(taskId) == unassignedTasks.end();
}

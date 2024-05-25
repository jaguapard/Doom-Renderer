#include "Threadpool.h"
#include <cassert>
#include "helpers.h"
#include <iostream>

Threadpool::Threadpool(std::optional<size_t> numThreads)
{
	size_t threadCount = numThreads.value_or(std::max(1u, std::thread::hardware_concurrency() - 1)); //don't crowd out the main thread, unless it is impossible 
	if (std::thread::hardware_concurrency() == 0) threadCount = 1; //hardware_concurrency can return 0
	this->spawnThreads(threadCount);
}

task_id Threadpool::addTask(const taskfunc_t& taskFunc, std::vector<task_id> dependencies, std::optional<task_id> wantedId)
{
	ThreadpoolTask task;
	task.func = taskFunc;
	task.dependencies = dependencies;
	task.wantedId = wantedId;
	return addTask(task);
}

task_id Threadpool::addTask(ThreadpoolTask task)
{
	assert(task.assignedId == -1);
	return this->addTaskBatch({ task }).front();
}

std::vector<task_id> Threadpool::addTaskBatch(const std::vector<ThreadpoolTask>& tasks)
{
	std::vector<task_id> retIds;
	retIds.reserve(tasks.size());

	std::lock_guard lck(taskListMutex);
	for (const auto& task : tasks)
	{
		task_id id;
		if (task.wantedId)
		{
			task_id wid = task.wantedId.value();
			if (reservedTaskIds.find(wid) == reservedTaskIds.end()) throw std::runtime_error("Threadpool: attempted to add task with reserved ID while that ID has not been reserved!");
			reservedTaskIds.erase(wid);
			id = wid;
		}
		else
		{
			id = lastFreeTaskId++;
		}

		ThreadpoolTask& addedTask = this->taskStore[id] = task;
		addedTask.assignedId = id;

		this->unassignedTasks.insert(id);
		retIds.push_back(id);
	}

	cv.notify_all();

	return retIds;
}

std::vector<task_id> Threadpool::reserveTaskIds(uint64_t count)
{
	std::vector<task_id> ret;
	ret.reserve(count);

	std::lock_guard lck(taskListMutex);
	task_id taskId = lastFreeTaskId;

	for (uint64_t i = 0; i < count; ++i)
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

Threadpool::~Threadpool() noexcept
{
	std::cout << "Destroying threadpool...\n";
	this->threadpoolMarkedForTermination = true; //signal all threads that it's time to stop
	{
		std::unique_lock cv_lck(cv_mtx);
		cv.notify_all();
	}

	std::unique_lock cv_lck(cv_mtx);
	cv.wait(cv_lck, [&]() { 
		std::lock_guard taskLck(taskListMutex); //wait until all threads finish. They "signal" their termination by setting a flag in threadTerminated vector
		for (const auto& it : threadTerminated) if (!it) return false;
		return true;
	});
	std::cout << "Threadpool destroyed, bye.\n";
}

void Threadpool::workerRoutine(size_t workerNumber)
{
	while (!this->threadpoolMarkedForTermination)
	{
		task_id myTaskId;
		{ //wait until there are runnable tasks
			std::unique_lock cv_lck(cv_mtx);
			cv.wait(cv_lck, [&]() {
				if (this->threadpoolMarkedForTermination) return true; //wake up instantly if it's time to stop

				std::lock_guard taskLck(taskListMutex); //do everything under the aegis of mutex to avoid other threads stealing our job between conditional awake and us marking the task as in-progress
				auto ret = tryGetTask();
				if (!ret.has_value()) return false;

				myTaskId = ret.value();
				markTaskAsInProgress(myTaskId);
				return true;
			});
		}

		if (this->threadpoolMarkedForTermination) break;

		taskStore[myTaskId].func();
		markTaskFinished(myTaskId);

		std::unique_lock cv_lck(cv_mtx);
		cv.notify_all();
	}

	{
		std::lock_guard taskLck(taskListMutex);
		threadTerminated[workerNumber] = true;
	}

	std::unique_lock cv_lck(cv_mtx);
	cv.notify_all();
}

void Threadpool::spawnThreads(size_t numThreads)
{
	threadTerminated.clear();
	threadTerminated = std::vector<bool>(numThreads, false);

	for (size_t i = 0; i < numThreads; ++i)
		threads.emplace_back(&Threadpool::workerRoutine, this, i);
}

std::optional<task_id> Threadpool::tryGetTask()
{
	std::lock_guard lck(taskListMutex);
	for (const auto& candidateTaskId : unassignedTasks)
	{
		const ThreadpoolTask& candidateTask = taskStore[candidateTaskId];
		if (candidateTask.dependencies.empty()) return candidateTaskId; //if task has no dependendencies, we can just give it to the worker

		for (auto& depTaskId : candidateTask.dependencies)
		{
			if (!isTaskFinished(depTaskId)) goto tryNextTask;
		}

		return { candidateTaskId }; //if the task had dependencies, but they have finished, return
		
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
	reservedTaskIds.erase(taskId);
	taskStore.erase(taskId);
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
		unassignedTasks.find(taskId) == unassignedTasks.end() &&
		inProgressTasks.find(taskId) == inProgressTasks.end() &&
		reservedTaskIds.find(taskId) == reservedTaskIds.end();
}

ThreadpoolTask::ThreadpoolTask(const taskfunc_t& taskFunc, std::vector<task_id> dependencies, std::optional<task_id> wantedId)
	: func(taskFunc), dependencies(dependencies), wantedId(wantedId) 
{};
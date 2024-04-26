#include "Threadpool.h"
#include <SDL/SDL.h>
Threadpool::Threadpool()
{
	this->spawnThreads(std::thread::hardware_concurrency());
}
Threadpool::Threadpool(size_t numThreads)
{
	spawnThreads(numThreads);
}
Threadpool::task_id Threadpool::addTask(task_t taskFunc)
{
	std::lock_guard lck(taskListMutex);
	this->tasks[lastTaskId++] = taskFunc;

	//std::unique_lock cv_lck(cv_mtx);
	cv.notify_one();

	return lastTaskId - 1;
}
void Threadpool::waitUntilTaskCompletes(task_id taskIndex)
{
	{
		std::lock_guard tasksLck(taskListMutex);
		if (taskIndex >= lastTaskId) throw std::runtime_error("Attempting to wait for non-existant task.");
		auto it = finishedTasks.find(taskIndex);
		if (it != finishedTasks.end())
		{
			finishedTasks.erase(it);
			return; //if task was completed, just return
		}
	}

	std::unique_lock cv_lck(cv_mtx);
	cv.wait(cv_lck, [&]() {
		std::lock_guard task_lck(taskListMutex);
		return finishedTasks.find(taskIndex) != finishedTasks.end();
		});

	{
		std::lock_guard tasksLck(taskListMutex);
		finishedTasks.erase(finishedTasks.find(taskIndex));
	}
}
void Threadpool::workerRoutine(size_t workerNumber)
{
	while (true)
	{
		{
			std::unique_lock cv_lck(cv_mtx);
			cv.wait(cv_lck, [&]() {
				std::lock_guard<std::mutex> taskLck(taskListMutex);
				return !tasks.empty();
				});
		}

		std::pair<task_id, task_t> myTask;
		{
			std::lock_guard task_lck(taskListMutex);
			if (tasks.empty()) continue;

			myTask = *tasks.begin();
			tasks.erase(myTask.first);
		}

		myTask.second();
		{
			std::lock_guard task_lck(taskListMutex);
			finishedTasks.insert(myTask.first);
		}

		std::unique_lock cv_lck(cv_mtx);
		cv.notify_all();
	}
}

void Threadpool::spawnThreads(size_t numThreads)
{
	for (size_t i = 0; i < numThreads; ++i)
		threads.emplace_back(&Threadpool::workerRoutine, this, i);
}

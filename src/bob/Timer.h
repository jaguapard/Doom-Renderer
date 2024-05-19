#pragma once
#include <chrono>
//Version 1.0 from Mar 16, 2024
namespace bob
{
	class Timer
	{
	public:
		Timer() //Stats running immediately on creation
		{
			restart();
		}
		Timer(bool startRunning) //You can specify if timer starts running immediately, or wait for start()
		{
			restart(startRunning);
		}
		void restart(bool restartRunning = true) //Start counting time from 0. If restartRunning is false, then it will start counting until resume() has been called 
		{
			if (restartRunning)	lastStartTime = std::chrono::high_resolution_clock::now();
			running = restartRunning;
			time = 0;
		}
		void resume() //resumes the timer. Will not do anything if timer is already running
		{
			if (!running)
			{
				lastStartTime = std::chrono::high_resolution_clock::now();
				running = true;
			}
		}
		void pause() //pause the timer. Will not do anything if timer is already paused
		{
			if (running)
			{
				auto endTime = std::chrono::high_resolution_clock::now();
				running = false;
				std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - lastStartTime);
				double dt = time_span.count();
				this->time += dt;
			}
		}
		double getTime() //returns total accumulated time
		{
			if (running)
			{
				pause();
				double ret = time;
				resume();
				return ret;
			}
			else return time;
		}
		double getTimeSinceLastPoll() //TODO: buggy with restarts
		{
			double t = this->getTime();
			double ret = t - lastPollTime;
			lastPollTime = t;
			return ret;
		}
		bool isRunning() const
		{
			return running;
		}
		void toggle()
		{
			if (running) pause();
			else resume();
		}
	private:
		bool running = false;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastStartTime;
		double lastPollTime = 0;
		double time = 0;
	};
}
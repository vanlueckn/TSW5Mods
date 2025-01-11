#include "timermanager.hpp"

TimerManager& TimerManager::Get()
{
	static TimerManager instance;
	return instance;
}

void TimerManager::SetTimer(int milliseconds, std::function<void()> callback) {
	auto executeTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
	timers.emplace(executeTime, std::move(callback));
}


void TimerManager::Tick() {
	auto now = std::chrono::steady_clock::now();
	while (!timers.empty() && timers.top().first <= now) {
		auto callback = timers.top().second;
		timers.pop();
		callback();
	}
}
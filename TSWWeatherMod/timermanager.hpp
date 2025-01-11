#include <functional>
#include <queue>
#include <chrono>

class TimerManager {
public:
	static TimerManager& Get();
	void SetTimer(int milliseconds, std::function<void()> callback);
	void Tick();
private:
	TimerManager() = default;
	TimerManager(const TimerManager&) = delete;
	TimerManager& operator=(const TimerManager&) = delete;

	using Timer = std::pair<std::chrono::steady_clock::time_point, std::function<void()>>;
	struct Compare {
		bool operator()(const Timer& lhs, const Timer& rhs) const {
			return lhs.first > rhs.first;
		}
	};

	std::priority_queue<Timer, std::vector<Timer>, Compare> timers;
};
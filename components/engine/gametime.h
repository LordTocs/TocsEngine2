#pragma once
#include <chrono>
#include <ratio>

namespace tocs {
namespace engine {

class game_time
{
	float time;
	float dt;
	int frame_number_;
public:

	game_time()
		: time(0)
		, dt(0)
		, frame_number_(1) //Frame number starts at 1 so frame - 1 exits
	{}

	int frame_number() const { return frame_number_; }
	float total_time() const { return time; }
	float delta_time() const { return dt; }

	friend class game_timer;
};

class game_timer
{
	std::chrono::high_resolution_clock::time_point start_time;
	std::chrono::high_resolution_clock::time_point last_time_chrono;
	game_time last_time;
public:
	game_time time() const { return last_time; }


	void advance_frame()
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto dt = now - last_time_chrono;
		auto total = now - start_time;

		last_time_chrono = now;
		
		last_time.time = std::chrono::duration_cast<std::chrono::duration<float>>(total).count();
		last_time.dt = std::chrono::duration_cast<std::chrono::duration<float>>(dt).count();
		++last_time.frame_number_;
	}
};

}}
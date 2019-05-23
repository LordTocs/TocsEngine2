#pragma once
#include "gametime.h"
#include "gameobject.h"
#include "component.h"

namespace tocs {
namespace engine {


//Game state represents one slice in time. It stores which entities are alive for that frame and there associated mappings to components.
class game_state
{
public:
	game_time time;

	static constexpr int num_state_histories = 3;

	//Component pools so that components in the same frame are stored together.
	all_component_storage component_storage;
	live_game_objects live_objects;

	game_state()
	{
	}

	void prepare_frame(const game_state &previous)
	{
		live_objects.prepare_frame(previous.live_objects);
		component_storage.prepare_frame(previous.component_storage);
	}
};


class world
{
	game_timer timer;
	std::unique_ptr<game_state> state_history[game_state::num_state_histories];
public:
	game_object_manager game_objects;
	
	world()
	{
		for (int i = 0; i < game_state::num_state_histories; ++i)
		{
			state_history[i].reset(new game_state());
		}
	}

	game_state &current_state()	{ return state_for_frame(timer.time().frame_number()); }
	const game_state &current_state() const	{ return state_for_frame(timer.time().frame_number()); }

	game_object_id spawn_object()
	{
		//Newly spawned objects enter a temporary pergatory until the end of the current frame.
		//This makes multithreading easier since no new objects can effect existing executing systems.
		//Potentially we could add object existance barriers.

		auto handle = game_objects.alloc_new_object();

		//How to do this concurrently
		current_state().live_objects.push_to_purgatory(handle);

		return handle->get_id();
	}

	template <class T>
	void add_component(game_object_id id)
	{

	}

	void advance_frame()
	{
		timer.advance_frame();

		game_time time = timer.time();
		game_state &state = current_state();
		game_state &prev_state = state_for_frame(time.frame_number() - 1);

		game_objects.move_from_pergatory(prev_state.live_objects.object_purgatory);

		state.prepare_frame(prev_state);
	}

	game_time get_time()
	{
		return timer.time();
	}
private:
	game_state &state_for_frame(int framenumber)
	{
		return *state_history[framenumber % game_state::num_state_histories];
	}

	const game_state &state_for_frame(int framenumber) const
	{
		return *state_history[framenumber % game_state::num_state_histories];
	}
};


}
}
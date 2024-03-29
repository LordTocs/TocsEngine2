// TocsEngine2.cpp : Defines the entry point for the console application.
//

#include <thread>
#include <math/vector.h>
#include <engine/component.h>
#include <engine/gameobject.h>
#include <engine/world.h>

using namespace tocs;

int main()
{
	tocs::math::vector3 v(1, 0, 0);
	
	v.dot(tocs::math::vector3::up);

	engine::world world;
	
	while (true)
	{
		world.advance_frame();

		engine::game_time time = world.get_time();
		engine::game_state &state = world.current_state();

		if (time.frame_number() % 10 == 0)
		{
			engine::game_object_id id = world.spawn_object();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		
	}
	
    return 0;
}


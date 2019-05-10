#pragma once
#include "component.h"
#include "gametime.h"
#include <threading/pool.h>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <thread>

namespace tocs {
namespace engine {

//using game_object_id = std::size_t;

class world;

class game_object
{
private:
	//constructor and destructor are private so we need to grant the freelist access to use them.
	friend class threading::concurrent_pool<game_object>;
	friend class threading::concurrent_pool_item_behaviour<game_object>;
	friend class game_object_manager;
	game_object_id id;

	bool is_root_obj;

	bool marked_for_destroy;

	game_object(game_object_id id)
		: id(id)
		, is_root_obj(false)
		, marked_for_destroy(false)
	{}


public:

	//Game objects get "created" and "destroyed" inbetween frames, so marking for destroy will remove it for the next frame
	void mark_for_destroy()
	{
		marked_for_destroy = true;
	}

	game_object_id get_id() const { return id; }

	friend class live_game_objects;
};

class live_game_objects
{
	std::vector<threading::concurrent_pool_handle<game_object>> live_objects;

	//Object purgatory is where objects go when they're first created mid frame. They're not available until the following frame for full use.
	std::shared_mutex purgatory_mutex;
	std::vector<threading::concurrent_pool_handle<game_object>> object_purgatory;
public:

	void push_to_purgatory(threading::concurrent_pool_handle<game_object> obj)
	{
		std::unique_lock<std::shared_mutex> lock(purgatory_mutex);
		object_purgatory.push_back(obj);
	}

	void prepare_frame(const live_game_objects &previous)
	{
		live_objects.clear();
		live_objects.reserve(previous.live_objects.size() + previous.object_purgatory.size());

		for (auto &obj : previous.object_purgatory)
		{
			live_objects.push_back(obj);
		}

		for (auto &obj : previous.live_objects)
		{
			if (!obj->marked_for_destroy)
			{
				live_objects.push_back(obj);
			}
		}
	}

	friend class world;
};


class game_object_manager
{
	std::atomic<game_object_id> last_id;
	threading::concurrent_pool<game_object> objects;

	std::unordered_map<game_object_id, threading::concurrent_pool_handle<game_object>> id_to_object;
public:
	friend class world;

	game_object &from_id(game_object_id id)
	{
		//std::shared_lock<std::shared_mutex> lock{ map_mutex };
		auto i = id_to_object.find(id);
		return *(i->second);
	}

	const game_object &from_id(game_object_id id) const
	{
		//std::shared_lock<std::shared_mutex> lock(map_mutex);
		auto i = id_to_object.find(id);
		return *(i->second);
	}

private:
	threading::concurrent_pool_handle<game_object> alloc_new_object()
	{
		game_object_id id = last_id++;
	
		return objects.get_item(id);
	}

	//Assumes no one else will be using any object stuff
	void move_from_pergatory(std::vector<threading::concurrent_pool_handle<game_object>> &objects)
	{
		for (threading::concurrent_pool_handle<game_object> &obj : objects)
		{
			id_to_object.emplace(std::make_pair(obj->get_id(), obj));
		}
	}
};



}
}

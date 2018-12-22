#pragma once
#include "component.h"
#include <threading/pool.h>

namespace tocs {
namespace engine {

using game_object_id = std::size_t;


class game_object
{

private:
	//constructor and destructor are private so we need to grant the freelist access to use them.
	friend class threading::concurrent_pool<game_object>;
	game_object_id id;

	game_object()
	{

	}

	~game_object()
	{
	}

public:
	template <class T>
	T &get_component()
	{
		return T::storage.components.get(T::storage.object_to_component[handle]);
	}

	template <class T>
	const T &get_component() const
	{
		return T::storage.components.get(T::storage.object_to_component[handle]);
	}

	template <class T>
	bool has_component()
	{
		return T::storage.object_to_component.find(handle) != T::storage.object_to_component.end();
	}

	template <class T>
	void add_component()
	{
		check(!has_component<T>())
			auto comp = T::storage.components.emplace(*this);
		T::storage.object_to_component[id] = comp.get_index();
	}

	template <class T, class... Args>
	void add_component(Args... &&args)
	{
		std::shared_ptr<T> comp = T::storage.components.get_item(*this, std::forward<Args>(args)...);
		T::storage.object_to_component[id] = comp;
	}


	void mark_for_death()
	{

	}
};

class prefab
{
public:

};


class game_object_manager
{
	threading::concurrent_pool<game_object> objects;
public:
	std::shared_ptr<game_object> new_object()
	{
		return objects.get_item();
	}

	std::shared_ptr<game_object> spawn(const prefab &object)
	{
		return nullptr;
	}
};


}
}

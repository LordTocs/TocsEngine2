#pragma once
#include <type_traits>
#include "state.h"
#include <math/transform.h>
#include <threading/pool.h>
#include <core/asserts.h>

namespace tocs {
namespace engine {

class game_object;

class component_base
{
};

template <class comp_type>
class component_meta_data
{
public:
	component_meta_data()
	{
		//self register the component.
		comp_type::init_component_meta_data();
	}
};


class component_store_base
{
public:
	virtual void remove_component_for_game_object(game_object_handle id) = 0;
	static std::vector<component_store_base *> component_stores;
};


template <class comp_type>
class component_store : public component_store_base
{
	concurrent_pool<comp_type> components;
	std::unordered_map<game_object *, std::shared_ptr<T>> object_to_component;
public:

	component_store()
	{
		component_store_base::component_stores.push_back(this);
	}

	friend class component;
	friend class comp_type;
	friend class game_object;

	void remove_component_for_game_object(game_object_handle handle) final override
	{
		auto i = object_to_component.find(handle);

		if (i != object_to_component.end())
		{
			components.remove(*i);
			object_to_component.erase(i);
		}
	}
};



template <class comp_type>
class component : public component_base
{
	game_object *object;
public:
	//static component_meta_data<comp_type> meta_data;
	static component_store<comp_type> storage;

	component(game_object &object)
		: object(&object)
	{}

	game_object &get_game_object() const { return *object; }

};

template <class comp_type>
component_meta_data<comp_type> component<comp_type>::meta_data{};



}
}
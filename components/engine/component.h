#pragma once
#include <type_traits>
#include "state.h"
#include "gametime.h"
#include <threading/pool.h>
#include <core/asserts.h>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <typeindex>

namespace tocs {
namespace engine {

class game_object;
using game_object_id = std::size_t;

template <class comp_type>
class component_mapping
{
	//Todo, can we do map access lockelessly?
	std::shared_mutex map_mutex;
	std::unordered_map<game_object_id, threading::concurrent_pool_handle<game_object>> obj_to_comp;
public:

	void match_allocations(const component_mapping<comp_type> &other_mapping)
	{
		std::unique_lock lock(map_mutex);

		for (auto &m : other_mapping.obj_to_comp)
		{
			auto i = obj_to_comp.find(m.first);

			if (i == obj_to_comp.end())
			{
				//An object got created, match it
				obj_to_comp.insert(std::make_pair(m.first, comp_type::manager.alloc_component(m.first)));
			}
		}

		for (auto &m : obj_to_comp)
		{
			auto i = other_mapping.obj_to_comp.find(m.first);

			if (i == other_mapping.obj_to_comp.end())
			{
				//An object got deleted, match it
				obj_to_comp.erase(m.first);
			}
		}
	}
};

class base_component_storage
{
public:
	virtual ~base_component_storage() {}

	virtual void prepare_frame(const base_component_storage &prev_storage) = 0;
};

template <class comp_type>
class component_storage : public base_component_storage
{
	threading::concurrent_pool<comp_type> storage;
	component_mapping<comp_type> mapping;
public:
	component_storage()
	{
	}

	void prepare_frame(const base_component_storage &prev_component_storage_untyped) override
	{
		const component_storage<comp_type> *prev_storage = dynamic_cast<const component_storage<comp_type> *> (&prev_component_storage_untyped);

		check(prev_storage != nullptr);
		//Setup the next frame by matching any existing components to the prev frame, avoiding new allocations if we don't need it.
		mapping.match_allocations(prev_storage->mapping);
	}

private:

	threading::concurrent_pool_handle<comp_type> alloc_component(game_object_id id)
	{
		return storage.get_item(nullptr);
	}

	static base_component_storage *factory_func()
	{
		return new component_storage<comp_type>{};
	}
};

class all_component_storage
{
	std::unordered_map<std::type_index, std::unique_ptr<base_component_storage>> component_storages;
	static std::vector<std::pair<std::type_index, std::function<base_component_storage *()>>> storage_factories;

	template <class comp_type>
	static void init_factory()
	{
		storage_factories.emplace_back(std::make_pair(std::type_index(typeid(comp_type)), component_storage<comp_type>::factory_func));
	}

public:
	all_component_storage()
	{
		for (auto fp : storage_factories)
		{
			component_storages.emplace(fp.first, fp.second());
		}
	}

	template <class comp_type>
	void alloc_component(game_object *obj)
	{
		base_component_storage *base_storage = component_storages[std::type_index(typeid(comp_type))];
		component_storage<comp_type> *storage = static_cast<component_storage<comp_type> *> (base_storage);

		storage->alloc_component(obj);
	}

	void prepare_frame(const all_component_storage &previous)
	{
		for (auto &pair : component_storages)
		{
			pair.second->prepare_frame(*previous.component_storages.find(pair.first)->second);
		}
	}
};

template <class comp_type>
class storage_factory_initializer
{
public:
	storage_factory_initializer()
	{
		all_component_storage::init_factory<comp_type>();
	}
};


template <class comp_type>
class component
{
	game_object *object;
	static storage_factory_initializer<comp_type> factory_initer;
public:

	component(game_object &object)
		: object(&object)
	{}

	game_object &get_game_object() const { return *object; }
};

template <class comp_type>
storage_factory_initializer<comp_type> component<comp_type>::factory_initer;


}
}
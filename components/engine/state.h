#pragma once
#include <vector>
#include <memory>
#include <bitset>

#include "Serializer.h"
#include "Interpolation.h"

namespace tocs {
namespace engine {

template<class ValType>
class state_value
{
	bool changed;
	ValType value;
public:

	state_value() : value(), changed(false) {}
	explicit state_value(const ValType &value) : value(value), changed(false) {}

	typedef ValType underlying_type;

	operator ValType () const
	{
		//read
		return value;
	}

	const ValType &get_value() const { return value; }

	state_value<ValType> &operator=(const ValType &new_value)
	{
		changed = true;
		value = new_value;
	}

	bool has_changed() const { return changed; }

	//SFINAE enabled operators.
};

class state_object_diff
{
public:
	std::bitset<64> changed_values;
	std::vector<unsigned char> value_memory;
};

template <class outer_type>
class base_state_value_metadata
{
public:
	int value_index;
	std::string value_name;

	base_state_value_metadata(int value_index, const std::string &value_name)
		: value_index(value_index)
		, value_name(value_name)
	{}

	virtual ~base_state_value_metadata() {}

	virtual size_t data_size() const = 0;

	virtual void serialize(outer_type *obj, void *data) = 0;

	//We might be able to avoid a virtual call here if we know the dirty flag is always the first thing in the state_value<>
	virtual bool is_dirty(outer_type *obj) const = 0;
};


template <class outer_type, class type, class interpolation_type = no_interp<type>, class serialization_type = serializer<type>>
class state_value_meta_data_constructor
{
public:
	const char *name;
	state_value<type> outer_type::*value_ptr;

	template<int n>
	constexpr state_value_meta_data_constructor(state_value<type> outer_type::* value_ptr, const char(&name)[n])
		: name(&name[0])
		, value_ptr(value_ptr)
	{}

	constexpr state_value_meta_data_constructor(const state_value_meta_data_constructor &copyme) = default;

	template <class new_interp>
	constexpr auto interpolation() const
	{
		return state_value_meta_data_constructor<outer_type, type, new_interp, serialization_type>(*this);
	}

	template <class new_serialization>
	constexpr auto serialization() const
	{
		return state_value_meta_data_constructor<outer_type, type, interpolation_type, new_serialization>(*this);
	}
};

#define STATE_REGISTRATION(NAME) \
state_value_meta_data_constructor< \
decltype(state)::meta_data_type::obj_type, \
decltype(NAME)::underlying_type> \
(&decltype(state)::meta_data_type::obj_type::NAME, #NAME)

template <class type, class outer_type, class interpolation = no_interp, class type_serializer = serializer<type>>
class state_value_metadata : public base_state_value_metadata<outer_type>
{
public:
	state_value<type> outer_type::*value_ptr;

	state_value_metadata(state_value<type> outer_type::* value_ptr, int index, const std::string &value_name)
		: base_state_value_metadata<outer_type>(index, value_name)
		, value_ptr(value_ptr)
	{}

	size_t data_size() const final override
	{
		return type_serializer::size_in_bytes();
	}

	void serialize(outer_type *obj, void *data) final override
	{
		type_serializer::write((obj->*value_ptr).get_value(), data);
	}

	bool is_dirty(outer_type *obj) const final override
	{
		return (obj->*value_ptr).has_changed();
	}
};

template <class outer_type>
class state_object_metadata
{
public:
	std::vector<std::unique_ptr<base_state_value_metadata<outer_type>>> values;

	typedef outer_type obj_type;

	state_object_metadata()
	{
		outer_type::init_state_metadata();
	}

	template <class type, class interpolation, class serializer>
	void register_value(state_value_meta_data_constructor<outer_type, type, interpolation, serializer> & value_data)
	{
		values.emplace_back(new state_value_metadata<type, outer_type, interpolation, serializer>(value_data.value_ptr, values.size(), value_data.name));
	}

	state_object_diff create_diff(outer_type &obj)
	{
		state_object_diff result;
		size_t result_binary_size = 0;

		for (auto &value : values)
		{
			if (value->is_dirty(&obj))
			{
				result.changed_values[value->value_index] = true;
				result_binary_size += value->data_size();
			}
		}

		//TODO: Some sort of allocation pool?
		result.value_memory.resize(result_binary_size, 0);
		unsigned char *data_ptr = &result.value_memory[0];

		for (auto &value : values)
		{
			if (value->is_dirty(&obj))
			{
				value->serialize(&obj, data_ptr);
				data_ptr += value->data_size();
			}
		}
	}
};


template<class outer_type>
class state_object
{
public:
	typedef state_object_metadata<outer_type> meta_data_type;
	static meta_data_type meta_data;

	int priority;

};


//register(var_name)
	//.interp<interp_type>()
	//.serializer<serial_type>()
	//.replication


//Heirarchical state
//
//state diffs vs state history
//state diffs computed for updates

//diffs for history?
//cache last update for each value?

//difference between volatile_state and state?
//volatile stored per frame
//normal state stored per update and cached update indexes.

}
}
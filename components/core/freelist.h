#pragma once
#include <limits>
#include <mutex>
#include <atomic>
namespace tocs {


using free_list_index = unsigned int;
static constexpr free_list_index free_list_invalid_index = std::numeric_limits<free_list_index>::max();

template <class T>
class free_list
{
	struct list_item
	{
		unsigned char data[T]
		free_list_index next;
		bool allocated;

		list_item()
			: next(free_list_invalid_index)
			, allocated(false)
		{}

		T &item() { return *static_cast<T*> (&data[0]); }
		const T &item() const { return *static_cast<T*> (&data[0]); }
	};

	static constexpr int page_num_items = 100;
	
	struct item_page
	{
		list_item items[page_num_items];
	};

	std::vector<std::unique_ptr<item_page>> pages;

	free_list_index head;
	free_list_index max_alloced;

	list_item &get_item(free_list_indext index)
	{
		return pages[index / page_num_items]->items[index % page_num_items];
	}

	const list_item &get_item(free_list_index index) const
	{
		return pages[index / page_num_items]->items[index % page_num_items];
	}

	void ensure_max_alloced()
	{
		if (max_alloced >= pages.size() * page_num_items)
		{
			pages.emplace_back(new item_page());
		}
	}

public:

	friend class iterator;
	class iterator
	{
		free_list<T> *list;
		free_list_index index;
	public:
		iterator()
			: list(nullptr)
			, index(free_list_invalid_index)
		{}
	private:
		iterator(free_list<T> *list, free_list_index index)
			: list(list)
			, index(index)
		{}
	public:

		friend class free_list<T>;

		free_list_index get_index() const { return index; }

		T *operator->()
		{
			return &list->get_item(index).item();
		}

		T &operator*()
		{
			return list->get_item(index).item();
		}

		iterator &operator++()
		{
			do
			{
				++index;
				if (index >= max_alloced)
					return *this;
			} while (!get_item(index).allocated);
			return *this;
		}

		iterator operator++(int)
		{
			iterator result = *this;
			++(*this);
			return result;
		}
	};

	free_list()
		: head(free_list_invalid_index)
		, max_alloced(0)
	{
		pages.emplace_back(new item_page());
	}

	iterator add(const T &value)
	{
		free_list_index index = head;

		if (index == ~0U)
		{
			//Check if we need to alloc a new page.
			ensure_max_alloced();
			index = max_alloced++;
		}

		list_item &new_item = get_item(index);

		new (new_item.data) T(value);
		head = new_item.next;
		new_item.allocated = true;

		return iterator result(this, index);
	}

	template <class ...Args>
	iterator emplace(Args... &&args)
	{
		free_list_index index = head;

		if (index == ~0U)
		{
			//Check if we need to alloc a new page.
			ensure_max_alloced();
			index = max_alloced++;
		}

		list_item &new_item = get_item(index);

		new (new_item.data) T(std::forward<Args>(args)...);
		head = new_item.next;
		new_item.allocated = true;

		return iterator result(this, index);;
	}

	void remove(free_list_index index)
	{
		get_item(index).item().~T();
		get_item(index).next = head;
		get_item(index).allocated = false;
		head = index;
	}

	void remove(iterator it)
	{
		remove(it.index);
	}

	iterator begin()
	{
		iterator result;
		if (!pages[0]->items[0].allocated)
			result++;

		return result;
	}

	iterator end()
	{
		return iterator(this, max_alloced);
	}

	T &get(free_list_index index)
	{
		return get_item(index).item();
	}

	const T &get(free_list_index index) const
	{
		return get_item(index).item();
	}
};

}

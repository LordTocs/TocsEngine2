#pragma once
#include <atomic>
#include <memory>
#include <mutex>
namespace tocs {
namespace threading {

namespace detail {

	//Contains modded copy pastes from: http://moodycamel.com/blog/2014/solving-the-aba-problem-for-lock-free-free-lists
	//Licensed under BSD

	template <class T>
	class concurrent_free_list
	{
		static const std::uint32_t REFS_MASK = 0x7FFFFFFF;
		static const std::uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;
	public:
		class free_list_node
		{
		public:
			std::aligned_storage<T, alignof(T)>::type data;

			std::atomic<std::uint32_t> refs;
			std::atomic<free_list_node*> next;

			bool constructed;

			T &item() { return *static_cast<T*> (&data); }
			const T &item() const { return *static_cast<T*> (&data); }

			free_list_node()
				: refs(0)
				, next(nullptr)
				, constructed(false)
			{}

			~free_list_node()
			{
				storage
				if (constructed)
				{
					item().~T();
				}
			}
		};
	private:
		std::atomic<free_list_node*> head;
	public:
		inline void add_node(free_list_node *node)
		{
			// We know that the should-be-on-freelist bit is 0 at this point, so it's safe to
			// set it using a fetch_add
			if (node->refs.fetch_add(SHOULD_BE_ON_FREELIST, std::memory_order_release) == 0)
			{
				add_knowing_refcount_is_zero(node);
			}
		}

		inline void add_knowing_refcount_is_zero(free_list_node *node)
		{
			// Since the refcount is zero, and nobody can increase it once it's zero (except us, and we
			// run only one copy of this method per node at a time, i.e. the single thread case), then we
			// know we can safely change the next pointer of the node; however, once the refcount is back
			// above zero, then other threads could increase it (happens under heavy contention, when the
			// refcount goes to zero in between a load and a refcount increment of a node in try_get, then
			// back up to something non-zero, then the refcount increment is done by the other thread) --
			// so, if the CAS to add the node to the actual list fails, decrease the refcount and leave
			// the add operation to the next thread who puts the refcount back at zero (which could be us,
			// hence the loop).
			auto head_ptr = head.load(std::memory_order_relaxed);
			while (true)
			{
				node->next.store(head_ptr, std::memory_order_relaxed);
				node->refs.store(1, std::memory_order_release);
				if (!head.compare_exchange_strong(head_ptr, node, std::memory_order_release, std::memory_order_relaxed))
				{
					// Hmm, the add failed, but we can only try again when the refcount goes back to zero
					if (node->refs.fetch_add(SHOULD_BE_ON_FREELIST - 1, std::memory_order_release) == 1)
					{
						continue;
					}
				}
				return;
			}
		}

		inline free_list_node* try_get()
		{
			auto head_ptr = head.load(std::memory_order_acquire);

			while (head_ptr != nullptr)
			{
				auto prevHead = head_ptr;
				auto refs = head_ptr->refs.load(std::memory_order_relaxed);

				if ((refs & REFS_MASK) == 0 || !head_ptr->refs.compare_exchange_strong(refs, refs + 1, std::memory_order_acquire, std::memory_order_relaxed))
				{
					head_ptr = head.load(std::memory_order_acquire);
					continue;
				}
				std::allocate_shared()

				// Good, reference count has been incremented (it wasn't at zero), which means
				// we can read the next and not worry about it changing between now and the time
				// we do the CAS
				auto next = head_ptr->next.load(std::memory_order_relaxed);
				if (head.compare_exchange_strong(head_ptr, next, std::memory_order_acquire, std::memory_order_relaxed))
				{
					// Yay, got the node. This means it was on the list, which means
					// shouldBeOnFreeList must be false no matter the refcount (because
					// nobody else knows it's been taken off yet, it can't have been put back on).
					assert((head->refs.load(std::memory_order_relaxed) & SHOULD_BE_ON_FREELIST) == 0);

					// Decrease refcount twice, once for our ref, and once for the list's ref
					head_ptr->refs.fetch_add(-2, std::memory_order_relaxed);

					return head_ptr;
				}

				// OK, the head must have changed on us, but we still need to decrease the refcount we
				// increased
				refs = prevHead->refs.fetch_add(-1, std::memory_order_acq_rel);
				if (refs == SHOULD_BE_ON_FREELIST + 1)
				{
					add_knowing_refcount_is_zero(prevHead);
				}
			}

			return nullptr;
		}
	};


	template <class T>
	class concurrent_pool_storage
	{
	public:
		class node_page
		{
		public:
			static const int max_node_count;

			std::atomic<std::uint32_t> used_node_count;
			detail::concurrent_free_list<T>::free_list_node nodes[max_node_count];
			std::atomic<node_page*> next_page;

			node_page()
				: used_node_count(0)
				, next_page(nullptr)
			{
				for (int i = 0; i < max_node_count; ++i)
				{
					if (nodes[i].constructed)
					{
						nodes[i].item().~T();
					}
				}
			}
		};
	private:
		node_page *first_page;
		node_page *tail_page;
		std::atomic<std::uint32_t> page_count;
	public:
		concurrent_pool_storage()
			: first_page(new node_page())
			, tail_page(first_page)
			, page_count(1)
		{}


		~concurrent_pool_storage()
		{
			node_page *page = first_page;

			do
			{
				node_page *next = page->next_page;
				delete page;
				page = next;
			} while (page);

		}


		detail::concurrent_free_list<T>::free_list_node *fetch_new_node()
		{
			while (true)
			{
				std::uint32_t node_slot = tail_page->used_node_count.fetch_add(1);

				if (node_slot == node_page::max_node_count - 1)
				{
					//We got the last node in the page so we alloc a new one for the next fetch.
					node_page *new_page = new node_page();

					detail::concurrent_free_list<T>::free_list_node* result_node = tail_page->nodes[node_slot];

					tail_page.next_page = new_page;
					tail_page = new_page;
					++page_count;

					return result_node;
				}
				else if (node_slot >= node_page::max_node_count)
				{
					//We're past the end of the last page, but we're not the first thread to get to it. Now we have to wait :/
					std::this_thread::yield();
				}
				else
				{
					return &tail_page->nodes[node_slot];
				}
			}
		}
	};
}

template <class T>
class concurrent_pool_item_behaviour
{
public:
	template <class... Args>
	void on_fetch(T *data, bool &construction_state, Args... &&args)
	{
		check(construction_state == false);
		new (data) T(std::forward<Args>(args)...);
		construction_state = true;
	}

	void on_return(T *data, , bool &construction_state)
	{
		check(construction_state == true);
		data->~T();
		construction_state = false;
	}
};


template <class T>
using pool_ptr = std::shared_ptr<T>;

template <class T, class ItemBehaviour = concurrent_pool_item_behaviour<T>>
class concurrent_pool
{
	void alloc_node()
	{
		free_list.add_node(*item_storage.fetch_new_node());
	}

	ItemBehaviour item_behaviour;
	detail::concurrent_free_list<T> free_list;
	detail::concurrent_pool_storage<T> item_storage;
public:

	template<class... Args>
	shared_ptr<T> get_item(Args... &&args)
	{
		free_list_node *new_item = nullptr;
		while ((new_item = free_list.try_get()) == nullptr)
		{
			alloc_node();
		}

		check(new_item != nullptr);
		check(!new_item->constructed);

		item_behaviour.on_fetch(&new_item->item(), new_item->constructed, std::forward<Args>(args)...);

		return std::shared_ptr<T> {&new_item->item(), [this, new_item](T*)
		{ 
			this->item_behaviour.on_return(&new_item->item(), new_item->constructed);
			this->add_node(new_item);  
		}};
	}
};

}}

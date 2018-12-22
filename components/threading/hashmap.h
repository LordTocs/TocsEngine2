#pragma once
#include <unordered_map>
#include "cacheline.h"

namespace tocs {
namespace threading {

template <class KeyType, class ValueType>
class wait_free_hashmap
{
public:

private:
	class bucket_data
	{
	public:
		KeyType key;
		ValueType value;

		template<class K, class... A>
		bucket_data(K&& k, A&&... args)
			: key(std::forward<K>(k))
			, value(std::forward<A>(args)...)
		{}
	};

	struct bucket
	{
		std::size_t hash;
		bucket_data *data;
	};

	static constexpr int num_buckets;
	static constexpr int elements_per_vbucket = sizeof(cache_line_padding) / num_buckets;

	class virtual_bucket
	{
	public:
		std::array<std::atomic<bucket>, num_buckets> buckets;
		std::atomic<virtual_bucket *> next;

		virtual_bucket()
			: buckets { bucket{, nullptr}}
			, next(nullptr)
		{}

		~virtual_bucket()
		{
			virtual_bucket *maybe_next = next.load();
			if (maybe_next)
				delete maybe_next;

			for (auto &bucket_holder : buckets)
			{
				bucket *b = bucket_holder.load();
				if (b->data)
					delete b->data;

			}
		}
	};
};


}
}

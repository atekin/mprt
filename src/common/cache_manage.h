#ifndef decode_list_manage_h__
#define decode_list_manage_h__

#include <queue>
#include <unordered_map>

#include "common/common_defs.h"
#include "common/cache_buffer.h"

template <typename cache_item>
class cache_manage
{
public:
	using cache_item_t = cache_item;

private:
	size_type _max_cache_count;
	std::unordered_map<url_id_t, cache_item_t> _usage_cache_index;
	std::queue<cache_item_t> _free_cache_list;

	template<typename create_func>
	cache_item_t get_cache_helper(create_func f)
	{
		cache_item_t cache_item_;
		if (_free_cache_list.empty())
		{
			BOOST_LOG_TRIVIAL(debug) << "creating new cache";
			cache_item_ = f();
		}
		else
		{
			BOOST_LOG_TRIVIAL(debug) << "NOT creating new cache";

			cache_item_ = _free_cache_list.front();
			_free_cache_list.pop();
		}

		return cache_item_;
	}

public:
	cache_manage(size_type max_cache_count)
		: _max_cache_count(max_cache_count)
	{

	}

	bool is_in_cache(url_id_t url_id)
	{
		return (_usage_cache_index.find(url_id) != _usage_cache_index.end());
	}

	cache_item_t get_from_cache(url_id_t url_id)
	{
		cache_item_t cache_item_;

		auto iter = _usage_cache_index.find(url_id);
		if (iter == _usage_cache_index.end())
		{
			cache_item_ = cache_item_t();
		}
		else
		{
			cache_item_ = iter->second;
		}

		return cache_item_;
	}

	template <typename create_func>
	cache_item_t get_from_cache(create_func f, url_id_t url_id)
	{
		cache_item_t cache_item_;

		auto iter = _usage_cache_index.find(url_id);
		if (iter == _usage_cache_index.end() || (iter != _usage_cache_index.end() && !iter->second))
		{
			cache_item_ = get_cache_helper<create_func>(f);
			_usage_cache_index[url_id] = cache_item_;
		}
		else
		{
			cache_item_ = iter->second;
		}

		return cache_item_;
	}

	template <typename cache_item_finish_touch_func>
	void put_cache_back(cache_item_finish_touch_func f, url_id_t url_id)
	{
		auto iter = _usage_cache_index.find(url_id);

		if (iter != _usage_cache_index.end())
		{
			cache_item_t cache_item_;
			cache_item_ = iter->second;
			f(cache_item_);
			_usage_cache_index.erase(iter);

			if (_free_cache_list.size() < _max_cache_count)
			//if (_free_cache_list.size() < 5)
			{
				_free_cache_list.push(cache_item_);
			}
		}
	}

	void reset()
	{
		clear_queue(_usage_cache_index);
		clear_queue(_free_cache_list);
	}
};

#endif // decode_list_manage_h__

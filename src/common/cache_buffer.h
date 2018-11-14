#ifndef cache_buffer_h__
#define cache_buffer_h__

#include <cstdint>
#include <memory>
#include <thread>

#include <boost/circular_buffer_fwd.hpp>
#include <boost/log/trivial.hpp>
#include <boost/optional.hpp>

#include "common_defs.h"
#include "producerconsumerqueue.h"

using chunk_buffer_type = std::pair<size_type /*byte_start_position*/, boost::circular_buffer<buffer_elem_t>>;
using shared_chunk_buffer_t = std::shared_ptr<chunk_buffer_type>;
using input_buffer_type_tagged = folly::ProducerConsumerQueue<shared_chunk_buffer_t>;
using shared_input_buffer_type_tagged = std::shared_ptr<input_buffer_type_tagged>;

template <typename Container>
class cache_buffer {
public:
	using value_type = typename Container::value_type;
	using Container_shared = typename std::shared_ptr<Container>;

private:
	using rotate_func_t = void (cache_buffer<Container>::*)();
	size_type _buffer_size;
	size_type _elem_size;
	size_type _buffer_size_count;

	std::atomic<size_type> _total_bytes_in_buffer;

	Container_shared _data_buffer;
	Container_shared _cache_buffer;

	size_type _cache_last_size;
	size_type _data_last_size;

	constexpr static bool _is_empty_check = true;

	void clear_buffers() {
		clear_data();
	}

	void init_buffer(Container_shared & buffer) 
	{
		while(!buffer->isFull()) {
			auto shared_chunk_buffer =
				std::make_shared<typename value_type::element_type>(std::make_pair(0, static_cast<std::size_t>(_elem_size)));
			buffer->write(shared_chunk_buffer);
		}
	}

	void init_cache_buffer() {
		init_buffer(_cache_buffer);
	}

	std::pair<bool, size_type> get_circ_buf_enough(value_type const& circ_buf, size_type size)
	{
		size_type item_count{ 0 };
		bool circ_buffer_enough{ false };

		if (circ_buf->second.size() > size) {
			item_count = size;
			circ_buffer_enough = true;
		}
		else {
			item_count = circ_buf->second.size();
			circ_buffer_enough = false;
		}

		return std::make_pair(circ_buffer_enough, item_count);
	}

	void update_size(size_type up_size) {
		_total_bytes_in_buffer += up_size;
	}
	
	void rotate_cache()
	{
		value_type val;
		bool read_ok = _cache_buffer->read(val);
		if (read_ok && !val->second.empty())
		{
			_data_buffer->write(val);
		}
	}

	void rotate_data()
	{
		value_type val;
		bool read_ok = _data_buffer->read(val);
		if (read_ok)
		{
			val->second.clear();
			_cache_buffer->write(val);
		}
	}

	void put_gen_ptr(
		value_type * buf_ptr,
		size_type last_size,
		rotate_func_t rotate_func,
		bool rotate,
		bool is_empty_full_check)
	{
		if (buf_ptr)
		{
			size_type size_diff = ((*buf_ptr)->second.size() - last_size);
			_total_bytes_in_buffer += size_diff;
			bool check_func_result = is_empty_full_check ? (*buf_ptr)->second.empty() : (*buf_ptr)->second.full();
			if (check_func_result || rotate) {
				(this->*rotate_func)();
			}
		}
	}

	value_type * get_gen_prt(Container_shared & cont, size_type & last_size)
	{
		if (cont->isEmpty())
			return nullptr;

		auto buf_ptr = cont->frontPtr();
		last_size = (*buf_ptr)->second.size();
		return buf_ptr;
	}

	

public:
	cache_buffer(size_type buffer_size, size_type elem_size)
		: _buffer_size(buffer_size)
		, _elem_size(elem_size)
		, _buffer_size_count((_buffer_size / _elem_size) + (_buffer_size % _elem_size != 0))
		, _data_buffer(std::make_shared<Container>(static_cast<std::size_t>(_buffer_size_count + 1)))
		, _cache_buffer(std::make_shared<Container>(static_cast<std::size_t>(_buffer_size_count + 1)))
		, _total_bytes_in_buffer(0)

	{
		init_cache_buffer();
	}

	void reset_buffer() 
	{
		clear_buffers();
	}

	// producer
	value_type* get_cache_ptr()
	{
		return get_gen_prt(_cache_buffer, _cache_last_size);
	}

	void put_cache_ptr(bool rotate)
	{
		put_gen_ptr(_cache_buffer->frontPtr(), _cache_last_size, &cache_buffer<Container>::rotate_cache, rotate, !_is_empty_check);
	}

	// consumer part
	value_type* get_data_ptr()
	{
		return get_gen_prt(_data_buffer, _data_last_size);
	}

	void put_data_ptr(bool rotate)
	{
		put_gen_ptr(_data_buffer->frontPtr(), _data_last_size, &cache_buffer<Container>::rotate_data, rotate, _is_empty_check);
	}

	size_type write_into_raw_buffer(buffer_elem_t *buffer, size_type size, bool remove_data = true)
	{
		size_type item_count{ 0 };
		bool circ_buffer_enough{ false };

		if (_data_buffer->isEmpty()) {
			return item_count;
		}

		// we received the data ptr here ... we should put back
		auto data_circ_buf = *get_data_ptr();
		
		auto enough_size = get_circ_buf_enough(data_circ_buf, size);
		circ_buffer_enough = enough_size.first;
		item_count = enough_size.second;

		auto begin_point = data_circ_buf->second.linearize();
		std::memcpy(buffer, begin_point, static_cast<std::size_t>(item_count));
		// we should remove the related bytes from the memory
		if (remove_data)
		{
			data_circ_buf->second.erase_begin(static_cast<std::size_t>(item_count));
			data_circ_buf->first -= item_count;
		}
		put_data_ptr(false);

		if (
			(!circ_buffer_enough)
			&& (size - item_count > 0)
			) {
			return (item_count + write_into_raw_buffer(buffer + item_count, size - item_count, remove_data));
		}
		
		return item_count;
	}

	void discard_data_upto(size_type seek_point)
	{
		if (seek_point <= 0)
			return;

		decltype(get_data_ptr()) data_ptr;
		while ((data_ptr = get_data_ptr()) == nullptr)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		auto discard_data_length = std::min(static_cast<std::size_t>(seek_point), (*data_ptr)->second.size());
		(*data_ptr)->second.erase_begin(discard_data_length);
		(*data_ptr)->first -= discard_data_length;
		put_data_ptr(false);

		seek_point -= discard_data_length;

		if (seek_point > 0)
		{
			discard_data_upto(seek_point);
		}
	}

	// 	void clear_data_with_seek(size_type seek_point)
// 	{
// 		while (auto data_ptr = get_data_ptr())
// 		{
// 			if (seek_point != (*data_ptr)->first)
// 				(*data_ptr)->second.clear();
// 
// 			put_data_ptr(true);
// 		}
// 	}

	void clear_data()
	{
		while (auto data_ptr = get_data_ptr())
		{
			(*data_ptr)->second.clear();

			put_data_ptr(true);
		}
	}

	void clear_cache()
	{
		if (auto cache_ptr = get_cache_ptr())
		{
			(*cache_ptr)->second.clear();
			put_cache_ptr(false);
		}
	}

	void sync_cache()
	{
		if (auto cache_ptr = get_cache_ptr())
		{
			put_cache_ptr(true);
		}
	}

	bool is_data_full() {
		return _data_buffer->isFull();
	}

	bool is_data_empty() {
		return _data_buffer->isEmpty();
	}

	bool is_cache_full() {
		return _cache_buffer->isFull();
	}

	bool is_cache_empty() {
		return _cache_buffer->isEmpty();
	}
	
	size_type total_bytes_in_buffer_guess()
	{
		return _total_bytes_in_buffer;
	}

	size_type buffer_size_count()
	{
		return _buffer_size_count;
	}

	size_type elem_size()
	{
		return _elem_size;
	}

	size_type data_size_guess()
	{
		return _data_buffer->sizeGuess();
	}

	size_type cache_size_guess()
	{
		return _cache_buffer->sizeGuess();
	}

	size_type available_bytes()
	{
		return _buffer_size - _total_bytes_in_buffer;
	}
};

using cache_buffer_t = cache_buffer<input_buffer_type_tagged>;
using cache_buffer_shared = std::shared_ptr<cache_buffer_t>;

#endif // cache_buffer_h__

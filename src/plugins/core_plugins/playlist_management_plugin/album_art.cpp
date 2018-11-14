
#include "album_art.h"

namespace mprt
{
	bool album_art::contains(album_name_t const& album_name)
	{
		return (_album_art_container.find(album_name) != _album_art_container.cend());
	}

	bool album_art::contains_increase(album_name_t const& album_name)
	{
		auto iter = _album_art_container.find(album_name);

		if (iter == _album_art_container.end())
		{
			return false;
		}
		
		++(iter->second._usage_count);
		return true;
	}

	album_art_list const& album_art::get_album_art(album_name_t const& album_name)
	{
		auto iter = _album_art_container.find(album_name);

		if (iter == _album_art_container.end())
		{
			return _empty_album_art;
		}

		return iter->second._album_art_contents;
	}

	void album_art::add_album_art(album_name_t const& album_name, album_art_filename_t album_art_filename, album_art_shared_contents_t album_art_conts)
	{
		_album_art_container[album_name]._album_art_contents[album_art_filename] = album_art_conts;
	}


	void album_art::remove_album_art(album_name_t const& album_name, album_art_filename_t album_art_filename)
	{
		auto iter = _album_art_container.find(album_name);
		if (iter != _album_art_container.end())
		{
			auto art_iter = iter->second._album_art_contents.find(album_art_filename);
			if (art_iter != iter->second._album_art_contents.end())
			{
				iter->second._album_art_contents.erase(art_iter);
			}
		}
	}

	void album_art::increase_usage(album_name_t const& album_name)
	{
		auto iter = _album_art_container.find(album_name);

		if (iter != _album_art_container.end())
		{
			++(iter->second._usage_count);
		}
	}

	void album_art::decrease_usage(album_name_t const& album_name)
	{
		auto iter = _album_art_container.find(album_name);

		if (iter != _album_art_container.end())
		{
			--(iter->second._usage_count);

			if (0 == iter->second._usage_count)
			{
				_album_art_container.erase(album_name);
			}
		}
	}

	void album_art::set_album_art(album_name_t const& album_name, album_art_list album_art_list_dets)
	{
		_album_art_container[album_name]._album_art_contents = album_art_list_dets;
	}

};

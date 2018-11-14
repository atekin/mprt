#ifndef album_art_h__
#define album_art_h__

#include <unordered_map>
#include <string>
#include <vector>

#include "album_art_details.h"

namespace mprt
{
	class album_art
	{
	private:
		std::unordered_map<album_name_t, album_art_details, album_name_hasher> _album_art_container;
		album_art_list _empty_album_art;

	public:
		bool contains(album_name_t const& album_name);
		bool contains_increase(album_name_t const& album_name);
		album_art_list const& get_album_art(album_name_t const& album_name);
		void add_album_art(album_name_t const& album_name, album_art_filename_t album_art_filename, album_art_shared_contents_t album_art_conts);
		void remove_album_art(album_name_t const& album_name, album_art_filename_t album_art_filename);
		void increase_usage(album_name_t const& album_name);
		void decrease_usage(album_name_t const& album_name);
		void set_album_art(album_name_t const& album_name, album_art_list album_art_list_dets);
	};

};

#endif // album_art_h__

#ifndef playlist_item_h__
#define playlist_item_h__

#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <memory>

#include "common/common_defs.h"
#include "common/type_defs.h"

#include "tag_parser_abstract.h"

namespace mprt
{
	using playlist_item_id_t = size_type;
	using playlist_id_t = size_type;

	struct playlist_item_t
	{
		using tag_name = std::string;
		using tag_value = std::string;
		using albumart_byte_t = unsigned char;

		playlist_item_id_t _playlist_item_id;
		url_id_t _url_id;
		std::string _url;
		std::chrono::microseconds _duration;
		tag_parser_abstract::tag_text_cnt_shr _tags;
		album_art_list _albumart;

		playlist_item_t()
			: _playlist_item_id(_INVALID_PLAYLIST_ITEM_ID_)
			, _url_id(_INVALID_URL_ID_)
			, _url("")
			, _duration(0)
		{}

		/*bool operator<(const playlist_item& play_item) const
		{
		return (_playlist_item_id < play_item._playlist_item_id);
		}*/
	};

	using playlist_item_shr_t = std::shared_ptr<playlist_item_t>;
	using playlist_item_shr_list_t = std::vector<playlist_item_shr_t>;
}

#endif // playlist_item_h__

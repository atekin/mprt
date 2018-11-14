#ifndef tag_parser_abstract_h__
#define tag_parser_abstract_h__

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>

#include <boost/algorithm/string.hpp>

#include "core/config.h"
#include "album_art_details.h"

namespace mprt
{
	constexpr static std::string_view _AUDIO_PROP_DURATION_MS_ = "Duration(ms)";
	constexpr static std::string_view _AUDIO_PROP_BITRATE_ = "Bit Rate(k/s)";
	constexpr static std::string_view _AUDIO_PROP_SAMPLERATE_ = "Sample Rate(Hz)";
	constexpr static std::string_view _AUDIO_PROP_CHANNELS_ = "Channels";

	constexpr static std::string_view _ALBUM_TAG_NAME = "Album";
	constexpr static std::string_view _ARTIST_TAG_NAME = "Artist";
	constexpr static std::string_view _ALBUM_ARTIST_TAG_NAME = "Album Artist";

	class album_art;

	class tag_parser_abstract
	{
	public:
		using byte_seq = unsigned char;
		using str_t = std::string;

		using tag_to_column = std::unordered_map<str_t, str_t>;
		using tag_text_cnt = std::unordered_map<str_t, str_t>;
		using tag_audio_props_cnt = std::unordered_map<str_t, str_t>;
		using tag_text_cnt_shr = std::shared_ptr<tag_text_cnt>;
		using tag_audio_props_cnt_shr = std::shared_ptr<tag_text_cnt>;

		//using album_art_pict = std::vector<byte_seq>;
		//using album_art_pict_ptr = std::shared_ptr<album_art_pict>;
		//using albumart_cnt = std::unordered_map<str_t/*tag_name*/, std::pair<str_t/*filename*/, album_art_pict_ptr>>;
		//using albumart_cnt_shr = std::shared_ptr<albumart_cnt>;

		tag_parser_abstract()
			: tag_parser_abstract("")
		{

		}
		
		tag_parser_abstract(std::string filename)
			: _filename(filename)
			, _tag_text_cnt_shr(std::make_shared<tag_text_cnt>())
			//, _albumart_cnt_shr(std::make_shared<albumart_cnt>())
			, _seper("\\/\\/")
		{
			init_tag_col();
		}

		virtual ~tag_parser_abstract() = default;

		void set_filename(std::string filename)
		{
			_filename = filename;
		}

		virtual bool read_metadata() = 0;
		virtual bool save_metadata() = 0;
		virtual bool save_album_art_metadata() = 0;

		tag_text_cnt_shr get_tags() { return _tag_text_cnt_shr; }
		tag_audio_props_cnt_shr get_audio_props() { return _tag_audio_props_cnt_shr; }
		virtual album_art_list get_album_arts() = 0;
		
		void save_tags(tag_text_cnt_shr tags, bool save_file = false)
		{
			_tag_text_cnt_shr = tags;
			if (save_file) save_metadata();
		}

		virtual void save_album_arts(album_art_list album_arts, album_name_t const& alb_name, bool save_file = false) = 0;

		void clear()
		{
			_filename = "";
			_tag_text_cnt_shr = (std::make_shared<tag_text_cnt>());
			_tag_audio_props_cnt_shr = (std::make_shared<tag_audio_props_cnt>());
		}

		std::string get_tag_col(std::string const& native_tag)
		{
			auto nat_tag_cap = boost::to_upper_copy<std::string>(native_tag);
			//auto nat_tag_cap =(native_tag);
			auto tag_col_iter = _tag_to_column.find(nat_tag_cap);
			if (tag_col_iter != _tag_to_column.end())
			{
				return tag_col_iter->second;
			}

			// we do not know the tag yet ...
			return native_tag;
		}

		void set_tag_val(std::string const& tag_name, std::string const& tag_val)
		{
			(*_tag_text_cnt_shr)[get_tag_col(tag_name)] = tag_val;

			//auto old_tag_val = get_tag_val(mapped_tag);
			//if (old_tag_val.empty())
			//	(*_tag_text_cnt_shr)[mapped_tag] = old_tag_val;
			//else
			//	(*_tag_text_cnt_shr)[mapped_tag] = old_tag_val + " " + tag_val;
		}

		std::string get_tag_val(std::string const& tag_name)
		{
			auto tag_val_iter = _tag_text_cnt_shr->find(get_tag_col(tag_name));
			if (tag_val_iter != _tag_text_cnt_shr->end())
			{
				return tag_val_iter->second;
			}

			return std::string();
		}

		album_name_t get_album_concat_name()
		{
			album_name_t alb_name;

			alb_name._artist = get_tag_val(std::string(_ARTIST_TAG_NAME));
			alb_name._album_artist = get_tag_val(std::string(_ALBUM_ARTIST_TAG_NAME));
			alb_name._album = get_tag_val(std::string(_ALBUM_TAG_NAME));
			
			return alb_name;
		}

		void init_tag_col()
		{
			config::instance().init("../config/config_tag_parser_plugin.xml");
			auto tree = config::instance().get_ptree_node("mprt");

			for (auto & tag_to_col : tree)
			{
				auto col_tag = tag_to_col.second.get_child("col");
				auto native_tags = tag_to_col.second.get_child("tags");
				for (auto & native_tag : native_tags)
				{
					_tag_to_column[boost::to_upper_copy<std::string>(native_tag.second.data())] = col_tag.data();
				}
			}
		}

		void set_album_art(std::shared_ptr<album_art> alb_art)
		{
			_album_art = alb_art;
		}

	protected:
		std::string _filename;
		tag_text_cnt_shr _tag_text_cnt_shr;
		tag_audio_props_cnt_shr _tag_audio_props_cnt_shr;
		std::string _seper;
		tag_to_column _tag_to_column;
		std::shared_ptr<album_art> _album_art;
	};
}


#endif // tag_parser_abstract_h__

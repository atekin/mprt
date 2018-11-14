#ifndef tag_lib_parser_h__
#define tag_lib_parser_h__

#include "tag_parser_abstract.h"

namespace TagLib
{
	class StringList;
	template <class T> class List;

	namespace APE
	{
		class Tag;
	}

	namespace ASF
	{
		class Tag;
	}

	namespace ID3v2
	{
		class Tag;
	}

	namespace Ogg
	{
		class XiphComment;
	}

	namespace MP4
	{
		class Tag;
	}

	namespace FLAC
	{
		class Picture;
	}
}

namespace mprt
{
	class tag_lib_parser : public tag_parser_abstract
	{
	public:

		tag_lib_parser();
		tag_lib_parser(std::string filename);
		virtual ~tag_lib_parser();
		
		void init();
		virtual bool read_metadata() override;
		virtual bool save_metadata() override;
		virtual bool save_album_art_metadata() override;

		virtual album_art_list get_album_arts() override;
		virtual void save_album_arts(album_art_list album_arts, album_name_t const& alb_name, bool save_file = false) override;


	private:
		bool read_meta();
		bool save_meta();
		bool save_album_art_meta();

		std::pair<str_t, str_t> get_couple_numbers(std::string const& str_to_split);
		std::string get_ext_from_mime(std::string const& mime_type);
		bool read_ape(TagLib::APE::Tag * ape_tag, bool read_pict, bool override_album_art_container = false);
		bool read_asf(TagLib::ASF::Tag * asf_tag, bool read_pict, bool override_album_art_container = false);
		bool read_ID3v2(TagLib::ID3v2::Tag* id3v2_tag, bool read_pict, bool override_album_art_container = false);
		bool read_xiph(TagLib::Ogg::XiphComment* xiph_tag, bool read_pict, bool override_album_art_container = false);
		bool read_mp4(TagLib::MP4::Tag* mp4_tag, bool read_pict, bool override_album_art_container = false);
		
		bool get_xiph_picts(TagLib::StringList const& mime_list, TagLib::StringList const& art_list);
		void convert_base_64(album_art_shared_contents_t album_art, const char* src);
		bool parse_flac_pict(album_art_shared_contents_t album_art);
		bool read_flac_pictures(TagLib::List<TagLib::FLAC::Picture *> const& pict_list);

		void set_tag_str_list(std::string const& tag_name, TagLib::StringList const& tag_str_list);
	};
}


#endif // tag_lib_parser_h__

#include <ctype.h>

#include <boost/log/trivial.hpp>
#include <boost/endian/conversion.hpp>

#include "common/utils.h"

#include "tag_lib_parser.h"

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/apefile.h>
#include <taglib/asffile.h>
#include <taglib/apetag.h>
#include <taglib/flacfile.h>
#include <taglib/mpcfile.h>
#include <taglib/mpegfile.h>
#include <taglib/mp4file.h>
#include <taglib/oggfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/opusfile.h>
#include <taglib/aifffile.h>
#include <taglib/wavfile.h>
#include <taglib/speexfile.h>
#include <taglib/trueaudiofile.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavpackfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/textidentificationframe.h>
#include <taglib/uniquefileidentifierframe.h>

#include "album_art.h"

namespace mprt
{
#ifdef _WIN32
	static inline std::wstring to_wide(const char *utf8)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
		if (len == 0)
			return NULL;

		std::wstring out(len * sizeof(wchar_t), '0');

		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &out[0], len);
		return out;
	}
#endif

	template <class T>
	class ext_resolver : public TagLib::FileRef::FileTypeResolver
	{
	public:
		ext_resolver(const std::string &ext) {
			_ext = ext;
			std::transform(_ext.begin(), _ext.end(), _ext.begin(), ::toupper);
		}

		~ext_resolver() {}

		virtual TagLib::File *createFile(TagLib::FileName filename, bool, TagLib::AudioProperties::ReadStyle) const
		{
#ifdef _WIN32
			std::string filename_ = filename.toString().to8Bit(true);
#else
			std::string filename_ = filename;
#endif
			std::size_t namesize = filename_.size();

			if (namesize > _ext.length())
			{
				std::string fext = filename_.substr(namesize - _ext.length(), _ext.length());
				std::transform(fext.begin(), fext.end(), fext.begin(), ::toupper);
				if (fext == _ext)
					return new T(filename, false, TagLib::AudioProperties::Accurate);
			}

			return 0;

		}

	protected:
		std::string _ext;
	};

	static ext_resolver<TagLib::MPEG::File> aacresolver(".aac");
	static ext_resolver<TagLib::MPEG::File> ac3resolver(".ac3");

	tag_lib_parser::tag_lib_parser()
		: tag_parser_abstract()
	{
		init();
	}

	tag_lib_parser::tag_lib_parser(std::string filename)
		: tag_parser_abstract(filename)
	{
		init();
	}

	tag_lib_parser::~tag_lib_parser()
	{
	}

	void tag_lib_parser::init()
	{
		TagLib::FileRef::addFileTypeResolver(&aacresolver);
		TagLib::FileRef::addFileTypeResolver(&ac3resolver);
	}

	bool tag_lib_parser::read_metadata()
	{
		return read_meta();
	}

	bool tag_lib_parser::save_metadata()
	{
		return save_meta();
	}

	bool tag_lib_parser::save_album_art_metadata()
	{
		return save_album_art_meta();
	}

	album_art_list tag_lib_parser::get_album_arts()
	{
		return _album_art->get_album_art(get_album_concat_name());
	}

	void tag_lib_parser::save_album_arts(album_art_list album_arts, album_name_t const& alb_name, bool save_file /*= false*/)
	{
		_album_art->set_album_art(alb_name, album_arts);
		if (save_file) save_album_art_metadata();
	}

	bool tag_lib_parser::read_meta()
	{
#ifdef _WIN32
		auto wchar_file_name = to_wide(_filename.c_str());
		TagLib::FileRef f(&wchar_file_name[0], true, TagLib::AudioProperties::Accurate);
#else
		TagLib::FileRef f(_filename.c_str());
#endif
		if (f.isNull())
			return false;

		// @TODO: we can do better with respect to file later ...
		auto audio_pros = f.audioProperties();
		if (audio_pros)
		{
			set_tag_val(std::string(_AUDIO_PROP_DURATION_MS_), std::to_string(audio_pros->lengthInMilliseconds()));
			set_tag_val(std::string(_AUDIO_PROP_BITRATE_), std::to_string(audio_pros->bitrate()));
			set_tag_val(std::string(_AUDIO_PROP_SAMPLERATE_), std::to_string(audio_pros->sampleRate()));
			set_tag_val(std::string(_AUDIO_PROP_CHANNELS_), std::to_string(audio_pros->channels()));
		}

		if (!f.tag() || f.tag()->isEmpty())
		{
			set_tag_val("TITLE", _filename);
			return true;
		}

		TagLib::PropertyMap tags = f.file()->properties();
		for (TagLib::PropertyMap::ConstIterator tag_name = tags.begin(); tag_name != tags.end(); ++tag_name)
		{
			set_tag_str_list(std::string(tag_name->first.toCString(true)), tag_name->second);
		}


		bool may_read_pict(true);

		if (TagLib::APE::File* ape = dynamic_cast<TagLib::APE::File*>(f.file()))
		{
			if (ape->APETag())
			{
				read_ape(ape->APETag(), may_read_pict);
			}
		}
		else if (TagLib::ASF::File* asf = dynamic_cast<TagLib::ASF::File*>(f.file()))
		{
			if (asf->tag())
				read_asf(asf->tag(), may_read_pict);
		}
		else if (TagLib::FLAC::File* flac_file = dynamic_cast<TagLib::FLAC::File*>(f.file()))
		{
			if (flac_file->ID3v2Tag())
				may_read_pict = !read_ID3v2(flac_file->ID3v2Tag(), may_read_pict);
			
			if (flac_file->xiphComment())
				may_read_pict = !read_xiph(flac_file->xiphComment(), may_read_pict, !may_read_pict && flac_file->ID3v2Tag());

			if (may_read_pict && !_album_art->contains(get_album_concat_name()))
			{
				read_flac_pictures(flac_file->pictureList());
			}
		}
		else if (TagLib::MP4::File *mp4 = dynamic_cast<TagLib::MP4::File*>(f.file()))
		{
			if (mp4->tag())
				read_mp4(mp4->tag(), may_read_pict);
		}
		else if (TagLib::MPC::File* mpc = dynamic_cast<TagLib::MPC::File*>(f.file()))
		{
			if (mpc->APETag())
				read_ape(mpc->APETag(), may_read_pict);
		}
		else if (TagLib::MPEG::File* mpeg = dynamic_cast<TagLib::MPEG::File*>(f.file()))
		{
			if (mpeg->APETag())
				read_ape(mpeg->APETag(), may_read_pict);
			
			if (mpeg->ID3v2Tag())
				read_ID3v2(mpeg->ID3v2Tag(), may_read_pict);
		}
		else if (dynamic_cast<TagLib::Ogg::File*>(f.file()))
		{
			if (TagLib::Ogg::FLAC::File* ogg_flac = dynamic_cast<TagLib::Ogg::FLAC::File*>(f.file()))
				read_xiph(ogg_flac->tag(), may_read_pict);
			else if (TagLib::Ogg::Speex::File* ogg_speex = dynamic_cast<TagLib::Ogg::Speex::File*>(f.file()))
				read_xiph(ogg_speex->tag(), may_read_pict);
			else if (TagLib::Ogg::Vorbis::File* ogg_vorbis = dynamic_cast<TagLib::Ogg::Vorbis::File*>(f.file()))
				read_xiph(ogg_vorbis->tag(), may_read_pict);
#if defined(TAGLIB_OPUSFILE_H)
			else if (TagLib::Ogg::Opus::File* ogg_opus = dynamic_cast<TagLib::Ogg::Opus::File*>(f.file()))
				read_xiph(ogg_opus->tag(), may_read_pict);
#endif
		}
		else if (dynamic_cast<TagLib::RIFF::File*>(f.file()))
		{
			if (TagLib::RIFF::AIFF::File* riff_aiff = dynamic_cast<TagLib::RIFF::AIFF::File*>(f.file()))
				read_ID3v2(riff_aiff->tag(), may_read_pict);
			else if (TagLib::RIFF::WAV::File* riff_wav = dynamic_cast<TagLib::RIFF::WAV::File*>(f.file()))
				read_ID3v2(riff_wav->tag(), may_read_pict);
		}
		else if (TagLib::TrueAudio::File* trueaudio = dynamic_cast<TagLib::TrueAudio::File*>(f.file()))
		{
			if (trueaudio->ID3v2Tag())
				read_ID3v2(trueaudio->ID3v2Tag(), may_read_pict);
		}
		else if (TagLib::WavPack::File* wavpack = dynamic_cast<TagLib::WavPack::File*>(f.file()))
		{
			if (wavpack->APETag())
				read_ape(wavpack->APETag(), may_read_pict);
		}
		return true;
	}

	bool tag_lib_parser::save_meta()
	{
		return false;
	}

	bool tag_lib_parser::save_album_art_meta()
	{
		return false;
	}

	std::pair<mprt::tag_parser_abstract::str_t, mprt::tag_parser_abstract::str_t> tag_lib_parser::get_couple_numbers(std::string const& str_to_split)
	{
		auto found = str_to_split.find_first_of('/');
		if (found != std::string::npos)
		{
			return std::make_pair(str_to_split.substr(0, found), str_to_split.substr(found + 1));
		}

		return std::make_pair(str_to_split, "");
	}

	std::string tag_lib_parser::get_ext_from_mime(std::string const& mime_type)
	{
		return get_ext(mime_type, std::string("/"));
	}

	bool tag_lib_parser::read_ape(TagLib::APE::Tag * ape_tag, bool read_pict, bool override_album_art_container)
	{
		TagLib::APE::ItemListMap fields(ape_tag->itemListMap());
		auto iter = fields.begin(), iter_end = fields.end();
		std::vector<decltype(iter)> pict_iters;

		// read all the tags
		for (; iter != iter_end; ++iter)
		{
			if (iter->second.isEmpty())
			{
				continue;
			}

			// save picture tags for later
			if (iter->second.type() == TagLib::APE::Item::Binary && read_pict)
			{
				pict_iters.push_back(iter);
			}
			else
			{
				set_tag_val(iter->first.toCString(true), iter->second.toString().toCString(true));
			}
		}

		bool if_read_pict(false);
		if (read_pict)
		{
			auto album_concat_name = get_album_concat_name();
			if_read_pict = !_album_art->contains(album_concat_name) || override_album_art_container;
			if (if_read_pict)
			{
				for (auto & iter_ref : pict_iters)
				{
					auto pict = iter_ref->second.binaryData();

					size_t desc_len = strnlen(pict.data(), pict.size());
					if (desc_len < pict.size())
					{
						auto album_art(std::make_shared<album_art_contents_t>());
						album_art->assign(pict.data() + desc_len + 1, pict.data() + pict.size());
						str_t filename(pict.data(), desc_len);
						//auto tag_name = get_tag_col(iter_ref->first.toCString(true));
						//(*_albumart_cnt_shr)[tag_name] = std::make_pair(filename, album_art);
						_album_art->add_album_art(album_concat_name, filename, album_art);
					}
				}
			}
		}


		return if_read_pict;
	}


	bool tag_lib_parser::read_asf(TagLib::ASF::Tag * asf_tag, bool read_pict, bool override_album_art_container)
	{

		auto
			attr_l_map_iter = asf_tag->attributeListMap().begin(),
			attr_l_map_iter_end = asf_tag->attributeListMap().end();

		TagLib::ASF::AttributeList::Iterator iter, iter_end;
		std::vector<decltype(iter)> pict_iters;


		for (; attr_l_map_iter != attr_l_map_iter_end; ++attr_l_map_iter)
		{
			std::string tag_name(attr_l_map_iter->first.toCString(true));
			TagLib::ASF::AttributeList list = attr_l_map_iter->second;
			iter = list.begin(), iter_end = list.end();

			if (tag_name != "WM/Picture")
			{
				TagLib::StringList tag_vals;

				for (; iter != iter_end; ++iter)
				{
					tag_vals.append(iter->toString());
				}

				set_tag_str_list(tag_name, tag_vals);
			}
			else if (read_pict)
			{
				for (; iter != iter_end; ++iter)
				{
					pict_iters.push_back(iter);
				}
			}
		}

		bool if_read_pict(false);
		if (read_pict)
		{
			auto album_concat_name = get_album_concat_name();
			if_read_pict = !_album_art->contains(album_concat_name) || override_album_art_container;
			if (if_read_pict)
			{
				str_t filename;
				int i = 0;
				for (auto & iter_ref : pict_iters)
				{
					// @TODO: will be corrected according to https://msdn.microsoft.com/en-us/library/windows/desktop/dd757977(v=vs.85).aspx
					auto suffix_num = std::to_string(i);
					const TagLib::ASF::Picture asfPicture = iter->toPicture();
					auto picture_data = asfPicture.picture();

					auto album_art(std::make_shared<album_art_contents_t>());
					album_art->assign(picture_data.data(), picture_data.data() + picture_data.size());
					//asfPicture.type()
					if (asfPicture.description().size() > 0)
					{
						filename = asfPicture.description().toCString(true);
					}
					else
					{
						auto file_ext = get_ext_from_mime(asfPicture.mimeType().toCString());

						filename = "pict" + suffix_num + "." + file_ext;
					}
					//tag_name = get_tag_col("WM/Picture");

					//(*_albumart_cnt_shr)[tag_name] = std::make_pair(filename, album_art);
					_album_art->add_album_art(get_album_concat_name(), filename, album_art);
				}
			}
		}

		return if_read_pict;
	}

	bool tag_lib_parser::read_ID3v2(TagLib::ID3v2::Tag* id3v2_tag, bool read_pict, bool override_album_art_container)
	{
		auto
			frame_list_map_iter = id3v2_tag->frameListMap().begin(),
			frame_list_map_iter_end = id3v2_tag->frameListMap().end();

		std::vector<TagLib::ID3v2::AttachedPictureFrame*> p_apiclist;

		for (; frame_list_map_iter != frame_list_map_iter_end; ++frame_list_map_iter)
		{
			TagLib::ID3v2::FrameList list = frame_list_map_iter->second;
			if (list.isEmpty())
			{
				continue;
			}

			auto list_iter = list.begin(), list_iter_end = list.end();

			if (frame_list_map_iter->first == "UFID")
			{
				for (; list_iter != list_iter_end; ++list_iter)
				{
					TagLib::ID3v2::UniqueFileIdentifierFrame* ufid =
						dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*list_iter);
					if (!ufid)
						continue;
					const char *owner = ufid->owner().toCString();
					if (!strcmp(owner, "http://musicbrainz.org"))
					{
						int max_size = std::min(ufid->identifier().size(), (unsigned int)63);
						std::string tag_val((char*)ufid->identifier().data(), max_size);
						set_tag_val("UFID", tag_val);
					}
				}

			}
			else if (frame_list_map_iter->first == "TXXX")
			{
				for (; list_iter != list_iter_end; ++list_iter)
				{
					TagLib::ID3v2::UserTextIdentificationFrame* p_txxx =
						dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*list_iter);
					if (!p_txxx)
						continue;
					set_tag_val(p_txxx->description().toCString(true), p_txxx->fieldList().back().toCString(true));
				}
			}
			else if (frame_list_map_iter->first == "TRCK")
			{
				auto pair_vals = get_couple_numbers((*list_iter)->toString().toCString(true));
				set_tag_val("Track", pair_vals.first);
				set_tag_val("TRACKTOTAL", pair_vals.second);
			}
			else if (frame_list_map_iter->first == "TPOS")
			{
				auto pair_vals = get_couple_numbers((*list_iter)->toString().toCString(true));
				set_tag_val("DisckNo", pair_vals.first);
				set_tag_val("DiscTotal", pair_vals.first);
			}
			else if (frame_list_map_iter->first == "APIC" && read_pict)
			{
				for (; list_iter != list_iter_end; ++list_iter)
				{
					TagLib::ID3v2::AttachedPictureFrame* p_apic =
						dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*list_iter);
					if (!p_apic)
						continue;

					p_apiclist.push_back(p_apic);
				}
			}
			else
			{
				for (; list_iter != list_iter_end; ++list_iter)
				{
					std::string tag_name(frame_list_map_iter->first.data(), frame_list_map_iter->first.size());
					set_tag_val(tag_name, (*list_iter)->toString().toCString(true));
				}
			}
		}

		bool if_read_pict(false);
		if (read_pict)
		{
			auto album_concat_name = get_album_concat_name();
			if_read_pict = !_album_art->contains(album_concat_name) || override_album_art_container;
			if (if_read_pict)
			{
				int suffix_num = 0;
				for (auto & p_apicref : p_apiclist)
				{
					std::string mime_type(p_apicref->mimeType().toCString(true));
					std::string descr(p_apicref->description().toCString(true));

					if (mime_type == "PNG" || descr == "\xC2\x89PNG")
					{
						BOOST_LOG_TRIVIAL(debug) << "Invalid picture embedded";
						continue;
					}

					std::string filename;
					auto picture_data = p_apicref->picture();
					auto album_art(std::make_shared<album_art_contents_t>());
					album_art->assign(picture_data.data(), picture_data.data() + picture_data.size());
					if (descr.size() > 0)
					{
						filename = descr;
					}
					else
					{

						filename = "pict" + std::to_string(suffix_num);
					}
					auto file_ext = get_ext_from_mime(mime_type);
					filename += "." + file_ext;

					int pict_type = p_apicref->type();
					if (pict_type < 0 || pict_type > 0x14)
						pict_type = 0;
					//auto tag_name = get_tag_col("PICT_" + std::to_string(pict_type) + "_ID3V2");

					//(*_albumart_cnt_shr)[tag_name] = std::make_pair(filename, album_art);
					_album_art->add_album_art(album_concat_name, filename, album_art);

					++suffix_num;
				}
			}
		}

		return if_read_pict;
	}

	bool tag_lib_parser::read_xiph(TagLib::Ogg::XiphComment* xiph_tag, bool read_pict, bool override_album_art_container)
	{
		auto field_list = xiph_tag->fieldListMap();
		
		auto
			tag_iter = field_list.begin(),
			tag_iter_end = field_list.end();

		TagLib::StringList mime_list;
		TagLib::StringList art_list;

		for (; tag_iter != tag_iter_end; ++tag_iter)
		{
			std::string tag_name(tag_iter->first.toCString(true));
			TagLib::StringList str_list = tag_iter->second;

			if (str_list.isEmpty())
			{
				continue;
			}

			auto
				str_iter = str_list.begin(),
				str_iter_end = str_list.end();

			if (tag_name == "TRACKNUMBER")
			{
				auto pair_nums = get_couple_numbers(std::string(str_iter->toCString(true)));
				set_tag_val("TRACKNUMBER", pair_nums.first);
				if (!pair_nums.second.empty())
				{
					set_tag_val("TRACKTOTAL", pair_nums.first);
				}
			}
			else if (tag_name == "COVERARTMIME")
			{
				mime_list = str_list;
			}
			else if (tag_name == "COVERART")
			{
				art_list = str_list;
			}
			else if (tag_name == "METADATA_BLOCK_PICTURE")
			{
				art_list = str_list;
				for (unsigned idx = 0; idx != art_list.size(); ++idx)
				{
					auto album_art(std::make_shared<album_art_contents_t>(art_list[idx].size()));
					convert_base_64(album_art, art_list[idx].toCString(true));
					parse_flac_pict(album_art);
				}
			}
			else
			{
				set_tag_str_list(tag_name, str_list);
			}
		}

		bool if_read_pict(false);
		if (read_pict)
		{
			auto album_concat_name = get_album_concat_name();
			if_read_pict = !_album_art->contains(album_concat_name) || override_album_art_container;
			if (if_read_pict)
			{

				if (!mime_list.isEmpty() && !art_list.isEmpty())
				{
					if_read_pict = get_xiph_picts(mime_list, art_list);
				}
				else
				{
					if_read_pict = false;
				}
				
				if_read_pict = if_read_pict || read_flac_pictures(xiph_tag->pictureList());
			}
		}

		return if_read_pict;
	}

	bool tag_lib_parser::get_xiph_picts(TagLib::StringList const& mime_list, TagLib::StringList const& art_list)
	{
		auto min_loop_size = std::min(mime_list.size(), art_list.size());

		for (decltype(min_loop_size) idx = 0; idx != min_loop_size; ++idx)
		{
			auto idx_str = std::to_string(idx);
			std::string tag_name = "PICT_" + std::to_string(idx) + "_XIPH";
			tag_name = get_tag_col(tag_name);
			auto filename = "cover_" + idx_str + "." + get_ext_from_mime(std::string(mime_list[idx].toCString(true)));
		
			auto album_art(std::make_shared<album_art_contents_t>(art_list[idx].size()));
			convert_base_64(album_art, art_list[idx].toCString(true));
			//(*_albumart_cnt_shr)[tag_name] = std::make_pair(filename, album_art);
			_album_art->add_album_art(get_album_concat_name(), filename, album_art);
		}

		return min_loop_size > 0;
	}

	void tag_lib_parser::convert_base_64(album_art_shared_contents_t album_art, const char* src)
	{
		static const int b64[256] = {
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
			52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
			-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
			15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
			-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
			41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
		};
		uint8_t *p_start = album_art->data();
		uint8_t *p_dst = album_art->data();
		uint8_t *p = (uint8_t *)src;

		int i_level;
		int i_last;
		int loop_idx;

		for (i_level = 0, i_last = 0, loop_idx = 0; loop_idx != album_art->size() && *p != '\0'; p++, ++loop_idx)
		{
			const int c = b64[(unsigned int)*p];
			if (c == -1)
				break;

			switch (i_level)
			{
			case 0:
				i_level++;
				break;
			case 1:
				*p_dst++ = (i_last << 2) | ((c >> 4) & 0x03);
				i_level++;
				break;
			case 2:
				*p_dst++ = ((i_last << 4) & 0xf0) | ((c >> 2) & 0x0f);
				i_level++;
				break;
			case 3:
				*p_dst++ = ((i_last & 0x03) << 6) | c;
				i_level = 0;
			}
			i_last = c;
		}

		album_art->resize(p_dst - album_art->data());
	}

	bool tag_lib_parser::parse_flac_pict(album_art_shared_contents_t album_art)
	{
		uint32_t type, len;
		size_t size = album_art->size();

		unsigned char * p_data = album_art->data();

		if (size < 8)
			return false;

#define RM(x) \
    do { \
        assert(size >= (x)); \
        size -= (x); \
        p_data += (x); \
    } while (0)

		type = *p_data;
		boost::endian::big_to_native_inplace(type);
		RM(4);
		len = *p_data;
		boost::endian::big_to_native_inplace(len);
		RM(4);

		if (size < len)
			return false;

		std::string mime_text((char*)p_data, len);
		RM(len);

		if (size < 4)
			return false;

		len = *p_data;
		boost::endian::big_to_native_inplace(len);
		RM(4);

		if (size < len)
			return false;

		std::string desc((const char *)p_data, len);
		RM(len);

		if (size < 20)
			return false;

		RM(4 * 4); /* skip */

		len = *p_data;
		boost::endian::big_to_native_inplace(len);
		RM(4);

#undef RM

		if (size < len)
			return false;

		if (mime_text == "image/jpeg")
			desc += ".jpg";
		else if (mime_text == "image/png")
			desc += ".png";

		std::string tag_name = "PICT_" + std::to_string(type) + "_ID3V2";
		tag_name = get_tag_col(tag_name);
		auto filename = desc;

		auto diff = p_data - album_art->data();
		std::vector<decltype(album_art)::element_type::value_type>(album_art->begin() + diff, album_art->end()).swap(*album_art);
		//(*_albumart_cnt_shr)[tag_name] = std::make_pair(filename, album_art);
		_album_art->add_album_art(get_album_concat_name(), filename, album_art);


		return true;
	}

	bool tag_lib_parser::read_mp4(TagLib::MP4::Tag* mp4_tag, bool read_pict, bool override_album_art_container)
	{
		auto item_list_map = mp4_tag->itemListMap();
		auto
			item_list_map_iter = item_list_map.begin(),
			item_list_map_iter_end = item_list_map.end();

		for (; item_list_map_iter != item_list_map_iter_end; ++item_list_map_iter)
		{
			std::string tag_name(item_list_map_iter->first.toCString(true));
			TagLib::MP4::Item item_list = item_list_map_iter->second;

			if (tag_name == "covr")
			{
				continue;
			}
			else
			{
				set_tag_str_list(tag_name, item_list.toStringList());
			}
		}

		bool if_read_pict(false);
		if (read_pict)
		{
			auto album_concat_name = get_album_concat_name();
			if_read_pict = !_album_art->contains(album_concat_name) || override_album_art_container;
			if (if_read_pict)
			{
				auto pict_tag = mp4_tag->itemListMap().find("covr");

				if (pict_tag != mp4_tag->itemListMap().end())
				{
					TagLib::MP4::CoverArtList cover_art_list = pict_tag->second.toCoverArtList();
					auto
						cover_art_iter = cover_art_list.begin(),
						cover_art_iter_end = cover_art_list.end();

					for (int cover_id = 0; cover_art_iter != cover_art_iter_end; ++cover_art_iter, ++cover_id)
					{
						std::string ext;

						switch (cover_art_iter->format())
						{
						case TagLib::MP4::CoverArt::JPEG:
							ext = ".jpg";
							break;

						case TagLib::MP4::CoverArt::PNG:
							ext = ".png";
							break;

						case TagLib::MP4::CoverArt::BMP:
							ext = ".bmp";
							break;

						case TagLib::MP4::CoverArt::GIF:
							ext = ".gif";
							break;

						case TagLib::MP4::CoverArt::Unknown:
						default:
							ext = ".jpg";
							break;
						}

						auto cover_id_str = std::to_string(cover_id);
						//tag_name = get_tag_col(tag_name + "_" + cover_id_str);
						auto album_art(std::make_shared<album_art_contents_t>());
						auto cover_data = cover_art_iter->data();
						auto start_ptr = cover_data.data();
						auto end_ptr = start_ptr + cover_data.size();
						album_art->assign(start_ptr, end_ptr);
						str_t filename("cover_" + cover_id_str + ext);
						//(*_albumart_cnt_shr)[tag_name] = std::make_pair(filename, album_art);
						_album_art->add_album_art(get_album_concat_name(), filename, album_art);

						++cover_id;
					}
				}
				else
				{
					if_read_pict = false;
				}
			}
		}

		return if_read_pict;
	}

	bool tag_lib_parser::read_flac_pictures(TagLib::List<TagLib::FLAC::Picture *> const& pict_list)
	{
		auto pict_list_iter = pict_list.begin(),
			pict_list_iter_end = pict_list.end();
		auto is_pict_present = pict_list_iter != pict_list_iter_end;
		for (; pict_list_iter != pict_list_iter_end; ++pict_list_iter)
		{
			auto flac_pict = *pict_list_iter;
			auto type_str = std::to_string((int)flac_pict->type());
			auto mime_type = flac_pict->mimeType();

			auto file_ext = get_ext_from_mime(mime_type.toCString(true));
			auto filename = "PICT_" + type_str + "_FLAC." + file_ext;

			auto pict_data = flac_pict->data();
			auto pict_data_begin = pict_data.data();
			auto pict_data_end = pict_data_begin+ pict_data.size();

			auto album_art(std::make_shared<album_art_contents_t>(pict_data.size()));
			album_art->assign(pict_data_begin, pict_data_end);
			//(*_albumart_cnt_shr)[file_name] = std::make_pair(filename, album_art);
			_album_art->add_album_art(get_album_concat_name(), filename, album_art);
		}

		return is_pict_present;
	}

	void tag_lib_parser::set_tag_str_list(std::string const& tag_name, TagLib::StringList const& tag_str_list)
	{
		if (tag_str_list.isEmpty())
		{
			return;
		}

		TagLib::StringList::ConstIterator
			tag_val_iter = tag_str_list.begin(),
			tag_val_iter_end = tag_str_list.end();

		std::string tag_concat_val = tag_val_iter->toCString(true);
		++tag_val_iter;
		for (; tag_val_iter != tag_val_iter_end; ++tag_val_iter)
		{
			tag_concat_val += (_seper + tag_val_iter->toCString(true));
		}
		set_tag_val(tag_name, tag_concat_val);
	}

}


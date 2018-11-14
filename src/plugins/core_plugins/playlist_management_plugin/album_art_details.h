#ifndef album_art_details_h__
#define album_art_details_h__

#include <boost/functional/hash.hpp>

namespace mprt
{
	// @INFO: album name must contain somethings like ARTIST+ALBUM_ARTIST+ALBUM otherwise clashes can occur
	struct album_name_t
	{
		std::string _artist;
		std::string _album_artist;
		std::string _album;

		bool operator==(album_name_t const& other) const
		{
			return (_artist == other._artist
				&& _album_artist == other._album_artist
				&& _album == other._album);
		}
	};

	struct album_name_hasher
	{
		std::size_t operator()(album_name_t const& alb_name) const
		{
			using boost::hash_value;
			using boost::hash_combine;

			std::size_t seed = 0;

			hash_combine(seed, hash_value(alb_name._artist));
			hash_combine(seed, hash_value(alb_name._album_artist));
			hash_combine(seed, hash_value(alb_name._album));

			return seed;
		}
	};

	using album_art_filename_t = std::string;
	using album_art_content_t = unsigned char;
	using album_art_contents_t = std::vector<album_art_content_t>;
	using album_art_shared_contents_t = std::shared_ptr<album_art_contents_t>;
	using album_art_list = std::unordered_map<album_art_filename_t, album_art_shared_contents_t>;

	struct album_art_details
	{
		album_art_list _album_art_contents;
		int _usage_count;

		album_art_details()
			: _usage_count(0)
		{}
	};
}

#endif // album_art_details_h__

#ifndef playlist_h__
#define playlist_h__

#include <string>
#include <chrono>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include "common/common_defs.h"

#include "playlist_item.h"

namespace mprt
{
	class tag_parser_abstract;
	class album_art;

	struct by_playlist {};
	struct by_playlist_item_id {};
	struct by_playlist_url_id {};

	// define a multi_index_container with a list-like index
	using playlist_container = boost::multi_index::multi_index_container<
		playlist_item_shr_t,
		boost::multi_index::indexed_by<
			  boost::multi_index::sequenced<boost::multi_index::tag<by_playlist>> // list-like index for #0 default
			, boost::multi_index::hashed_unique<boost::multi_index::tag<by_playlist_item_id>, boost::multi_index::member<playlist_item_t, playlist_item_id_t, &playlist_item_t::_playlist_item_id>>
			, boost::multi_index::hashed_non_unique<boost::multi_index::tag<by_playlist_url_id>, boost::multi_index::member<playlist_item_t, url_id_t, &playlist_item_t::_url_id>>
		>
	>;

	using playlist_list_t = playlist_container::index<by_playlist>::type;
	using playlist_itemlist_t = playlist_container::index<by_playlist_item_id>::type;
	using playlist_urllist_t = playlist_container::index<by_playlist_url_id>::type;
	using playlist_const_iter_t = playlist_list_t::const_iterator;
	using playlist_iter_t = playlist_list_t::iterator;
	using playlist_itemlist_const_iter_t = playlist_itemlist_t::const_iterator;
	using playlist_itemlist_iter_t = playlist_itemlist_t::iterator;
	using playlist_urllist_const_iter_t = playlist_urllist_t::const_iterator;
	using playlist_urllist_iter_t = playlist_urllist_t::iterator;


	/*template<class Class, typename Type, Type Class::*PtrToMember>
	struct mem_type
	{
		Type & operator()(Class & item) {
			return item.*PtrToMember;
		}
	};

	using playlist_container = std::list<playlist_item>;*/

	class playlist
	{
	private:
		std::string _playlist_name;
		playlist_container _playlist;
		std::atomic<size_type> _playlist_id_counter;
		std::shared_ptr<tag_parser_abstract> _tag_parser;
		std::string _base_path;
		std::shared_ptr<album_art> _album_art;

		template<typename Tag, typename ItemType, ItemType playlist_item_t::*member_pointer, typename Iter, typename ItemValue>
		bool update_playlist_item(Iter & iter, ItemValue value)
		{
			/*try
			{
				mem_type<playlist_item, ItemType, member_pointer> play_item_mem;
				play_item_mem(*iter) = value;
				return true;
			}
			catch (...)
			{
				return false;
			}*/
			
			try
			{
				auto mod_lambda =
					[value] (playlist_item_shr_t & pl_item) {
					boost::multi_index::member<playlist_item_t, ItemType, member_pointer> play_item_mem;
					play_item_mem(*pl_item) = value;
				};

				return _playlist.get<Tag>().modify(iter, mod_lambda);
			}
			catch (...)
			{
				BOOST_LOG_TRIVIAL(debug) << "there is an error in modifying the playlist item";
				return false;
			}
		}

		template <typename Iter>
		bool update_url_id_bylist(Iter const& iter, url_id_t url_id)
		{
			return update_playlist_item<by_playlist, decltype(playlist_item_t::_url_id), &playlist_item_t::_url_id>(
				iter,
				url_id);
		}

		template <typename Iter>
		bool update_url_id_hashed(Iter const& iter, url_id_t url_id)
		{
			return update_url_id_bylist(
				_playlist.project<by_playlist>(iter),
				url_id);
		}

		playlist_item_shr_t construct_playlist_item(std::string url);

		//playlist_const_iter_t get_list_iter(playlist_item_id_t playlist_item_id);

		template <typename Tag, typename id_type>
		auto get_hash_iter_temp(id_type item_or_url_id)
		{
			return _playlist.get<Tag>().find(item_or_url_id);
		}

		template <typename Tag, typename id_type>
		auto get_list_iter_temp(id_type item_or_url_id)
		{
			auto unordered_iter = get_hash_iter_temp<Tag>(item_or_url_id);
			if (unordered_iter == _playlist.get<Tag>().end())
			{
				return _playlist.get<by_playlist>().end();
			}

			return _playlist.project<by_playlist>(unordered_iter);
		}
		//playlist_itemlist_const_iter_t get_hash_iter(playlist_item_id_t playlist_item_id);

	public:
		playlist(std::shared_ptr<tag_parser_abstract> tag_parser);
		playlist(std::shared_ptr<tag_parser_abstract> tag_parser, std::string filename, bool reverse_insert);
		~playlist();

		std::string playlist_name();
		void set_playlist_name(std::string pl_name);

		playlist_const_iter_t get_seq_cbegin();
		playlist_const_iter_t get_seq_cend();
		playlist_iter_t get_seq_begin();
		playlist_iter_t get_seq_end();

		playlist_item_shr_t get_playlist_item_with_item_id(playlist_item_id_t playlist_item);
		playlist_item_shr_t get_playlist_item_with_url_id(url_id_t url_id);
		playlist_item_id_t get_last_item_id();
		playlist_item_id_t get_next_item_id(playlist_item_id_t playlist_item_id);

		playlist_item_shr_t add_item(std::string const& url);
		playlist_item_shr_t add_item(playlist_const_iter_t const& iter, std::string const& url);
		playlist_item_shr_t add_item(playlist_item_id_t playlist_item_id, std::string const& url);
		bool move_item(playlist_item_id_t from_playlist_item_id, playlist_item_id_t to_playlist_item_id);
		bool move_item(playlist_const_iter_t const& from_playlist_item_iter, playlist_const_iter_t const& to_playlist_item_iter);
		void move_items(playlist_const_iter_t const& from_playlist_item_id, playlist_const_iter_t const& to_playlist_item_iter, size_type count_items);
		void move_items(playlist_const_iter_t const& from_playlist_item_id, playlist_const_iter_t const& stop_playlist_item_id, playlist_const_iter_t const& to_playlist_item_iter);
		void move_items(std::vector<playlist_const_iter_t> const& items, playlist_const_iter_t const& to_playlist_item_iter);
		void move_items(std::vector<playlist_item_id_t> const& item_ids, playlist_item_id_t to_playlist_item_id);
		bool remove_item(playlist_const_iter_t const& item_iter);
		bool remove_item(playlist_item_id_t playlist_item_id);
		void remove_items(playlist_const_iter_t const& from_playlist_item_iter, playlist_const_iter_t const& to_playlist_item_iter);
		void remove_items(playlist_const_iter_t const& from_playlist_item_iter, size_type count_items);
		void clear();

		bool update_url_id(playlist_item_id_t playlist_item, url_id_t url_id);

		std::shared_ptr<playlist_item_shr_list_t> load_playlist(std::string filename, bool reverse_insert);
		bool save_playlist(std::string const& filename);
		std::string base_path();
		void set_album_art(std::shared_ptr<album_art> alb_art);

	};
}

#endif // playlist_h__

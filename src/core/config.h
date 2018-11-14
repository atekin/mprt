#ifndef config_h__
#define config_h__

#include <string>

#include <boost/property_tree/xml_parser.hpp>

#include "singleton.h"

namespace mprt {

namespace pt = boost::property_tree;

class config : public singleton<config> {
public:
	static const std::string CONFIG_STR;
	static const std::string CONFIG_STATE_FILE;
	static const std::string CONFIG_LOG_FILE;

	static const std::string STATE_STR;
	static const std::string STATE_CURRENT_PLAYLIST_ITEM;

	config(singleton<config>::token) {}

	std::string config_filename();
	std::string state_filename();
	bool init(std::string const& config_file, std::string const& state_file);
	bool init(std::string const& config_file);
	pt::ptree get_ptree_node(std::string nodename);

	template <typename T>
	T config_item(std::string const& item_name) {
		return config_state_item<T>(item_name, _config_tree);
	}

	template <typename T>
	T state_item(std::string const& item_name) {
		return config_state_item<T>(item_name, _state_tree);
	}

	boost::property_tree::ptree::path_type find_subtree(boost::property_tree::ptree const& haystack, boost::property_tree::ptree const& needle) {
		boost::property_tree::ptree::path_type path;

		if (!find_subtree_helper(haystack, needle, path))
			throw std::range_error("not subtree");

		return path;
	}


private:
	std::string _config_filename;
	std::string _state_filename;
	pt::ptree _config_tree;
	pt::ptree _state_tree;

	template <typename T>
	T config_state_item(std::string const& item_name, pt::ptree const& root_tree) {
		return root_tree.get<T>(item_name);
	}

	bool find_subtree_helper(boost::property_tree::ptree const& haystack, boost::property_tree::ptree const& needle, boost::property_tree::ptree::path_type& path) {
		if (std::addressof(haystack) == std::addressof(needle))
			return true;

		for (auto& child : haystack) {
			auto next = path;
			next /= child.first;

			if (std::addressof(child.second) == std::addressof(needle)
				|| find_subtree_helper(child.second, needle, next))
			{
				path = next;
				return true;
			}
		}

		return false;
	}
};
}

#endif // config_h__

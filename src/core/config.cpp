#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/log/trivial.hpp>

#include "config.h"

namespace mprt {
	const std::string config::CONFIG_STR = "mprt.config";
	const std::string config::CONFIG_STATE_FILE = config::CONFIG_STR + ".state_file";
	const std::string config::CONFIG_LOG_FILE = config::CONFIG_STR + ".log_file";

	const std::string config::STATE_STR = "mprt.states";
	const std::string config::STATE_CURRENT_PLAYLIST_ITEM = config::STATE_STR + ".current_playlist_item";


	std::string config::config_filename()
	{
		return _config_filename;
	}

	std::string config::state_filename()
	{
		return _state_filename;
	}

	bool config::init(std::string const& config_file, std::string const& state_file)
	{
		_config_filename = config_file;
		_state_filename = state_file;

		// Parse the XML into the property tree.
		try
		{
			pt::read_xml(_config_filename, _config_tree);
			pt::read_xml(_state_filename, _state_tree);
		}
		catch (pt::ptree_error const& err)
		{
			BOOST_LOG_TRIVIAL(debug) << err.what();

			return false;
		}

		return true;
	}

	bool config::init(std::string const& config_file)
	{
		_config_filename = config_file;

		// Parse the XML into the property tree.
		try
		{
			pt::read_xml(_config_filename, _config_tree);
		}
		catch (pt::ptree_error const& err)
		{
			BOOST_LOG_TRIVIAL(debug) << err.what();

			return false;
		}

		return true;
	}

	pt::ptree config::get_ptree_node(std::string nodename)
	{
		return _config_tree.get_child(nodename);
	}

}


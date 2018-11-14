#ifndef refcounting_plugin_api_h__
#define refcounting_plugin_api_h__

#include <memory>
#include <string>

#include <boost/dll/shared_library.hpp>
#include <boost/dll/import.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include "type_defs.h"
#include "plugin_types.h"

class refcounting_plugin_api /*: public std::enable_shared_from_this<refcounting_plugin_api>*/
{
protected:

	template <typename Container, typename ... Args>
	void call_callback_funcs(Container & cont, Args && ... args)
	{
		for (auto const& item : cont)
		{
			item.second(args...);
		}
	}

public:
	refcounting_plugin_api() {}

	virtual ~refcounting_plugin_api()
	{
		BOOST_LOG_TRIVIAL(debug) << "refcounting_plugin_api::~refcounting_plugin_api() called";
	}

	virtual void init(void *arguments = nullptr) = 0;

	virtual boost::filesystem::path location() const = 0;

	virtual std::string plugin_name() const = 0;

	virtual plugin_types plugin_type() const = 0;
};

struct library_holding_deleter {
	std::shared_ptr<boost::dll::shared_library> _lib;

	void operator()(refcounting_plugin_api * p) const {

		BOOST_LOG_TRIVIAL(debug) << "deleted " + p->plugin_name();

		delete p;
	}
};

inline std::shared_ptr<refcounting_plugin_api> bind(std::unique_ptr<refcounting_plugin_api> & plugin) {
	// getting location of the shared library that holds the plugin
	boost::filesystem::path location = plugin->location();

	// `make_shared` is an efficient way to create a shared pointer
	std::shared_ptr<boost::dll::shared_library> lib
		= std::make_shared<boost::dll::shared_library>(location);

	library_holding_deleter deleter;
	deleter._lib = lib;

	return std::shared_ptr<refcounting_plugin_api>(plugin.release(), deleter);
}

inline std::shared_ptr<refcounting_plugin_api> get_plugin(
	boost::filesystem::path path, const char* func_name)
{
	typedef std::unique_ptr<refcounting_plugin_api>(func_t)();

	//using func_t = std::unique_ptr<refcounting_plugin_api>(*)();

	std::function<func_t> creator = boost::dll::import_alias<func_t>(
		path,
		func_name,
		boost::dll::load_mode::append_decorations   // will be ignored for executable
		);

	// `plugin` does not hold a reference to shared library. If `creator` will go out of scope, 
	// then `plugin` can not be used.
	std::unique_ptr<refcounting_plugin_api> plugin{ std::move(creator()) };

	// Returned variable holds a reference to 
	// shared_library and it is safe to use it.
	return bind(plugin);

	// `creator` goes out of scope here and will be destroyed.
}

#endif // refcounting_plugin_api_h__

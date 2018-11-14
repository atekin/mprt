#ifndef output_plugin_dsound_h__
#define output_plugin_dsound_h__

#include <boost/log/trivial.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include "common/output_plugin_api.h"

struct IDirectSound8;
struct IDirectSoundBuffer;
struct IDirectSoundBuffer8;

namespace mprt
{
	
	class output_plugin_dsound : public output_plugin_api
	{
	private:
		IDirectSound8 * _output_context;
		IDirectSoundBuffer *_primary_buffer;
		IDirectSoundBuffer8 *_secondary_buffer;
		std::vector<std::pair<std::string, std::string>> _dxdevice_list;
		std::string _preffered_device_name;
		size_type _max_buffer_duration_msec;
		size_type _volume;
		size_type _write_offset;
		size_type _dsound_buffer_size;

		void get_dxdevice_list();
		LPCGUID get_preferred_device_id();
		BOOL ds_enum_callback(LPGUID lpGuid, LPCSTR description, LPCSTR module, LPVOID context);

		void reset_buffers();
		bool init_dsound();
		void set_volume_internal(size_type volume);
		size_type dsound_available_bytes_to_write();
		size_type dsound_available_bytes_to_play();
		std::chrono::microseconds get_next_duration();


		virtual void play() override;
		virtual void pause_play_internal() override;
		virtual void resume_play_internal() override;
		virtual void resume_clear_play_internal() override;
		virtual void stop_internal() override;
		virtual void pause_internal() override;
		//virtual void cont_internal() override;
		virtual void quit_internal() override;

		//virtual bool is_no_job() override;
		bool is_drained();
		size_type fill_drain(size_type bytes);
		virtual void fill_drain_internal() override;

		virtual void init_api() override;

	public:
		output_plugin_dsound() {}
		
		virtual ~output_plugin_dsound() {
			BOOST_LOG_TRIVIAL(debug) << "output_plugin_dsound::~output_plugin_dsound() called";
		}

		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void * arguments = nullptr) override;

		// job thread functions
		virtual void set_volume(size_type volume) override; // 0 to 100
		virtual size_type get_volume() override { return _volume; }

	};

}

#endif // output_plugin_dsound_h__

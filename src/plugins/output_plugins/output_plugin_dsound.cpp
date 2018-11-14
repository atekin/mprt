#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")


#include <boost/dll/runtime_symbol_info.hpp>

#include "core/config.h"

#include "common/decoder_plugin_api.h"
#include "common/scope_exit.h"

#include "output_plugin_dsound.h"

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <MMReg.h>
#include <ks.h>
#include <Ksmedia.h>

#include "common/type_defs.h"

namespace mprt
{
	static inline std::string utf16to8(const wchar_t* utf16) {
		if (!utf16) return "";
		int size = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, 0, 0, 0, 0);
		if (size <= 0) return "";
		char* buffer = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, utf16, -1, buffer, size, 0, 0);
		std::string utf8str(buffer);
		delete[] buffer;
		return utf8str;
	}

	static inline std::wstring utf8to16(const char* utf8) {
		int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, 0, 0);
		if (size <= 0) return L"";
		wchar_t* buffer = new wchar_t[size];
		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buffer, size);
		std::wstring utf16fn(buffer);
		delete[] buffer;
		return utf16fn;
	}

	BOOL CALLBACK ds_enum_callback_C(LPGUID lpGuid, LPCSTR description, LPCSTR module, LPVOID context) {
		auto _dxdevice_list = (std::vector<std::pair<std::string, std::string>>*)context;

		std::string id = "";
		std::string desc = description;

		if (lpGuid) {
			OLECHAR* guidString;
			StringFromCLSID(*lpGuid, &guidString);
			id = utf16to8(guidString);
			CoTaskMemFree(guidString);
		}
		_dxdevice_list->push_back(std::make_pair(id, desc));

		return 1;
	}
	
	// Must be instantiated in plugin
	boost::filesystem::path output_plugin_dsound::location() const {
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string output_plugin_dsound::plugin_name() const {
		return "output_plugin_dsound";
	}

	plugin_types output_plugin_dsound::plugin_type() const {
		return plugin_types::output_plugin;
	}

	void output_plugin_dsound::init(void * /*arguments*/)
	{
		config::instance().init("../config/config_output_plugin_dsound.xml");
		auto pt = config::instance().get_ptree_node("mprt.output_plugin_dsound");
		_async_task = std::make_shared<async_tasker>(pt.get<std::size_t>("max_free_timer_count", 10));
		_preffered_device_name = pt.get<std::string>("preffered_device_name", "Primary Sound Driver");
		_max_buffer_duration_msec = pt.get<size_type>("max_buffer_duration_msec", 200);
		_max_chunk_read_size = pt.get<size_type>("max_chunk_read_size", 128) * 1024;
		_use_duration = pt.get<std::string>("use_memory_size_or_durationms", "duration") == "duration";
		if (!_use_duration)
		{
			_max_memory_size_per_file = pt.get<size_type>("max_memory_size_per_file_duration", 4096) * 1024;
		}
		else
		{
			_max_memory_size_per_file = pt.get<size_type>("max_memory_size_per_file_duration", 20000);

		}
		while (!_async_task->is_ready()) {
			std::this_thread::yield();
		}

		add_job([this]() { 
			get_dxdevice_list();
			_primary_buffer = nullptr;
			_secondary_buffer = nullptr;
			_output_context = nullptr;
			_init_api = false;
		});

	}


	void output_plugin_dsound::stop_internal()
	{
		//fill_drain(_dsound_buffer_size);

		if (_secondary_buffer) {
			_secondary_buffer->Stop();
		}

		_init_api = false;

		for (auto & sound_det : _sound_details_queue)
		{
			sound_det._decoder_play_finished_callback(sound_det._url_id);
		}

		output_plugin_api::stop_internal();
		sound_plugin_api::reset_buffers();

	}

	void output_plugin_dsound::pause_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "output_plugin_dsound::pause_internal() called";

		pause_play_internal();
	}

	void output_plugin_dsound::quit_internal()
	{
	}

	void output_plugin_dsound::set_volume(size_type volume)
	{
		add_job([this, volume]() { set_volume_internal(volume); });
	}


	void output_plugin_dsound::get_dxdevice_list()
	{
		//auto ds_cb = bind_cpp_mem_fn_to_c<BOOL(*)(LPGUID, LPCSTR, LPCSTR, LPVOID)>(&output_plugin_dsound::ds_enum_callback, this);

		DirectSoundEnumerate(ds_enum_callback_C, LPVOID(&_dxdevice_list));
	}

	LPCGUID output_plugin_dsound::get_preferred_device_id()
	{
		GUID* guid = nullptr;

		/* if we have a preferred device id, see if we can find it in the CURRENT
			devices! otherwise we'll return null for the primary device */
		for (auto device_id_desc : _dxdevice_list) {
			if (device_id_desc.second == _preffered_device_name) {
				std::wstring guidW = utf8to16(device_id_desc.first.c_str());
				guid = new GUID();
				HRESULT result = CLSIDFromString(guidW.c_str(), guid);
				if (result != S_OK) {
					delete guid;
					guid = nullptr;
					break;
				}
			}
		}

		return guid;
	}

	BOOL output_plugin_dsound::ds_enum_callback(LPGUID lpGuid, LPCSTR description, LPCSTR module, LPVOID context)
	{
		auto _dxdevice_list = (std::vector<std::pair<std::string, std::string>>*)context;

		std::string id = "";
		std::string desc = description;

		if (lpGuid) {
			OLECHAR* guidString;
			StringFromCLSID(*lpGuid, &guidString);
			id = utf16to8(guidString);
			CoTaskMemFree(guidString);
		}
		_dxdevice_list->push_back(std::make_pair(id, desc));

		return 1;
	}

	void output_plugin_dsound::reset_buffers()
	{
		_output_context = nullptr;
		_primary_buffer = _secondary_buffer = nullptr;
		_init_api = false;
	}

	bool output_plugin_dsound::init_dsound()
	{
		if (_sound_details_queue.empty()) {
			//wait until some sound detail has arrived ...
			return false;
		}

		auto & new_sound_dets = sound_details_top_ref();

		auto is_sound_details_same_as_before_ = is_sound_details_same_as_before();
		_prev_sound_details = new_sound_dets;

		BOOST_LOG_TRIVIAL(debug) << "direct sound changing current cache";

		// check for previous ...
		if (_output_context
			&& _primary_buffer
			&& _secondary_buffer
			&& is_sound_details_same_as_before_) {

			_init_api = true;

			return true;
		}

		if (_secondary_buffer) {
			_secondary_buffer->Stop();
			_secondary_buffer->Release();
			_secondary_buffer = nullptr;
		}

		HRESULT result;
		if (!_output_context) {
			// get the preferred device ...
			LPCGUID guid = get_preferred_device_id();
			result = DirectSoundCreate8(guid, &_output_context, nullptr);
			delete guid;

			if (result != S_OK) {
				result = DirectSoundCreate8(nullptr, &_output_context, nullptr);
			}

			if (result != DS_OK) {
				return false;
			}

			/* DSSCL_PRIORITY allows us to change the sample format */
			result = _output_context->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

			if (result != DS_OK) {
				return false;
			}
		}

		if (_primary_buffer == nullptr) {
			DSBUFFERDESC bufferInfo;
			ZeroMemory(&bufferInfo, sizeof(DSBUFFERDESC));

			/* LOCHARDWARE means we want the audio hardware (not software) to
			do the mixing for us. */
			bufferInfo.dwFlags =
				DSBCAPS_PRIMARYBUFFER |
				DSBCAPS_CTRLVOLUME |
				DSBCAPS_LOCHARDWARE;

			bufferInfo.dwSize = sizeof(DSBUFFERDESC);
			bufferInfo.dwBufferBytes = 0;
			bufferInfo.lpwfxFormat = nullptr;

			result = _output_context->CreateSoundBuffer(
				&bufferInfo, &_primary_buffer, nullptr);

			if (result != DS_OK) {
				/* try again with software mixing... */
				bufferInfo.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
				result = _output_context->CreateSoundBuffer(
					&bufferInfo, &_primary_buffer, nullptr);

				if (result != DS_OK) {
					return false;
				}
			}
		}

		if (_secondary_buffer == nullptr) {
			BOOST_LOG_TRIVIAL(debug) << "starting new direct sound parameters";

			DSBUFFERDESC bufferInfo;
			ZeroMemory(&bufferInfo, sizeof(DSBUFFERDESC));

			bufferInfo.dwSize = sizeof(DSBUFFERDESC);

			bufferInfo.dwFlags =
				DSBCAPS_CTRLFREQUENCY |
				DSBCAPS_CTRLPAN |
				DSBCAPS_CTRLVOLUME |
				DSBCAPS_GLOBALFOCUS |
				DSBCAPS_GETCURRENTPOSITION2 |
				DSBCAPS_CTRLPOSITIONNOTIFY;

			DWORD speakerConfig = 0;
			switch (new_sound_dets._channels) {
			case 1:
				speakerConfig = KSAUDIO_SPEAKER_MONO;
				break;
			case 2:
				speakerConfig = KSAUDIO_SPEAKER_STEREO;
				break;
			case 4:
				speakerConfig = KSAUDIO_SPEAKER_QUAD;
				break;
			case 5:
				speakerConfig = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT);
				break;
			case 6:
				speakerConfig = KSAUDIO_SPEAKER_5POINT1;
				break;
			case 7:
				speakerConfig = KSAUDIO_SPEAKER_7POINT0;
				break;
			case 8:
				speakerConfig = KSAUDIO_SPEAKER_7POINT1;
				break;
			}

			WAVEFORMATEXTENSIBLE wf;
			ZeroMemory(&wf, sizeof(WAVEFORMATEXTENSIBLE));
			wf.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
			wf.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			wf.Format.nChannels = (WORD)new_sound_dets._channels;
			wf.Format.wBitsPerSample = (WORD)new_sound_dets._bps;
			wf.Format.nSamplesPerSec = (DWORD)new_sound_dets._sample_rate;
			wf.Samples.wValidBitsPerSample = (WORD)new_sound_dets._bps;
			wf.Format.nBlockAlign = (wf.Format.wBitsPerSample / 8) * wf.Format.nChannels;
			wf.Format.nAvgBytesPerSec = wf.Format.nSamplesPerSec * wf.Format.nBlockAlign;
			wf.dwChannelMask = speakerConfig;
			wf.SubFormat = new_sound_dets._is_float ?  KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;

			bufferInfo.lpwfxFormat = (WAVEFORMATEX*)&wf;
			_dsound_buffer_size = time_duration_to_bytes(std::chrono::milliseconds(_max_buffer_duration_msec), new_sound_dets);
			bufferInfo.dwBufferBytes = (DWORD)(_dsound_buffer_size);

			IDirectSoundBuffer *tempBuffer;
			result = _output_context->CreateSoundBuffer(&bufferInfo, &tempBuffer, nullptr);

			if (result != DS_OK) {
				return false;
			}

			result = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&_secondary_buffer);
			if (result != DS_OK) {
				tempBuffer->Release();
				return false;
			}

			// we requested the buffer, but what is given to us?
			DSBCAPS bufferCaps;
			ZeroMemory(&bufferCaps, sizeof(bufferCaps));
			bufferCaps.dwSize = sizeof(bufferCaps);
			if (DS_OK != _secondary_buffer->GetCaps(&bufferCaps)) {
				return false;
			}
			_dsound_buffer_size = bufferCaps.dwBufferBytes;

			_write_offset = 0;
			tempBuffer->Release();
		}
		_primary_buffer->SetCurrentPosition(0);
		_primary_buffer->Play(0, 0, DSBPLAY_LOOPING);

		_secondary_buffer->SetCurrentPosition(0);
		_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

		_init_api = true;

		return true;
	}

	void output_plugin_dsound::set_volume_internal(size_type volume)
	{
		_volume = volume;


		if (_secondary_buffer) {
			/* normalize between 1 and 10000 (DSBVOLUME_MIN) */
			double scaled = fabs(volume * 100);
			scaled = scaled < 0 ? 1 : scaled;

			/* found on experts-exchange (20181717) */
			double db = ((log10(scaled) / 4) * 10000) - 10000;

			if (db > DSBVOLUME_MAX) {
				db = DSBVOLUME_MAX;
			}

			if (db < DSBVOLUME_MIN) {
				db = DSBVOLUME_MIN;
			}

			_secondary_buffer->SetVolume((LONG)db);
		}
	}

	void output_plugin_dsound::play()
	{
		_STATE_CHECK_(plugin_states::play);

		std::chrono::microseconds next_duration(10000);

		if (_play_timer && is_active_timer(_play_timer))
		{
			return;
		}

		SCOPE_EXIT_REF(
			if (!is_no_job() && _current_state == plugin_states::play) {
				if (!_play_timer || (_play_timer && is_timer_expired(_play_timer)))
				{
					_play_timer = add_job_thread_internal([this, next_duration]() {play(); }, next_duration);
					//BOOST_LOG_TRIVIAL(debug) << "play called with: " << next_duration.count() << " microseconds with timer: " << _play_timer;
				}
			}
			else {
				BOOST_LOG_TRIVIAL(debug) << "stopping dsound";

				pause_play_internal();
				give_cache_buf_back(_prev_sound_details._url_id);
				_init_api = false;
			}
		);

		if (_current_state != plugin_states::play) {
			BOOST_LOG_TRIVIAL(debug) << "direct sound state not play";

			return;
		}

		if (_sound_details_queue.empty())
			return;

		auto & current_sound_dets = sound_details_top_ref();

		if (!_init_api) {
			auto is_init_ok = init_dsound();
			
			if (!is_init_ok)
			{
				BOOST_LOG_TRIVIAL(debug) << "cannot initialize the direct sound with the current sound parameters";
				current_sound_dets._current_cache_buffer->clear_data();
				current_sound_dets._decoder_play_finished_callback(current_sound_dets._url_id);

				sound_details_pop();

				return;
			}
		}

		if (!current_sound_dets._current_cache_buffer)
		{
			sound_details_pop();
			return;
		}
		int wait_count = 0;
		while (current_sound_dets._current_cache_buffer->is_data_empty())
		{
			BOOST_LOG_TRIVIAL(debug) << "sound waiting for the decoder";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			++wait_count;
			if (wait_count > 20)
			{
				sound_details_pop();
				return;
			}
		}

		unsigned char *dst1 = nullptr, *dst2 = nullptr;
		DWORD size1 = 0, size2 = 0;
		auto avail_bytes_to_write = std::min({ 
			dsound_available_bytes_to_write()
			, current_sound_dets._current_cache_buffer->total_bytes_in_buffer_guess()
			//, samples_to_bytes(current_sound_dets._total_samples - current_sound_dets._current_samples_written_to_sound_buffer, current_sound_dets)
			});
		//BOOST_LOG_TRIVIAL(debug) << "should start play soon avail bytes: " << avail_bytes_to_write;

		HRESULT hr = _secondary_buffer->Lock(
			(DWORD)_write_offset,
			(DWORD)avail_bytes_to_write,
			(void **)&dst1, &size1,
			(void **)&dst2, &size2,
			0);

		if (DSERR_BUFFERLOST == hr)
		{
			_secondary_buffer->Lock(
				(DWORD)_write_offset,
				(DWORD)avail_bytes_to_write,
				(void **)&dst1, &size1,
				(void **)&dst2, &size2,
				0);
		}

		if (SUCCEEDED(hr))
		{
			auto writen_bytes1 = (DWORD)current_sound_dets._current_cache_buffer->write_into_raw_buffer(
				(buffer_elem_t*)dst1,
				static_cast<size_type>(size1));
			
			auto writen_bytes2 = 0;

			if (NULL != dst2)
			{
				writen_bytes2 = (DWORD)current_sound_dets._current_cache_buffer->write_into_raw_buffer(
					(buffer_elem_t*)dst2,
					static_cast<size_type>(size2));
			}

			auto total_writen_bytes = writen_bytes1 + writen_bytes2;
			current_sound_dets._current_samples_written_to_sound_buffer += bytes_to_samples(total_writen_bytes, current_sound_dets);

			// Release the data back to DirectSound. 
			hr = _secondary_buffer->Unlock(dst1, size1, dst2,
				size2);
			
			if (SUCCEEDED(hr))
			{
				avail_bytes_to_write -= total_writen_bytes;
				// we do not need to refill buffer at this duration
				next_duration = bytes_to_time_duration(_dsound_buffer_size - avail_bytes_to_write, current_sound_dets) / 2;

				//BOOST_LOG_TRIVIAL(debug) << "play cur duration msecs: " << next_duration.count();
				
				_write_offset += total_writen_bytes;
				_write_offset %= _dsound_buffer_size;

				call_callback_funcs(_progress_func_call_list, current_sound_dets._url_id, samples_to_time_duration(current_sound_dets._current_samples_written_to_sound_buffer, current_sound_dets).count() / 1000);

				if (current_sound_dets._current_samples_written_to_sound_buffer == current_sound_dets._total_samples) {
					BOOST_LOG_TRIVIAL(debug) << "direct sound play finished for: " << current_sound_dets._url_id;

					current_sound_dets._decoder_play_finished_callback(current_sound_dets._url_id);
					_prev_sound_details._current_samples_written_to_sound_buffer = current_sound_dets._current_samples_written_to_sound_buffer;
					sound_details_pop();

					if (!is_sound_details_same_as_before())
					{
						size_type write_play_bytes = 2 * dsound_available_bytes_to_play();
						size_type write_bytes = write_play_bytes;

						// drain the remaining buffer
						while ( write_play_bytes > 0) {
							write_bytes = std::min(dsound_available_bytes_to_write(), write_play_bytes);

							// allow some play
							std::this_thread::sleep_for(bytes_to_time_duration(write_play_bytes, _prev_sound_details) / 3);
							// fill buf
							auto real_written_bytes = fill_drain(write_bytes);
							// how much to write to finish the play buffer
							write_play_bytes -= real_written_bytes;
							BOOST_LOG_TRIVIAL(debug) << "drain bytes: " << write_play_bytes;

							_prev_sound_details._current_samples_written_to_sound_buffer += bytes_to_samples(real_written_bytes, _prev_sound_details);
							call_callback_funcs(_progress_func_call_list, _prev_sound_details._url_id, samples_to_time_duration(_prev_sound_details._current_samples_written_to_sound_buffer, _prev_sound_details).count() / 1000);
						}

						// play the remaining
						std::this_thread::sleep_for(bytes_to_time_duration(dsound_available_bytes_to_play(), _prev_sound_details));
					
						_secondary_buffer->Stop();
						_primary_buffer->Stop();
					}
					else
					{
						next_duration = std::chrono::microseconds(0);
					}

					give_cache_buf_back(_prev_sound_details._url_id);
					_init_api = false;
				}

				if (is_no_job()) {
					BOOST_LOG_TRIVIAL(debug) << "direct sound play finished";

					_current_state = plugin_states::stop;
				}

			}
		}
	}

	void output_plugin_dsound::pause_play_internal()
	{
		//if (_primary_buffer)
		//{
		//	_primary_buffer->Stop();
		//}

		if (_secondary_buffer)
		{
			_secondary_buffer->Stop();
		}
	}

	void output_plugin_dsound::resume_play_internal()
	{
		//if (_primary_buffer)
		//{
		//	_primary_buffer->Play(0, 0, DSBPLAY_LOOPING);
		//}
		if (_secondary_buffer)
		{
			_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
		}

		play();
	}

	void output_plugin_dsound::resume_clear_play_internal()
	{
		//if (_primary_buffer)
		//{
		//	_primary_buffer->SetCurrentPosition(0);
		//	_primary_buffer->Play(0, 0, DSBPLAY_LOOPING);
		//}

		if (_secondary_buffer) {
			_secondary_buffer->SetCurrentPosition(0);
			_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
		}

		play();
	}

	size_type output_plugin_dsound::dsound_available_bytes_to_play()
	{
		return _dsound_buffer_size - dsound_available_bytes_to_write();
	}

	size_type output_plugin_dsound::dsound_available_bytes_to_write()
	{
		DWORD playoffset = 0;
		DWORD junk = 0;
		_secondary_buffer->GetCurrentPosition(&playoffset, &junk);

		//BOOST_LOG_TRIVIAL(debug) <<
		//	"play " << playoffset << " " << writeOffset << "diff: " << writeOffset - playoffset;

		//if (writeOffset == playoffset) {
		//	return 0;
		//}

		size_type avail;
		size_type diff_byte = playoffset - _write_offset;

// 		if (writeOffset >= playoffset) {
// 			avail = _dsound_buffer_size +diff_byte;
// 		}
// 		else {
// 			avail = diff_byte;
// 		}

		avail = (diff_byte > 0) ? diff_byte : _dsound_buffer_size + diff_byte;

		// BOOST_LOG_TRIVIAL(debug) << "available: " << avail;

		return avail;
	}

	std::chrono::microseconds output_plugin_dsound::get_next_duration()
	{
		return bytes_to_time_duration(dsound_available_bytes_to_play(), sound_details_top());
	}

	bool output_plugin_dsound::is_drained()
	{
		return dsound_available_bytes_to_write() == _dsound_buffer_size;
	}


	size_type output_plugin_dsound::fill_drain(size_type bytes)
	{
		if (!_primary_buffer || !_secondary_buffer)
		{
			return 0;
		}

		unsigned char *dst1 = nullptr, *dst2 = nullptr;
		DWORD size1 = 0, size2 = 0;

		HRESULT hr = _secondary_buffer->Lock(
			(DWORD)_write_offset,
			(DWORD)bytes,
			(void **)&dst1, &size1,
			(void **)&dst2, &size2,
			0);

		if (DSERR_BUFFERLOST == hr)
		{
			_secondary_buffer->Lock(
				(DWORD)_write_offset,
				(DWORD)bytes,
				(void **)&dst1, &size1,
				(void **)&dst2, &size2,
				0);
		}

		if (SUCCEEDED(hr))
		{

			std::memset(dst1, 0, size1);

			if (NULL != dst2)
			{
				std::memset(dst2, 0, size2);
			}

			// Release the data back to DirectSound. 
			hr = _secondary_buffer->Unlock(dst1, size1, dst2,
				size2);
		}

		auto total_writen_bytes = size1 + size2;
		_write_offset += total_writen_bytes;
		_write_offset %= _dsound_buffer_size;

		return total_writen_bytes;
	}

	void output_plugin_dsound::fill_drain_internal()
	{
		fill_drain(_dsound_buffer_size);
	}

	void output_plugin_dsound::init_api()
	{
		init_dsound();
	}

}

// Factory method. Returns *simple pointer*!
std::unique_ptr<refcounting_plugin_api> create() {
	return std::make_unique<mprt::output_plugin_dsound>();
}

BOOST_DLL_ALIAS(create, create_refc_plugin)

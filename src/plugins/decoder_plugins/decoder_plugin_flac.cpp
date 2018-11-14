//MinGW related workaround
#define BOOST_DLL_FORCE_ALIAS_INSTANTIATION

#include <cmath>
#include <iostream>

#include <boost/dll/runtime_symbol_info.hpp>

#include <core/config.h>
#include "common/job_type_enums.h"
#include "common/input_plugin_api.h"
#include "common/output_plugin_api.h"
#include "common/enum_cast.h"

#include "decoder_plugin_flac.h"

extern "C"
{
	static FLAC__StreamDecoderReadStatus read_callback_C(
		const FLAC__StreamDecoder * decoder,
		FLAC__byte buffer[],
		size_t *bytes,
		void * client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->read_callback(decoder, buffer, bytes, client_data);
	}

	static FLAC__StreamDecoderSeekStatus seek_callback_C(
		const FLAC__StreamDecoder * decoder,
		FLAC__uint64 absolute_byte_offset,
		void * client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->seek_callback(decoder, absolute_byte_offset, client_data);
	}

	static FLAC__StreamDecoderTellStatus tell_callback_C(
		const FLAC__StreamDecoder * decoder,
		FLAC__uint64 *absolute_byte_offset,
		void * client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->tell_callback(decoder, absolute_byte_offset, client_data);
	}

	static FLAC__StreamDecoderLengthStatus length_callback_C(
		const FLAC__StreamDecoder * decoder,
		FLAC__uint64 *stream_length,
		void * client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->length_callback(decoder, stream_length, client_data);
	}

	static FLAC__bool eof_callback_C(
		const FLAC__StreamDecoder * decoder,
		void * client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->eof_callback(decoder, client_data);
	}

	static FLAC__StreamDecoderWriteStatus write_callback_C(
		const FLAC__StreamDecoder *decoder,
		const FLAC__Frame *frame,
		const FLAC__int32 * const buffer[],
		void *client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->write_callback(decoder, frame, buffer, client_data);
	}

	static void error_callback_C(
		const FLAC__StreamDecoder *decoder,
		FLAC__StreamDecoderErrorStatus status,
		void *client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->error_callback(decoder, status, client_data);
	}

	static void metadata_callback_C(const FLAC__StreamDecoder *decoder,
		const FLAC__StreamMetadata *metadata,
		void *client_data)
	{
		mprt::decoder_plugin_flac *decoder_flac = reinterpret_cast<mprt::decoder_plugin_flac*>(client_data);
		return decoder_flac->metadata_callback(decoder, metadata, client_data);
	}
}

namespace mprt {

	decoder_plugin_flac::decoder_plugin_flac()
		//: _decoder(nullptr)
		//: _init_flac(false)
		: _decoders(3)
		, _finish_flac_dec_func(std::bind(&decoder_plugin_flac::finish_flac_decoder, this, std::placeholders::_1))
	{
	
	}

	decoder_plugin_flac::~decoder_plugin_flac() {
		BOOST_LOG_TRIVIAL(debug) << "flac_decode_plugin::~flac_decode_plugin() called";
	}

	// Must be instantiated in plugin
	boost::filesystem::path decoder_plugin_flac::location() const {
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string decoder_plugin_flac::plugin_name() const {
		return "decoder_plugin_flac";
	}

	plugin_types decoder_plugin_flac::plugin_type() const {
		return plugin_types::decoder_plugin;
	}

	void decoder_plugin_flac::init(void *)
	{
		config::instance().init("../config/config_decoder_plugin_flac.xml");
		auto pt = config::instance().get_ptree_node("mprt.decoder_plugin_flac");

		_check_md5 = (pt.get<std::string>("check_md5") == "true");
		_max_chunk_read_size = pt.get<size_type>("max_chunk_read_size", 128) * 1024;
		_max_memory_size_per_file = pt.get<size_type>("max_memory_size_per_file", 16384) * 1024;
		_priority = pt.get<size_type>("priority", 100);

		auto file_extensions = pt.get_child("file_extensions");
		for (auto & file_extension : file_extensions)
		{
			_supported_extensions.push_back(std::regex(file_extension.second.data(),
				std::regex_constants::ECMAScript | std::regex_constants::icase));
		}

		_play_finished_callback =
			std::bind(
				&decoder_plugin_flac::play_finished_callback<flac_cache_man_t, finish_flac_dec_func_t>,
				this,
				std::ref(_decoders),
				_finish_flac_dec_func,
				std::placeholders::_1);
	}

	void decoder_plugin_flac::init_api()
	{
		init_flac();
	}

	void decoder_plugin_flac::decode()
	{
		auto cur_det = _decoder_plugins_manager->get_current_decoder_details();
		auto pdecoder = _decoders.get_from_cache(cur_det->_sound_details._url_id).get();
		if (!pdecoder)
		{
			return;
		}

		FLAC__stream_decoder_process_single(pdecoder);

		auto decoder_state = FLAC__stream_decoder_get_state(pdecoder);
		if ((decoder_state == FLAC__STREAM_DECODER_END_OF_STREAM ||
			decoder_state == FLAC__STREAM_DECODER_ABORTED)) {

			_decoder_plugins_manager->finish_decode_internal_single(cur_det->_sound_details._url_id);
		}

		//BOOST_LOG_TRIVIAL(debug) << "flac_decode_plugin::decode_cont exited";
	}

	void decoder_plugin_flac::seek_duration(size_type duration_ms)
	{
		auto & decoder_dets = _decoder_plugins_manager->get_current_decoder_details_ref();

		auto sample_to_seek = (FLAC__uint64)((double)decoder_dets->_sound_details._sample_rate * duration_ms / 1000.0);

		auto pdecoder = _decoders.get_from_cache(decoder_dets->_sound_details._url_id).get();

		if (FLAC__stream_decoder_seek_absolute(pdecoder, sample_to_seek)) {
			return;
		}
		else
		{
			if (FLAC__stream_decoder_get_state(pdecoder) == FLAC__STREAM_DECODER_SEEK_ERROR) {
				if (FLAC__stream_decoder_flush(pdecoder)) {
					if (FLAC__stream_decoder_seek_absolute(pdecoder, sample_to_seek)) {
						return;
					}
					else
					{
						BOOST_LOG_TRIVIAL(debug) << "FLAC seek error";
					}
				}
			}
		}
	}

	decoder_plugin_flac::flac_cache_man_t::cache_item_t decoder_plugin_flac::create_new_flac_decoder()
	{
		return std::shared_ptr<FLAC__StreamDecoder>(
			FLAC__stream_decoder_new(),
			[](FLAC__StreamDecoder *pdecoder)
			{
				FLAC__stream_decoder_delete(pdecoder);
				pdecoder = nullptr;
			});
	}

	void decoder_plugin_flac::finish_flac_decoder(decoder_plugin_flac::flac_cache_man_t::cache_item_t decoder)
	{
		if (!FLAC__stream_decoder_finish(decoder.get())) {
			BOOST_LOG_TRIVIAL(error) << "FLAC finish decoder error";
		}
	}

	bool decoder_plugin_flac::init_flac()
	{
		auto & decoder_dets = _decoder_plugins_manager->get_current_decoder_details_ref();
		decoder_dets->_current_cache_buf = get_cache_put_buf(decoder_dets->_sound_details._url_id);
		auto flac_decoder = _decoders.get_from_cache(std::bind(&decoder_plugin_flac::create_new_flac_decoder, this), decoder_dets->_sound_details._url_id);
		auto pflac_decoder = flac_decoder.get();

		auto init_status =
			FLAC__stream_decoder_init_stream(
				pflac_decoder,
				read_callback_C, seek_callback_C, tell_callback_C,
				length_callback_C, eof_callback_C, write_callback_C,
				metadata_callback_C, error_callback_C, this);

		bool is_init_flac = 
			(FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED == init_status ||
			FLAC__STREAM_DECODER_INIT_STATUS_OK == init_status);

		if (!is_init_flac) return false;

		(void)FLAC__stream_decoder_set_md5_checking(pflac_decoder, _check_md5);

		FLAC__stream_decoder_set_metadata_ignore_all(pflac_decoder);
		FLAC__stream_decoder_set_metadata_respond(pflac_decoder,
			FLAC__METADATA_TYPE_STREAMINFO);

		auto result = FLAC__stream_decoder_process_until_end_of_metadata(pflac_decoder);
		if (!result) {
			BOOST_LOG_TRIVIAL(debug) <<
				"metadata error: " << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(pflac_decoder)];
		}

		return result;
	}

	void decoder_plugin_flac::seek_internal(size_type msecs)
	{
		auto & decoder_dets = _decoder_plugins_manager->get_current_decoder_details_ref();
		FLAC__uint64 sample_seek = static_cast<FLAC__uint64>(decoder_dets->_sound_details._sample_rate * msecs * 1000.);

		auto pdecoder = _decoders.get_from_cache(decoder_dets->_sound_details._url_id).get();
		if (!FLAC__stream_decoder_seek_absolute(pdecoder, sample_seek)) {
			if (FLAC__STREAM_DECODER_SEEK_ERROR == FLAC__stream_decoder_get_state(pdecoder)) {
				if (FLAC__stream_decoder_flush(pdecoder)) {
					FLAC__stream_decoder_seek_absolute(pdecoder, sample_seek);
				}
			}
		}
	}

	void decoder_plugin_flac::init_decode_internal_single(url_id_t url_id)
	{
		init_decode_internal_single_internal(_decoders, url_id);
	}

	void decoder_plugin_flac::reset_buffers()
	{
		decoder_plugin_api::reset_buffers();

	}


	// consumer of the input
	FLAC__StreamDecoderReadStatus decoder_plugin_flac::read_callback(
		const FLAC__StreamDecoder * /*decoder*/,
		FLAC__byte buffer[],
		size_t *bytes,
		void  * /*client_data*/)
	{
		auto read_res = _decoder_plugins_manager->decoder_read_buffer(reinterpret_cast<buffer_elem_t*>(buffer), *bytes);
		*bytes = static_cast<std::size_t>(read_res.first);

		if (-1 == *bytes) {
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
		else if (0 == *bytes)
		{
			if (_decoder_plugins_manager->decoder_is_exhausted()) {
				BOOST_LOG_TRIVIAL(debug) << "stream exhausted";
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			}
			else if (_decoder_plugins_manager->decoder_is_last_read_empty()) {
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE; // try another time
			}
		}
		
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}

	FLAC__StreamDecoderSeekStatus decoder_plugin_flac::seek_callback(
		const FLAC__StreamDecoder * /*decoder*/,
		FLAC__uint64 absolute_byte_offset,
		void * /*client_data*/)
	{
		auto const& cur_decoder_details = _decoder_plugins_manager->get_current_decoder_details();

		return
			cur_decoder_details->_seek_supported ?
				(_decoder_plugins_manager->seek_byte_internal(cur_decoder_details->_sound_details._url_id, static_cast<size_type>(absolute_byte_offset)) ?
					FLAC__STREAM_DECODER_SEEK_STATUS_OK :
					FLAC__STREAM_DECODER_SEEK_STATUS_ERROR) :
				FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
	}

	FLAC__StreamDecoderTellStatus decoder_plugin_flac::tell_callback(
		const FLAC__StreamDecoder * /*decoder*/, 
		FLAC__uint64 *absolute_byte_offset, 
		void * /*client_data*/)
	{
		BOOST_LOG_TRIVIAL(trace) << "flac_decode_plugin::tell_callback called";

		auto const& cur_decoder_details = _decoder_plugins_manager->get_current_decoder_details();

		return 
			cur_decoder_details->_tell_supported ?
				((*absolute_byte_offset = _decoder_plugins_manager->decoder_tell()) ?
					FLAC__STREAM_DECODER_TELL_STATUS_OK : FLAC__STREAM_DECODER_TELL_STATUS_ERROR) :
				FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
	}

	FLAC__StreamDecoderLengthStatus decoder_plugin_flac::length_callback(
		const FLAC__StreamDecoder * /*decoder*/,
		FLAC__uint64 *stream_length,
		void * /*client_data*/)
	{
		BOOST_LOG_TRIVIAL(trace) << "flac_decode_plugin::length_callback called";

		auto const& cur_decoder_details = _decoder_plugins_manager->get_current_decoder_details();

		return 
			cur_decoder_details->_length_supported ?
				((*stream_length = _decoder_plugins_manager->decoder_length()) ?
					FLAC__STREAM_DECODER_LENGTH_STATUS_OK : FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR) :
				FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
	}

	FLAC__bool decoder_plugin_flac::eof_callback(const FLAC__StreamDecoder * /*decoder*/, void * /*client_data*/)
	{
		//BOOST_LOG_TRIVIAL(trace) << "flac_decode_plugin::eof_callback called";
		auto const& cur_decoder_details = _decoder_plugins_manager->get_current_decoder_details();

		return 
			cur_decoder_details->_length_supported ?
				(_decoder_plugins_manager->decoder_eof() ? 1 : 0) :
				0;
	}

	// producer of the flac to pcm
	FLAC__StreamDecoderWriteStatus decoder_plugin_flac::write_callback(
		const FLAC__StreamDecoder * /*decoder*/,
		const FLAC__Frame *frame,
		const FLAC__int32 * const buffer[],
		void * /*client_data*/)
	{
		//std::cout << "w" << std::flush;
		auto & cur_dec_det = _decoder_plugins_manager->get_current_decoder_details_ref();
		
		/* write decoded PCM samples */
		auto one_sample_to_byte = samples_to_bytes(1, cur_dec_det->_sound_details);
		auto total_bytes_to_write = one_sample_to_byte * frame->header.blocksize;
		using pf = void (decoder_plugin_flac::*)(shared_chunk_buffer_t & /*buf*/, float_int32_bytes /*sample*/, std::size_t /*sample_width*/);
		pf push_call;
		std::size_t sample_width;
		switch (cur_dec_det->_sound_details._orig_bps)
		{
		case 24:
			push_call = &decoder_plugin_flac::push_func_call_24;
			sample_width = 4;
			break;
		default:
			push_call = &decoder_plugin_flac::push_func_call;
			sample_width = static_cast<std::size_t>(cur_dec_det->_sound_details._bps / 8);
			break;
		}

		for (auto decoder_output_buffer : cur_dec_det->_output_buf_list) {

			decltype(decoder_output_buffer->get_cache_ptr()) free_chunk_buffer;
			while (!(free_chunk_buffer = decoder_output_buffer->get_cache_ptr()))
			{
				BOOST_LOG_TRIVIAL(debug) << "waiting cache";
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

			auto need_free_bytes = total_bytes_to_write - static_cast<size_type>((*free_chunk_buffer)->second.reserve());
			if (need_free_bytes > 0) {
				(*free_chunk_buffer)->second.set_capacity((*free_chunk_buffer)->second.capacity() + static_cast<std::size_t>(need_free_bytes));
				// BOOST_LOG_TRIVIAL(debug) << "enlarging buffer with: " << need_free_bytes;
			}
			cur_dec_det->_current_samples_written += frame->header.blocksize;
			for (int i = 0; i != frame->header.blocksize; ++i) {
				for (int channel = 0; channel != cur_dec_det->_sound_details._channels; ++channel) {
					float_int32_bytes data;
					data.ival = buffer[channel][i];
					(this->*push_call)(*free_chunk_buffer, data, sample_width);
				}
			}

			decoder_output_buffer->put_cache_ptr(
				// is this the very very last data
				(cur_dec_det->_current_samples_written == cur_dec_det->_sound_details._total_samples) ||
				(*free_chunk_buffer)->second.reserve() < one_sample_to_byte
			);
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	void decoder_plugin_flac::error_callback(
		const FLAC__StreamDecoder * /*decoder*/,
		FLAC__StreamDecoderErrorStatus status,
		void * /*client_data*/)
	{
		auto pdecoder = _decoders.get_from_cache(_decoder_plugins_manager->get_current_decoder_details()->_sound_details._url_id).get();

		BOOST_LOG_TRIVIAL(error) << "flac error:" <<
			to_underlying(status) << " error: " <<
			FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(pdecoder)];
	}

	void decoder_plugin_flac::metadata_callback(
		const FLAC__StreamDecoder * /*decoder*/,
		const FLAC__StreamMetadata *metadata,
		void * /*client_data*/)
	{
		if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
			auto & current_decoder_dets = _decoder_plugins_manager->get_current_decoder_details_ref();
			auto bits = get_bit_count(metadata->data.stream_info.bits_per_sample);
			current_decoder_dets->_sound_details._ok = true;
			current_decoder_dets->_sound_details._total_samples = metadata->data.stream_info.total_samples;
			current_decoder_dets->_sound_details._current_samples_written_to_sound_buffer = 0;
			current_decoder_dets->_sound_details._channels = metadata->data.stream_info.channels;
			current_decoder_dets->_sound_details._is_float = false;
			current_decoder_dets->_sound_details._bps = (bits == 24 ? 32 : bits);
			current_decoder_dets->_sound_details._orig_bps = metadata->data.stream_info.bits_per_sample;
			current_decoder_dets->_sound_details._sample_rate = metadata->data.stream_info.sample_rate;
			current_decoder_dets->_sound_details._decoder_set_output_buffer_callback = _decoder_plugins_manager->set_output_buf_callback();
			current_decoder_dets->_sound_details._decoder_play_finished_callback = _play_finished_callback;

			current_decoder_dets->_current_samples_written = 0;

			BOOST_LOG_TRIVIAL(debug) <<
				"total samples: " << current_decoder_dets->_sound_details._total_samples <<
				" sample rate: " << current_decoder_dets->_sound_details._sample_rate <<
				" channels: " << current_decoder_dets->_sound_details._channels <<
				" bps: " << current_decoder_dets->_sound_details._orig_bps;


			_decoder_plugins_manager->decoder_opened(current_decoder_dets->_sound_details);
		}
	}

} // namespace mprt

// Factory method. Returns *simple pointer*!
std::unique_ptr<refcounting_plugin_api> create() {
	return std::make_unique<mprt::decoder_plugin_flac>();
}

BOOST_DLL_ALIAS(create, create_refc_plugin)

//MinGW related workaround
#define BOOST_DLL_FORCE_ALIAS_INSTANTIATION

#include <cmath>
#include <iostream>

#include <boost/dll/runtime_symbol_info.hpp>

#include "core/config.h"

#include "common/job_type_enums.h"
#include "common/input_plugin_api.h"
#include "common/output_plugin_api.h"
#include "common/scope_exit.h"

#include "core/decoder_plugins_manager.h"

#include "decoder_plugin_ffmpeg.h"
#include "common/scope_exit.h"

namespace mprt {

	extern "C"
	{
		static int read_callback_C(void* opaque, uint8_t* buffer, int bufferSize)
		{
			decoder_plugin_ffmpeg *decoder_ffmpeg = reinterpret_cast<decoder_plugin_ffmpeg*>(opaque);
			return decoder_ffmpeg->read_callback(opaque, (buffer_elem_t*)buffer, bufferSize);
		}
		
		static size_type seek_callback_C(void* opaque, size_type offset, int whence)
		{
			decoder_plugin_ffmpeg *decoder_ffmpeg = reinterpret_cast<decoder_plugin_ffmpeg*>(opaque);
			return decoder_ffmpeg->seek_callback(opaque, offset, whence);
		}
	}
	
	decoder_plugin_ffmpeg::decoder_plugin_ffmpeg()
		: _decoders(0)
		, _decoded_frame(
			std::unique_ptr<AVFrame, void(*)(AVFrame*)>(
				av_frame_alloc(),
				[](AVFrame *decoded_frame)
				{
					av_frame_free(&decoded_frame);
					decoded_frame = nullptr;
				}))
	{
	}

	decoder_plugin_ffmpeg::~decoder_plugin_ffmpeg()
	{
		BOOST_LOG_TRIVIAL(debug) << "decoder_plugin_ffmpeg::~decoder_plugin_ffmpeg() called";

		av_packet_unref(&_packet);
	}

	// Must be instantiated in plugin
	boost::filesystem::path decoder_plugin_ffmpeg::location() const
	{
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string decoder_plugin_ffmpeg::plugin_name() const
	{
		return "decoder_plugin_ffmpeg";
	}

	plugin_types decoder_plugin_ffmpeg::plugin_type() const
	{
		return plugin_types::decoder_plugin;
	}

	void decoder_plugin_ffmpeg::init(void *)
	{
		config::instance().init("../config/config_decoder_plugin_ffmpeg.xml");
		auto pt = config::instance().get_ptree_node("mprt.decoder_plugin_ffmpeg");

		_max_chunk_read_size = pt.get<size_type>("max_chunk_read_size", 128) * 1024;
		_max_memory_size_per_file = pt.get<size_type>("max_memory_size_per_file", 16384) * 1024;
		_ffmpeg_buffer_size = pt.get<size_type>("ffmpeg_buffer_size", 4) * 1024;
		_ffmpeg_probe_size = pt.get<size_type>("ffmpeg_probe_size", 32) * 1024;
		_use_seek_file = pt.get<std::string>("use_seek_file", "true") == "true";
		_priority = pt.get<size_type>("priority", 1);

		auto file_extensions = pt.get_child("file_extensions");
		for (auto & file_extension : file_extensions)
		{
			_supported_extensions.push_back(std::regex(file_extension.second.data(),
				std::regex_constants::ECMAScript | std::regex_constants::icase));
		}

		_play_finished_callback =
			std::bind(
				&decoder_plugin_ffmpeg::play_finished_callback<ffmpeg_cache_man_t, finish_flac_dec_func_t>,
				this,
				std::ref(_decoders),
				[](ffmpeg_cache_man_t::cache_item_t /*decoder*/) {}, std::placeholders::_1);

		std::vector<unsigned char>(static_cast<std::size_t>(_ffmpeg_buffer_size), 0).swap(_ffmpeg_buffer);

		/*_decodedFrame = std::shared_ptr<AVFrame>(
			av_frame_alloc(),
			[](AVFrame * decoded_frame)
			{
				av_frame_free(&decoded_frame);
				decoded_frame = nullptr;
			});*/

		av_init_packet(&_packet);
	}

	void decoder_plugin_ffmpeg::init_api()
	{
		init_ffmpeg();
	}

	decoder_plugin_ffmpeg::ffmpeg_cache_man_t::cache_item_t decoder_plugin_ffmpeg::create_new_ffmpeg_decoder()
	{
		return std::make_shared<ffmpeg_details>();
	}

	bool decoder_plugin_ffmpeg::init_ffmpeg()
	{
		bool is_ok_cont = false;
		int err;
		auto & decoder_dets = _decoder_plugins_manager->get_current_decoder_details_ref();
		auto ffmpeg_decoder = _decoders.get_from_cache(std::bind(&decoder_plugin_ffmpeg::create_new_ffmpeg_decoder, this), decoder_dets->_sound_details._url_id);
		decoder_dets->_current_cache_buf = get_cache_put_buf(decoder_dets->_sound_details._url_id);
		decltype(decoder_dets->_current_cache_buf->get_data_ptr()) data_ptr = nullptr;
		
		while ((data_ptr = decoder_dets->_current_cache_buf->get_data_ptr()) == nullptr)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			BOOST_LOG_TRIVIAL(debug) << "waiting for the data to arrive";
		}

		SCOPE_EXIT_REF(
			decoder_dets->_sound_details._ok = is_ok_cont;
			if (is_ok_cont)
			{
				decoder_dets->_sound_details._total_samples = std::numeric_limits<size_type>::max();
				decoder_dets->_sound_details._current_samples_written_to_sound_buffer = 0;
				decoder_dets->_sound_details._channels = ffmpeg_decoder->formatContext->streams[ffmpeg_decoder->streamId]->codecpar->channels;
				decoder_dets->_sound_details._is_float =
					ffmpeg_decoder->codecContext->sample_fmt == AV_SAMPLE_FMT_FLTP ||
					ffmpeg_decoder->codecContext->sample_fmt == AV_SAMPLE_FMT_FLT;
				decoder_dets->_sound_details._bps = decoder_dets->_sound_details._is_float ? 8 * sizeof(float) :
					ffmpeg_decoder->codecContext->sample_fmt == AV_SAMPLE_FMT_S32 ? 32 : 16;
				decoder_dets->_sound_details._orig_bps = decoder_dets->_sound_details._bps;
				decoder_dets->_sound_details._sample_rate = ffmpeg_decoder->formatContext->streams[ffmpeg_decoder->streamId]->codecpar->sample_rate;
				decoder_dets->_sound_details._decoder_set_output_buffer_callback = _decoder_plugins_manager->set_output_buf_callback();
				decoder_dets->_sound_details._decoder_play_finished_callback = _play_finished_callback;

				decoder_dets->_current_samples_written = 0;

				_decoder_plugins_manager->decoder_opened(decoder_dets->_sound_details);
			}
			else
			{
				_play_finished_callback(decoder_dets->_sound_details._url_id);
			}
		);

		ffmpeg_decoder->ioContext = std::shared_ptr<AVIOContext>(
			avio_alloc_context(
				&_ffmpeg_buffer[0],
				static_cast<int>(_ffmpeg_buffer_size - AV_INPUT_BUFFER_PADDING_SIZE),
				0,
				this,
				read_callback_C,
				nullptr,
				seek_callback_C), &av_free);


		if (!ffmpeg_decoder->ioContext)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot create io context";
			return false;
		}

		ffmpeg_decoder->formatContext = std::shared_ptr<AVFormatContext>(
					avformat_alloc_context(), &avformat_free_context);

		if (!ffmpeg_decoder->formatContext)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot create format context";
			return false;
		}

		ffmpeg_decoder->formatContext->pb = ffmpeg_decoder->ioContext.get();
		ffmpeg_decoder->formatContext->flags |= AVFMT_FLAG_CUSTOM_IO | AVFMT_FLAG_GENPTS | AVFMT_FLAG_DISCARD_CORRUPT;

		AVProbeData probeData = { 0 };
		probeData.buf = (unsigned char*)(*data_ptr)->second.linearize();
		probeData.buf_size = std::min(static_cast<int>(_ffmpeg_probe_size), static_cast<int>((*data_ptr)->second.size()));
		probeData.filename = "";

		ffmpeg_decoder->formatContext->iformat = av_probe_input_format(&probeData, 1);
		if (!ffmpeg_decoder->formatContext->iformat)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot init formatContext->iformat";
			return false;
		}

		auto format_ctx_ptr = ffmpeg_decoder->formatContext.get();
		if ((err = avformat_open_input(std::addressof(format_ctx_ptr), ""/*decoder_dets->_url.c_str()*/, ffmpeg_decoder->formatContext->iformat, nullptr)) < 0)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin avformat_open_input error: " << ffmpeg_strerror(err);
			return false;
		}

		av_format_inject_global_side_data(ffmpeg_decoder->formatContext.get());

		if (avformat_find_stream_info(ffmpeg_decoder->formatContext.get(), nullptr) < 0)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin avformat_find_stream_info error";
			return false;
		}

		av_dump_format(ffmpeg_decoder->formatContext.get(), 0, "", 0);
			
		ffmpeg_decoder->streamId = av_find_best_stream(ffmpeg_decoder->formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);

		//for (unsigned i = 0; i < ffmpeg_decoder->formatContext->nb_streams; i++) {
		//	if (ffmpeg_decoder->formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
		//		ffmpeg_decoder->streamId = (int)i;
		//		break;
		//	}
		//}

		if (ffmpeg_decoder->streamId == -1) {
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot find streamId";
			return false;
		}

		AVCodec* pcodec = avcodec_find_decoder(ffmpeg_decoder->formatContext->streams[ffmpeg_decoder->streamId]->codecpar->codec_id);
		if (!pcodec) {
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot find avcodec_find_decoder";
			return false;
		}
		
		ffmpeg_decoder->codecContext = std::shared_ptr<AVCodecContext>(
			avcodec_alloc_context3(pcodec), [](AVCodecContext *pcodec_ctx)
		{
			avcodec_flush_buffers(pcodec_ctx);
			avcodec_close(pcodec_ctx);

			avcodec_free_context(&pcodec_ctx);
		});

		if (!ffmpeg_decoder->codecContext)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot do avcodec_alloc_context3";
			return false;
		}

		if (avcodec_parameters_to_context(ffmpeg_decoder->codecContext.get(), ffmpeg_decoder->formatContext->streams[ffmpeg_decoder->streamId]->codecpar) < 0)
		{
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot do avcodec_parameters_to_context";
			return false;
		}

		if (avcodec_open2(ffmpeg_decoder->codecContext.get(), pcodec, nullptr) < 0) {
			BOOST_LOG_TRIVIAL(error) << "ffmpeg plugin cannot avcodec_open2";
			return false;
		}

		/*
		AVDictionaryEntry *tag = NULL;
		while ((tag = av_dict_get(pffmpeg_decoder->formatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
			printf("%s=%s\n", tag->key, tag->value);
			*/
		auto dur =
			double(ffmpeg_decoder->formatContext->streams[ffmpeg_decoder->streamId]->duration)
			* av_q2d(ffmpeg_decoder->formatContext->streams[ffmpeg_decoder->streamId]->time_base);
		BOOST_LOG_TRIVIAL(debug) << "file duration: " << dur << "(s)";

		is_ok_cont = true;

		return true;
	}

	void decoder_plugin_ffmpeg::close_ffmpeg_details(ffmpeg_details* pffmpeg_details)
	{
	}

	std::string decoder_plugin_ffmpeg::ffmpeg_strerror(int errnum)
	{
		char result[256];

		av_strerror(errnum, result, 256);
		result[255] = 0;

		return std::string(result);
	}



	void decoder_plugin_ffmpeg::decode_new()
	{
		auto & cur_dec_det = _decoder_plugins_manager->get_current_decoder_details_ref();
		auto pdecoder = _decoders.get_from_cache(cur_dec_det->_sound_details._url_id).get();

		auto ret = avcodec_send_packet(pdecoder->codecContext.get(), &_packet);
		//BOOST_LOG_TRIVIAL(debug) << "packet size: " << _packet.size;
		if (ret < 0) {
			BOOST_LOG_TRIVIAL(error) << "Error submitting the packet to the decoder: " << ffmpeg_strerror(ret);
		}

		while ((ret = avcodec_receive_frame(pdecoder->codecContext.get(), _decoded_frame.get())) != AVERROR_EOF) {
			//BOOST_LOG_TRIVIAL(debug) << "received frame";
			if (ret == AVERROR(EAGAIN))
			{
				//BOOST_LOG_TRIVIAL(debug) << "receive frame error: " << ffmpeg_strerror(ret);
				//std::this_thread::sleep_for(std::chrono::milliseconds(100));
				return;
			}
			else if (ret < 0)
			{
				BOOST_LOG_TRIVIAL(debug) << "receive frame error: " << ffmpeg_strerror(ret);
				return;
			}

			int samples = _decoded_frame->nb_samples;
			int channels = pdecoder->codecContext->channels;

			auto one_sample_to_byte = samples_to_bytes(1, cur_dec_det->_sound_details);
			auto total_bytes_to_write = one_sample_to_byte * samples;
			auto sample_width = av_get_bytes_per_sample(pdecoder->codecContext->sample_fmt);
			cur_dec_det->_current_samples_written += samples;
			for (auto decoder_output_buffer : cur_dec_det->_output_buf_list) {

				decltype(decoder_output_buffer->get_cache_ptr()) free_chunk_buffer;
				while (!(free_chunk_buffer = decoder_output_buffer->get_cache_ptr()))
				{
					BOOST_LOG_TRIVIAL(debug) << "waiting cache";
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				//BOOST_LOG_TRIVIAL(debug) << "cont cache";


				auto need_free_bytes = total_bytes_to_write - static_cast<size_type>((*free_chunk_buffer)->second.reserve());
				if (need_free_bytes > 0) {
					(*free_chunk_buffer)->second.set_capacity((*free_chunk_buffer)->second.capacity() + static_cast<std::size_t>(need_free_bytes));
					// BOOST_LOG_TRIVIAL(debug) << "enlarging buffer with: " << need_free_bytes;
				}
				bool is_planar = av_sample_fmt_is_planar(pdecoder->codecContext->sample_fmt);
				if (is_planar) {
					for (int i = 0; i < samples; i += 1) {
						for (int channel = 0; channel < cur_dec_det->_sound_details._channels; channel += 1)
						{
							float_int32_bytes samplex;
							for (int j = 0; j != sample_width; ++j)
							{
								samplex.bytes[j] = *(_decoded_frame->extended_data[channel] + i * sample_width + j);
							}
							//_sound_details._is_float ? 
							//	samplex.fval = *(float*)(_decodedFrame->extended_data[ch] + i * sample_width) : 
							//	samplex.ival = *(int32_t*)(_decodedFrame->extended_data[ch] + i * sample_width);
							//(this->*push_call)((*free_chunk_buffer), samplex);
							push_func_call(*free_chunk_buffer, samplex, sample_width);
						}
					}
				}
				else
				{
					//(*free_chunk_buffer)->second.resize((*free_chunk_buffer)->second.size() + static_cast<std::size_t>(total_bytes_to_write));
					//std::memcpy((*free_chunk_buffer)->second.linearize(), _decodedFrame->extended_data[0], static_cast<std::size_t>(total_bytes_to_write));

					for (int i = 0; i != samples; ++i) {
						for (int channel = 0; channel != cur_dec_det->_sound_details._channels; ++channel) {
							float_int32_bytes samplex;
							for (int j = 0; j != sample_width; ++j)
							{
								samplex.bytes[j] = *(_decoded_frame->extended_data[0] + (2 * i + channel) * sample_width + j);
							}
							//samplex.ival = *(uint32_t*)(_decodedFrame->extended_data[0] + (2 * i + channel) * sample_width);
							//samplex.fval = *(float*)(_decodedFrame->extended_data[0] + (2 * i + channel) * sample_width);
							push_func_call(*free_chunk_buffer, samplex, sample_width);
						}
					}
				}

				decoder_output_buffer->put_cache_ptr(
					// is this the very very last data
					(cur_dec_det->_current_samples_written == cur_dec_det->_sound_details._total_samples) ||
					(*free_chunk_buffer)->second.reserve() < one_sample_to_byte
				);
			}
		}
	}

	void decoder_plugin_ffmpeg::decode()
	{
		auto cur_det = _decoder_plugins_manager->get_current_decoder_details();

		auto pdecoder = _decoders.get_from_cache(cur_det->_sound_details._url_id).get();
		if (!pdecoder)
		{
			return;
		}

		int retx = av_read_frame(pdecoder->formatContext.get(), &_packet);
		if (AVERROR_EOF == retx) {
			BOOST_LOG_TRIVIAL(debug) << "finish read ...";
			_packet.buf = nullptr;
			_packet.size = 0;
			cur_det->_current_stream_pos = cur_det->_stream_length;
		}
		else if (0 != retx) {
			BOOST_LOG_TRIVIAL(debug) << "ffmpeg av_read_frame error retx: " << retx << " desc: " << ffmpeg_strerror(retx);
			//_decoder_plugins_manager->add_job([this] { decode(); }, std::chrono::milliseconds(1000));
			return;
		}
		
		decode_new();

		if (AVERROR_EOF == retx)
		{
			_decoder_plugins_manager->finish_decode_internal_single(cur_det->_sound_details._url_id);
		}

		av_packet_unref(& _packet);
	}

	void decoder_plugin_ffmpeg::reset_buffers()
	{
		decoder_plugin_api::reset_buffers();
	}


	// consumer of the input
	int decoder_plugin_ffmpeg::read_callback(void* /*opaque*/, buffer_elem_t* buffer, int bufferSize)
	{
		auto read_res = _decoder_plugins_manager->decoder_read_buffer(reinterpret_cast<buffer_elem_t*>(buffer), bufferSize);

		if (-1 == read_res.first) {
			BOOST_LOG_TRIVIAL(error) << "read_bytes problem";
			return AVERROR_EOF;
		}
		else if (0 == read_res.first)
		{
			if (_decoder_plugins_manager->decoder_is_exhausted()) {
				BOOST_LOG_TRIVIAL(debug) << "stream exhausted";
				return AVERROR_EOF;
			}
			else if (_decoder_plugins_manager->decoder_is_last_read_empty()) {
				return 0; // try another time
			}
		}

		//if (bufferSize != read_res.first)
		//{
		//	BOOST_LOG_TRIVIAL(debug) << "READ ERROR: " << bufferSize << " real_read: " << read_res.first;
		//}

		return static_cast<int>(read_res.first);
	}

	size_type decoder_plugin_ffmpeg::seek_callback(void* /*opaque*/, size_type offset, int whence)
	{
		
		//std::this_thread::sleep_for(std::chrono::seconds(1));
		BOOST_LOG_TRIVIAL(debug) << "seek called with: " << offset << " whence: " << whence;
		auto cur_det = _decoder_plugins_manager->get_current_decoder_details();
		size_type absolute_pos;
		switch (whence) {
		case AVSEEK_SIZE:
			return cur_det->_stream_length;
			break;
		case SEEK_SET:
			absolute_pos = offset;
			break;
		case SEEK_CUR:
			absolute_pos = cur_det->_current_stream_pos + offset;
			break;
		case SEEK_END:
			absolute_pos = cur_det->_stream_length - offset;
			break;
		}

		if ((absolute_pos < 0) || (absolute_pos > cur_det->_stream_length))
			return -1;

		return _decoder_plugins_manager->seek_byte_internal(cur_det->_sound_details._url_id, absolute_pos) ? absolute_pos : -1;
	}

	void decoder_plugin_ffmpeg::seek_duration(size_type duration_ms)
	{
		auto & cur_dec_det = _decoder_plugins_manager->get_current_decoder_details_ref();
		auto pdecoder = _decoders.get_from_cache(cur_dec_det->_sound_details._url_id).get();
		if(!pdecoder)
		{
			return;
		}

		//size_type seekTime = av_rescale_q(seekTimeDS, ds_time_base, m_pFormatContext->streams[seekStreamIndex]->time_base);
		//size_type seekStreamDuration = m_pFormatContext->streams[seekStreamIndex]->duration;

		//int flags = AVSEEK_FLAG_BACKWARD;
		//if (seekTime > 0 && seekTime < seekStreamDuration)
		//	flags |= AVSEEK_FLAG_ANY; // H.264 I frames don't always register as "key frames" in FFmpeg

		//int ret = av_seek_frame(m_pFormatContext, seekStreamIndex, seekTime, flags);
		//if (ret < 0)

		double file_duration_ms =
			double(pdecoder->formatContext->streams[pdecoder->streamId]->duration)
			* av_q2d(pdecoder->formatContext->streams[pdecoder->streamId]->time_base) * 1000;

		if (duration_ms > 0 && double(duration_ms) < file_duration_ms)
		{
			/*
			int defaultStreamIndex = av_find_default_stream_index(m_pFormatContext);
		int seekStreamIndex = (m_videoStreamIndex != -1)? m_videoStreamIndex : defaultStreamIndex;
		__int64 seekTime = av_rescale_q(seekTimeDS, ds_time_base, m_pFormatContext->streams[seekStreamIndex]->time_base);
		__int64 seekStreamDuration = m_pFormatContext->streams[seekStreamIndex]->duration;

		int flags = AVSEEK_FLAG_BACKWARD;
		if (seekTime > 0 && seekTime < seekStreamDuration)
			flags |= AVSEEK_FLAG_ANY; // H.264 I frames don't always register as "key frames" in FFmpeg

		int ret = av_seek_frame(m_pFormatContext, seekStreamIndex, seekTime, flags);
		if (ret < 0)
			ret = av_seek_frame(m_pFormatContext, seekStreamIndex, seekTime, AVSEEK_FLAG_ANY);*/
			AVRational scale_ms;
			scale_ms.den = 1000;
			scale_ms.num = 1;
			auto seek_time = av_rescale_q(duration_ms, scale_ms, pdecoder->formatContext->streams[pdecoder->streamId]->time_base);
			auto file_duration_ffmpeg_time_base = std::llround(file_duration_ms);
			//auto request_timestamp = std::llround(double(duration_ms) / 1000);
			auto result =
				_use_seek_file ?
				avformat_seek_file(pdecoder->formatContext.get(),
					pdecoder->streamId,	0, seek_time, pdecoder->formatContext->streams[pdecoder->streamId]->duration, AVSEEK_FLAG_ANY)
				:
				av_seek_frame(pdecoder->formatContext.get(), pdecoder->streamId, seek_time, AVSEEK_FLAG_ANY);
			if (result < 0)
			{
				BOOST_LOG_TRIVIAL(debug) << "ffmpeg seek error: " << ffmpeg_strerror(result);
			}
			avcodec_flush_buffers(pdecoder->codecContext.get());
		}

	}

	void decoder_plugin_ffmpeg::init_decode_internal_single(url_id_t url_id)
	{
		init_decode_internal_single_internal(_decoders, url_id);
	}

} // namespace mprt

// Factory method. Returns *simple pointer*!
std::unique_ptr<refcounting_plugin_api> create() {
	return std::make_unique<mprt::decoder_plugin_ffmpeg>() ;
}

BOOST_DLL_ALIAS(create, create_refc_plugin)

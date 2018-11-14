//MinGW related workaround
#define BOOST_DLL_FORCE_ALIAS_INSTANTIATION

#include <thread>
#include <memory>

#include <boost/filesystem/path.hpp>
#include <boost/circular_buffer_fwd.hpp>

#include "common/decoder_plugin_api.h"
#include "common/producerconsumerqueue.h"
#include "common/type_defs.h"
#include "common/cache_manage.h"

extern "C" {
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
//#include <libavutil/opt.h>
}

namespace mprt {

	struct ffmpeg_details
	{
		std::shared_ptr<AVIOContext> ioContext;
		std::shared_ptr<AVFormatContext> formatContext;
		std::shared_ptr<AVCodecContext> codecContext;
		int streamId;
		
		ffmpeg_details()
			: ioContext(nullptr)
			, formatContext(nullptr)
			, codecContext(nullptr)
			, streamId(-1)
		{}
	};

	class decoder_plugin_ffmpeg : public decoder_plugin_api {
	private:
		using ffmpeg_cache_man_t = cache_manage<std::shared_ptr<ffmpeg_details>>;
		using finish_flac_dec_func_t = std::function<void (ffmpeg_cache_man_t::cache_item_t decoder)>;
		

		AVPacket _packet;
		std::unique_ptr<AVFrame, void(*)(AVFrame*)> _decoded_frame;
		size_type _ffmpeg_buffer_size;
		size_type _ffmpeg_probe_size;
		std::vector<unsigned char> _ffmpeg_buffer;
		bool _use_seek_file;

		ffmpeg_cache_man_t _decoders;

		ffmpeg_cache_man_t::cache_item_t create_new_ffmpeg_decoder();

		bool init_ffmpeg();

		virtual void reset_buffers() override;

		void init_decode_internal_single(url_id_t url_id) override;

		void close_ffmpeg_details(ffmpeg_details *pffmpeg_details);

		std::string ffmpeg_strerror(int errnum);
		void decode_new();



	public:
		decoder_plugin_ffmpeg();

		virtual ~decoder_plugin_ffmpeg();

		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void * arguments = nullptr) override;

		virtual void decode() override;
		virtual void init_api() override;

		// ffmpeg specific functions
		int read_callback(void* opaque, buffer_elem_t* buffer, int bufferSize);
		size_type seek_callback(void* opaque, size_type offset, int whence);

		virtual void seek_duration(size_type duration_ms) override;

	};

} // namespace mprt

//MinGW related workaround
#define BOOST_DLL_FORCE_ALIAS_INSTANTIATION

#include <thread>

#include <boost/filesystem/path.hpp>
#include <boost/circular_buffer_fwd.hpp>

extern "C"
{
#include <FLAC/stream_decoder.h>
}

#include "common/decoder_plugin_api.h"
#include "common/producerconsumerqueue.h"
#include "common/type_defs.h"
#include "common/cache_manage.h"

namespace mprt {

	class decoder_plugin_flac : public decoder_plugin_api {
	private:
		using flac_cache_man_t = cache_manage<std::shared_ptr<FLAC__StreamDecoder>>;
		using finish_flac_dec_func_t = std::function<void (flac_cache_man_t::cache_item_t decoder)>;
		bool _check_md5;
		flac_cache_man_t _decoders;
		finish_flac_dec_func_t _finish_flac_dec_func;

		flac_cache_man_t::cache_item_t create_new_flac_decoder();
		void finish_flac_decoder(flac_cache_man_t::cache_item_t decoder);

		bool init_flac();

		virtual void reset_buffers() override;

		void init_decode_internal_single(url_id_t url_id) override;


		void seek_internal(size_type msecs);

	public:
		decoder_plugin_flac();

		virtual ~decoder_plugin_flac();

		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void * arguments = nullptr) override;

		virtual void init_api() override;
		virtual void decode() override;

		virtual void seek_duration(size_type duration_ms) override;

		// flac specific functions
		FLAC__StreamDecoderReadStatus read_callback(
			const FLAC__StreamDecoder * /*decoder*/,
			FLAC__byte buffer[],
			size_t *bytes,
			void * /*client_data*/);

		FLAC__StreamDecoderSeekStatus seek_callback(
			const FLAC__StreamDecoder * /*decoder*/,
			FLAC__uint64 absolute_byte_offset,
			void * /*client_data*/);

		FLAC__StreamDecoderTellStatus tell_callback(
			const FLAC__StreamDecoder * /*decoder*/,
			FLAC__uint64 *absolute_byte_offset,
			void * /*client_data*/);

		FLAC__StreamDecoderLengthStatus length_callback(
			const FLAC__StreamDecoder * /*decoder*/,
			FLAC__uint64 *stream_length,
			void * /*client_data*/);

		FLAC__bool eof_callback(
			const FLAC__StreamDecoder * /*decoder*/,
			void * /*client_data*/);

		FLAC__StreamDecoderWriteStatus write_callback(
			const FLAC__StreamDecoder *decoder,
			const FLAC__Frame *frame,
			const FLAC__int32 * const buffer[],
			void *client_data);

		void error_callback(
			const FLAC__StreamDecoder *decoder,
			FLAC__StreamDecoderErrorStatus status,
			void *client_data);

		void metadata_callback(const FLAC__StreamDecoder *decoder,
			const FLAC__StreamMetadata *metadata,
			void *client_data);

	};

} // namespace mprt

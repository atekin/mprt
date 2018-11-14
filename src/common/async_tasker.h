#ifndef async_tasker_h__
#define async_tasker_h__

#include <memory>
#include <thread>
#include <queue>
#include <unordered_set>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

#include "type_defs.h"

namespace mprt
{
	class async_tasker {
	public:
		using timer_type = boost::asio::steady_timer;
		using timer_type_shared = std::shared_ptr<timer_type>;

	private:
		boost::asio::io_context _io;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _work_guard;
		std::unordered_set<timer_type_shared> _active_timers;
		std::queue<timer_type_shared> _expired_timers;
		size_type _max_timer_count;
		std::atomic_bool _ready;
		std::thread _async_task;

		void quit() {
			boost::asio::post(
				_io,
				[this]() {
				// release the work guard
				_work_guard.reset();

				BOOST_LOG_TRIVIAL(trace) << "canceling all the timers";

				// cancel all the timers
				for (auto active_timer : _active_timers) {
					active_timer->expires_at(timer_type::clock_type::time_point::min());
				}
			});

			boost::asio::post(
				_io,
				[this]() {
				_io.stop();
			});
		}

		void process_events() {
			_ready = true;

			for (int try_count = 0; try_count != 100; ++try_count) {
				try
				{
					_io.run();

					// since we are here we exited normally
					break;
				}
				catch (std::exception const &e)
				{
					BOOST_LOG_TRIVIAL(error) << "boost::asio::io_context::run() error: " << e.what();
				}
				catch (...)
				{
					BOOST_LOG_TRIVIAL(error) << "boost::asio::io_context::run() produced an unknown error";
				}

			}
		}

		template<typename Func>
		void handle_timeout(const boost::system::error_code& ec, timer_type_shared timer_instance, Func f) {

			_active_timers.erase(timer_instance);
			if (_expired_timers.size() < _max_timer_count) {
				_expired_timers.emplace(timer_instance);
			}

			if (!ec) {
				if (timer_instance->expires_at() != timer_type::time_point::min()) {
					// timer ended gracefully
					// manage the timer
					//BOOST_LOG_TRIVIAL(trace) << "handle_timeout: timer timer ended gracefully" << timer_instance;

				}
				else {
					BOOST_LOG_TRIVIAL(trace) << "handle_timeout: timer shutdown" << timer_instance;
				}
			}
			else /*if (ec != boost::asio::error::operation_aborted)*/ {
				BOOST_LOG_TRIVIAL(trace) << "handle_timeout error " << ec.message() << " timer: " << timer_instance;
			}

			// post the job
			f();
		}

		template<typename Func>
		timer_type_shared create_get_timer(Func f, timer_type::duration dur) {
			timer_type_shared timer;

			if (_expired_timers.empty()) {
				timer = std::make_shared<timer_type>(_io);
				//BOOST_LOG_TRIVIAL(debug) << "creating timer: " << timer;
			}
			else {
				timer = _expired_timers.front();
				_expired_timers.pop();
				//BOOST_LOG_TRIVIAL(debug) << "getting timer: " << timer;
			}

			timer->expires_after(dur);
			timer->async_wait(std::bind(&async_tasker::handle_timeout<Func>, this, std::placeholders::_1, timer, f));
			_active_timers.insert(timer);

			return timer;
		}

	public:
		async_tasker(size_t max_timer_count)
			: _io(1) // @TODO: will be checked later ... https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/overview/core/concurrency_hint.html
			, _work_guard(boost::asio::make_work_guard(_io)) // needed for _io.run not to return
			, _max_timer_count(max_timer_count)
			, _ready(false)
			, _async_task(std::thread(&async_tasker::process_events, this))
		{

		}

		bool is_ready() {
			return _ready;
		}

		template<typename Func>
		timer_type_shared add_async_job_thread_internal(Func f, timer_type::duration dur) {
			return create_get_timer<Func>(f, dur);
		}

		template<typename Func>
		void add_async_job(Func f) {
			// we need to post this, since otherwise race condition would occur
			//boost::asio::post(_io, [this, f]() { create_get_timer<Func>(f, std::chrono::nanoseconds(0)); });
			boost::asio::post(_io, [f]() { f(); });
		}

		template<typename Func>
		void add_async_job(Func f, timer_type::duration dur) {
			// we need to post this, since otherwise race condition would occur
			boost::asio::post(_io, [this, f, dur]() { create_get_timer<Func>(f, dur); });
		}

		bool is_timer_expired(timer_type_shared const& timer)
		{
			return !is_active_timer(timer);
		}

		bool is_active_timer(timer_type_shared const& timer)
		{
			return _active_timers.find(timer) != _active_timers.cend();
		}

		~async_tasker() {
			quit();

			if (_async_task.joinable()) {
				_async_task.join();
			}
		}
	};
}

#endif // async_tasker_h__

#ifndef async_task_h__
#define async_task_h__

#include <type_traits>

#include "common/async_tasker.h"
#include "common/job_type_enums.h"

#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/empty.hpp>

#define _STATE_CHECK_MACRO_1(_arg_) if (_arg_ != _current_state)  return
#define _STATE_CHECK_MACRO_2(_arg1_, _arg2_) if (_arg1_ != _current_state) return _arg2_

#if !BOOST_PP_VARIADICS_MSVC
#define _STATE_CHECK_(...) BOOST_PP_OVERLOAD(_STATE_CHECK_MACRO_,__VA_ARGS__)(__VA_ARGS__)
#else // or for Visual C++
#define _STATE_CHECK_(...) \
  BOOST_PP_CAT(BOOST_PP_OVERLOAD(_STATE_CHECK_MACRO_,__VA_ARGS__)(__VA_ARGS__),BOOST_PP_EMPTY())
#endif

namespace mprt
{
	class async_task
	{
	protected:

		std::shared_ptr<async_tasker> _async_task;

		virtual void stop_internal() = 0;
		virtual void pause_internal() = 0;
		virtual void cont_internal() = 0;
		virtual void quit_internal() = 0;

	public:
		plugin_states _current_state;

		template <typename Func>
		void add_job(Func f) {
			_async_task->add_async_job(f);
		}

		// invokes the job at specified time later ...
		template <typename Func>
		void add_job(Func f, async_tasker::timer_type::duration dur) {
			return _async_task->add_async_job(f, dur);
		}

		// invokes the job at specified time later ...
		template <typename Func>
		async_tasker::timer_type_shared add_job_thread_internal(Func f, async_tasker::timer_type::duration dur) {
			return _async_task->add_async_job_thread_internal(f, dur);
		}

		bool is_timer_expired(async_tasker::timer_type_shared const& timer)
		{
			return _async_task->is_timer_expired(timer);
		}

		bool is_active_timer(async_tasker::timer_type_shared const& timer)
		{
			return _async_task->is_active_timer(timer);
		}

		virtual void stop()
		{
			BOOST_LOG_TRIVIAL(debug) << " called stop";

			add_job([this] {
				_current_state = plugin_states::stop;
				stop_internal();
			});
		}

		virtual void pause()
		{
			BOOST_LOG_TRIVIAL(debug) << " called pause";

			add_job([this] {
				_current_state = plugin_states::pause;
				pause_internal();
			});
		}

		virtual void cont()
		{
			BOOST_LOG_TRIVIAL(debug) << " called cont";

			add_job([this] {
				_current_state = plugin_states::play;

				cont_internal();
			});
		}

		virtual void quit()
		{
			BOOST_LOG_TRIVIAL(debug) << " called quit";

			add_job([this] {
				_current_state = plugin_states::quit;
				quit_internal();
			});

		}
	};
}


#endif // async_task_h__

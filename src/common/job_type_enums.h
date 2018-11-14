#ifndef job_type_enums_h__
#define job_type_enums_h__

enum class plugin_states
{
	stop,
	pause,
	play,
	quit,

	FIRST_ITEM = stop,
	LAST_ITEM = quit
};

#endif // job_type_enums_h__

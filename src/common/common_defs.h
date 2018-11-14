#ifndef common_defs_h__
#define common_defs_h__

#include <stdint.h>

struct uninitialized_char {
	unsigned char m;
	uninitialized_char() {}
};

using size_type = int64_t;
//using buffer_elem_t = uninitialized_char;
using buffer_elem_t = unsigned char;
using url_id_t = size_type;

#endif // common_defs_h__

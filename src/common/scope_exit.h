#ifndef scope_exit_h__
#define scope_exit_h__
// see http://the-witness.net/news/2012/11/scopeexit-in-c11/


template <typename F>
struct scope_exit {
	scope_exit(F f) : f(f) {}
	~scope_exit() { f(); }
	F f;
};

template <typename F>
scope_exit<F> make_scope_exit(F f) {
	return scope_exit<F>(f);
};

#define _SCOPE_EXIT_STRING_JOIN2_(arg1, arg2) _SCOPE_EXIT_DO_STRING_JOIN2_(arg1, arg2)
#define _SCOPE_EXIT_DO_STRING_JOIN2_(arg1, arg2) arg1 ## arg2

#define SCOPE_EXIT_VAL(code) \
    auto _SCOPE_EXIT_STRING_JOIN2_(scope_exit_, __LINE__) = make_scope_exit([=](){code;})

#define SCOPE_EXIT_REF(code) \
    auto _SCOPE_EXIT_STRING_JOIN2_(scope_exit_, __LINE__) = make_scope_exit([&](){code;})



#endif // scope_exit_h__

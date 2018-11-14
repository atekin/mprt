#ifndef singleton_h__
#define singleton_h__

namespace mprt {
template<typename T>
class singleton {
public:
	static T& instance();

	singleton(const singleton&) = delete;
	singleton& operator= (const singleton) = delete;

protected:
	struct token {};
	singleton() {}
};

template<typename T>
T& singleton<T>::instance()
{
	static T instance{ token{} };
	return instance;
}
}

#endif // singleton_h__

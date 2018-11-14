#ifndef utils_h__
#define utils_h__

#include <chrono>
#include <tuple>
#include <string>
#include <iomanip>

inline bool is_big_endian(void)
{
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01020304 };

	return bint.c[0] == 1;
}

template <typename T>
T bound_val(T const& min_val, T const& val, T const& max_val)
{
	return std::max<T>(min_val, std::min<T>(val, max_val));
}

template <typename Q>
void clear_queue(Q & q) {
	Q empty;
	std::swap(q, empty);
}

template <typename T>
void delete_ptr(T *p)
{
	if (p)
	{
		delete p;
		p = nullptr;
	}
}

template <typename T, typename Func>
void delete_ptr(T *p, Func f)
{
	if (p)
	{
		f(p);
		p = nullptr;
	}
}

template <typename T, typename Func>
void delete_dbl_ptr(T *p, Func f)
{
	if (p)
	{
		f(&p);
		p = nullptr;
	}
}


template <typename T>
void delete_arr(T *p)
{
	if (p)
	{
		delete[]p;
		p = nullptr;
	}
}

template<typename T>
T& string_ltrim(T& str, const T& chars = "\t\n\v\f\r ")
{
	str.erase(0, str.find_first_not_of(chars));
	return str;
}

template<typename T>
T& string_rtrim(T& str, const T& chars = "\t\n\v\f\r ")
{
	str.erase(str.find_last_not_of(chars) + 1);
	return str;
}

template<typename T>
T& string_trim(T& str, const T& chars = "\t\n\v\f\r ")
{
	return string_ltrim(string_rtrim(str, chars), chars);
}

template<typename T>
T get_ext(T const& url_name, T const& sep_char = ".")
{
	T file_ext;

	auto found = url_name.find_last_of(sep_char);
	if (found != T::npos)
		file_ext = url_name.substr(found + 1);

	return file_ext;
}

template<typename T>
T get_pair_cantor(T const& a, T const& b)
{
	return ((a + b) * (a + b + 1) / 2 + b);
}

template <typename Container, typename Fun>
void tuple_for_each(const Container& c, Fun fun)
{
	for (auto& e : c) {
		fun(std::get<0>(e), std::get<1>(e), std::get<2>(e));
	}
}

template <typename T>
std::string create_time_format_string(std::chrono::milliseconds time)
{
	using namespace std::chrono;

	using tup_t = std::tuple<milliseconds, int, const char *>;

	constexpr tup_t formats[] = {
		tup_t{ hours(1), 2, "" },
		tup_t{ minutes(1), 2, ":" },
		tup_t{ seconds(1), 2, ":" },
		tup_t{ milliseconds(1), 3, "." }
	};

	T o;
	tuple_for_each(formats, [&time, &o](auto denominator, auto width, auto separator) {
		o << separator << std::setw(width) << std::setfill('0') << (time / denominator);
		time = time % denominator;
	});
	auto res = o.str();
	if (res.compare(0, 3, "00:") == 0)
	{
		res.erase(0, 3);
	}
	return res;
}

#endif // utils_h__

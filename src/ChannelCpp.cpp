//============================================================================
// Name        : ChannelCpp.cpp
// Author      : Ankit Gupta
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <NewOrderMessage.hpp>

#include <boost/fusion/mpl.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/functional.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/define_struct_inline.hpp>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>

#include <sstream>
#include <array>
#include <boost/array.hpp>
#include <tuple>
#include <cstddef>
#include <type_traits>
#include <boost/optional.hpp>
#include <boost/utility/identity_type.hpp>
#include <vector>
#include <unordered_map>
#include <boost/foreach.hpp>

#define COMMA ,

namespace example {
// Optional Type Data
template<typename T, size_t N>
struct optional_field: boost::optional<T> {
	static const size_t bit = N;
};

//template<int N>
//size_t least_significant_bit(const std::bitset<N> &bt) {
//	for (size_t i = 0; i < bt.size(); ++i) {
//		bt.flip(i);
//	}
//}

template<typename T, size_t N = sizeof(T) * CHAR_BIT>
struct optional_field_set {
//	using value_type = T;
//	using bits_type = std::bitset<N>;
	typedef T value_type;
	typedef std::bitset<N> bits_type;
};

//using opt_fields = optional_field_set<uint16_t>;
typedef optional_field_set<uint16_t> opt_fields;

}

// Reader Adaptor
struct reader {
	mutable boost::asio::const_buffer _buffer;
	mutable boost::optional<example::opt_fields::bits_type> _opts;

	explicit reader(boost::asio::const_buffer buf) :
			_buffer(std::move(buf)) {

	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, void>::type operator()(
			T& val) const {
		val = *(boost::asio::buffer_cast<T const*>(_buffer));
		_buffer = _buffer + sizeof(T);
		std::cout << " <-- reader is_integral functor called for " << val
				<< std::endl;
	}

	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, void>::type operator()(
			T& val) const {
		val = *(boost::asio::buffer_cast<T const*>(_buffer));
		_buffer = _buffer + sizeof(T);
		std::cout << " <-- reader is_enum functor called for " << val
				<< std::endl;
	}

	template<typename T, T v>
	void operator()(std::integral_constant<T, v>) const {
		typedef std::integral_constant<T, v> type;
		typename type::value_type val;
		(*this)(val);
		std::cout << " <-- reader integral_constant functor called for " << val
				<< std::endl;
		if (val != type::value) {
			throw std::runtime_error(
					" Wrong Header Information [ Magic / Vreison ]....");
		}
	}

	void operator()(std::string& val) const {
		// Read Length first
		int length = 0;
		(*this)(length);
		val = std::string(boost::asio::buffer_cast<char const *>(_buffer),
				length);
		_buffer = _buffer + length;
		std::cout << " <-- reader std::string functor called for " << val
				<< std::endl;
	}

	template<typename T>
	void operator()(std::vector<T>& vals) const {
		size_t length;
		(*this)(length);
		for (; length; --length) {
			T v;
			(*this)(v);
			//vals.emplace_back(std::move(v));
			vals.push_back(v);

		}
	}

	template<typename K, typename V>
	void operator()(std::unordered_map<K, V> & vals) const {
		size_t len;
		(*this)(len);

		for (; len; --len) {
			K key;
			(*this)(key);
			V val;
			(*this)(val);
			vals.insert(std::make_pair(key, val));
		}
	}

	template<typename T>
	typename std::enable_if<boost::fusion::traits::is_sequence<T>::value, void>::type operator()(
			T& val) const {
		boost::fusion::for_each(val, *this);
	}

	// Reading to know whether the bit is set or not bit set
	// Reading the bitset array
	//template<class T, size_t N>
	void operator()(example::opt_fields) const {
		example::opt_fields::value_type val;
		(*this)(val);
		_opts = example::opt_fields::bits_type(val);
//		std::cout << " <-- reader bitset array functor called for "
//						<< _opts.get() << std::endl;
		//example::opt_fields::bits_type bs = _opts.get();
		//bs.flip();
		//_opts.reset(bs);
		//example::least_significant_bit(*_opts);
		std::cout << " <-- reader bitset array functor called for "
				<< _opts.get() << std::endl;
	}

	// Reading the actual optional field data
	template<typename T, size_t N>
	void operator()(example::optional_field<T, N>& val) const {
		if (!_opts) {
			throw std::runtime_error(" Optional bits not set!! ");
		}
//		std::cout << " optional_field called for N: " << N << " bitset: "
//				<< *_opts << std::endl;
		if ((*_opts)[N] == 1) {
			T v;
			(*this)(v);
			//val = example::optional_field<T, N>(v);
			val.reset(v);
			std::cout << " <-- reader optional_field array functor called for "
					<< val.get() << std::endl;
		}
	}

};

template<typename T>
std::pair<T, boost::asio::const_buffer> read(boost::asio::const_buffer b) {
	reader r(std::move(b));
	T readSeq;
	// Now call reader functor for all members of T
	boost::fusion::for_each(readSeq, r);
	return std::make_pair(readSeq, r._buffer);
}

// Writer Adaptor
struct writer {

	mutable boost::asio::mutable_buffer _buffer;
	mutable example::opt_fields::bits_type _opts;
	mutable example::opt_fields::value_type * _optv;

	explicit writer(boost::asio::mutable_buffer buf) :
			_buffer(std::move(buf)) {

	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, void>::type operator()(
			T const& val) const {
		std::cout << " -- > writer is_integral functor called for " << val
				<< " Size(T): " << sizeof(T) << std::endl;
		boost::asio::buffer_copy(_buffer, boost::asio::buffer(&val, sizeof(T)));
		_buffer = _buffer + sizeof(T);
	}

	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, void>::type operator()(
			T const& val) const {

		std::cout << " -- > writer is_enum functor called for " << val
				<< " Size(T): " << sizeof(T) << std::endl;
		boost::asio::buffer_copy(_buffer, boost::asio::buffer(&val, sizeof(T)));
		_buffer = _buffer + sizeof(T);
	}

	template<typename T, T v>
	void operator()(std::integral_constant<T, v>) const {
		std::cout << " -- > writer integral_constant functor called for " << v
				<< " Size(T): " << sizeof(T) << std::endl;
		(*this)(std::integral_constant<T, v>::type::value);
	}

	void operator()(const std::string & val) const {

		(*this)(static_cast<int>(val.length()));
		std::cout << " -- > writer std::string functor called for " << val
				<< std::endl;
		boost::asio::buffer_copy(_buffer, boost::asio::buffer(val));
		_buffer = _buffer + val.length();
	}

	template<typename T>
	void operator()(std::vector<T> const & vals) const {
		size_t len = vals.size();
		(*this)(len);
		for (int i = 0; i < vals.size(); ++i) {
			std::cout << " --> writer for vector at: " << i << " val: "
					<< vals.at(i) << " size: " << sizeof(vals.at(i))
					<< std::endl;
			(*this)(vals.at(i));
		}

	}

	template<typename K, typename V>
	void operator()(std::unordered_map<K, V> const& vals) const {
		typedef typename std::unordered_map<K, V>::const_iterator MAP_IT;
		size_t len = vals.size();
		(*this)(len);
		for (MAP_IT it = vals.begin(); it != vals.end(); ++it) {
			(*this)(it->first);
			(*this)(it->second);
		}
	}

	template<typename T>
	typename std::enable_if<boost::fusion::traits::is_sequence<T>::value, void>::type operator()(
			T const& val) const {
		boost::fusion::for_each(val, *this);
	}

//	template<class T, size_t N>
	void operator()(example::opt_fields) const {
		//_opts.reset();
		std::cout << " -- > writer opt_fields bitset functor called. bitset: "
				<< _opts << std::endl;
		_optv = boost::asio::buffer_cast<example::opt_fields::value_type*>(
				_buffer);
		_buffer = _buffer + sizeof(example::opt_fields::value_type);
	}

	template<typename T, size_t N>
	void operator()(example::optional_field<T, N> const& val) const {
		if (!_optv) {
			std::runtime_error(" bitset not set!! ");
		}
		if (val) {
			_opts.set(N);
			*_optv =
					static_cast<example::opt_fields::value_type>(_opts.to_ulong());
			std::cout
					<< " -- > writer optional_field functor called for bitset value_type: "
					<< *_optv << " bitset: " << _opts << " value: " << val.get()
					<< std::endl;
			(*this)(val.get());
		}
	}

};

template<typename T>
boost::asio::mutable_buffer write(boost::asio::mutable_buffer b, T const& val) {
	writer w(std::move(b));
	boost::fusion::for_each(val, w);
	bool isEmpty = true;
	for (int i = 0; i < w._opts.size(); ++i) {
		if (w._opts.test(i)) {
			isEmpty = false;
			break;
		}

	}
	if (isEmpty) {
		*w._optv =
				static_cast<example::opt_fields::value_type>(w._opts.to_ulong());
	}
	return w._buffer;
}

namespace example {
enum msg_type_t {
	OPTION = 0, SPREAD = 1
};
}

namespace example {
using magic_t = std::integral_constant<int, 0xabcd>;
using version_t = std::integral_constant<int, 0xaaaa>;

}

typedef example::optional_field<int, 0> EXERCISE_T;
typedef example::optional_field<char, 1> EXERCISE_T2;
typedef std::unordered_map<int, std::string> FIXTAG_T1;

BOOST_FUSION_DEFINE_STRUCT((example), header,
		(example::magic_t, magic) (example::version_t, version) (int, length)

		(example::msg_type_t, msg_type) (std::string, id) (std::vector<int>, vecInts) (FIXTAG_T1, fixTags) (example::opt_fields, opts) (EXERCISE_T, et)(EXERCISE_T2, tt)

		//((example::optional_field<char COMMA 0>, et)
		//(boost::remove_reference<decltype(std::declval<example::optional_field<char, 0> >())>::type, et)
		);

typedef std::unordered_map<uint16_t, std::string> FIX_FIELD_STRING_TYPE;
typedef std::unordered_map<uint16_t, int> FIX_FIELD_INT_TYPE;
typedef std::unordered_map<uint16_t, char> FIX_FIELD_CHAR_TYPE;
//typedef example::optional_field<FIX_FIELD_STRING_TYPE, 0>



BOOST_FUSION_DEFINE_TPL_STRUCT(
		(T),
		(example),fg,
		(T, _value)
		);


BOOST_FUSION_DEFINE_STRUCT((example), fixHeader,
		(FIX_FIELD_STRING_TYPE, beginString) (FIX_FIELD_INT_TYPE, bodyLength) (FIX_FIELD_STRING_TYPE, msgType) (FIX_FIELD_STRING_TYPE, senderCompId) (FIX_FIELD_STRING_TYPE, targetCompId));


BOOST_FUSION_DEFINE_STRUCT((example), fixMsg, (example::fixHeader, fh)(std::string, field1));


int main() {

	NewOrderMessage now = { "Z$", "ZHJ6", 12.56, 10, 'B' };

	example::header _head;
	_head.length = 19;
	_head.msg_type = example::SPREAD;
	_head.id = "TEST";
	_head.et.reset(34);
	_head.tt.reset('A');

	_head.vecInts.push_back(10);
	_head.vecInts.push_back(20);
	_head.vecInts.push_back(30);

	_head.fixTags.insert(std::make_pair(9, "FIX.4.2"));
	_head.fixTags.insert(std::make_pair(35, "D"));

	{
		boost::array<char, 128> buf;
		boost::asio::mutable_buffer newBuf = write<example::header>(
				boost::asio::buffer(buf), _head);

		example::header h;
		boost::asio::const_buffer res;
		std::pair<example::header, boost::asio::const_buffer> resPair = read<
				example::header>(boost::asio::buffer(buf));

	}

	example::fixMsg fmsg;
	fmsg.fh.beginString.insert(std::make_pair(9, "FIX.4.2"));


	{
//		boost::array<char, 128> buf;
//		boost::asio::mutable_buffer newBuf = write<NewOrderMessage>(
//				boost::asio::buffer(buf), now);
//
//		example::header h;
//		boost::asio::const_buffer res;
//		std::pair<NewOrderMessage, boost::asio::const_buffer> resPair = read<
//				NewOrderMessage>(boost::asio::buffer(buf));
	}

	return 0;
}

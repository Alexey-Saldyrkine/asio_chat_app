#pragma once
#include <utility>
#include <type_traits>
template<typename ... Ts>
struct mp_list {
};

template<typename T>
struct type_tag {
	using type = T;
};

template<int i>
struct index_tag {
	static constexpr int value = i;
};

template<typename K, typename V>
struct typePair {
	using first_type = K;
	using second_type = V;
};

template<typename Pair>
struct element {
	static auto value(type_tag<typename Pair::first_type>) ->
	typename Pair::second_type;
};

struct emptyElement {
	static auto value(...)->index_tag<-1>;
};

template<typename ... elems>
struct type_map:emptyElement, element<elems> ... {
	using emptyElement::value;
	using element<elems>::value...;
	//static auto value(...) -> index_tag<-1>;
	template<typename K>
	static constexpr int find = decltype(type_map::value(type_tag<K> {}))::value; // @suppress("Function cannot be resolved")

	template<typename T>
	static constexpr bool hasType = (std::is_same_v<T, elems> || ...);
};

template<typename, typename >
struct createMap {
	static_assert(0,"should not be reached");
};

template<typename ... Ts, int ... Is>
struct createMap<mp_list<Ts...>, std::integer_sequence<int, Is...>> {
	using type = type_map<typePair<Ts,index_tag<Is>>...>;
};

template<typename T>
constexpr std::size_t hashOfType() {
	return typeid(T).hash_code();
}

template<typename ... Ts>
struct hashTypePack {
	static_assert(0,"should not be reached");
};

template<typename T, typename ... Rs>
struct hashTypePack<T, Rs...> {
	static constexpr std::size_t seed = hashTypePack<Rs...>::value;
	static constexpr std::size_t value = seed
			^ (hashOfType<T>() + 0x9e3779b9 + (seed << 6) + (seed >> 2));
};

template<typename T>
struct hashTypePack<T> {
	static constexpr std::size_t seed = 0;
	static constexpr std::size_t value = seed
			^ (hashOfType<T>() + 0x9e3779b9 + (seed << 6) + (seed >> 2));
};

template<typename ... Ts>
struct TypeIdMap {
	using typeId = int;
	using map = typename createMap<mp_list<Ts...>,std::make_integer_sequence<int,sizeof...(Ts)>>::type;

	static constexpr std::size_t size = sizeof...(Ts);

	template<typename T>
	static constexpr int toId = map::template find<
			std::remove_cv_t<std::remove_reference_t<T>>>;

	template<typename T>
	static constexpr bool hasType = (toId<T> != -1);

	static constexpr std::size_t packHash = hashTypePack<Ts...>::value;
};


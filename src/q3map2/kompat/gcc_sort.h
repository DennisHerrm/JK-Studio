// gcc_sort.h - std::sort mit derselben Reihenfolge wie GCCs libstdc++ (Stufe 3
// Punkt 2, Anpassung an MSVC Nummer 7).
//
// q3map2 sortiert an mehreren Stellen Elemente, die fuer den Vergleich gleich
// sein koennen (lightmaps_ydnar.cpp: Flaechen nach Shader und Lightmap-Groesse,
// tjunction.cpp: Kanten nach Laenge, vis.cpp: Portale nach nummightsee, ...).
// std::sort ist nicht stabil: welche Reihenfolge gleichwertige Elemente danach
// haben, haengt vom Verfahren der Standardbibliothek ab - und damit Reihenfolge
// der Flaechen, Ecken und Lightmaps in der .bsp. NRCs q3map2.exe ist mit GCC
// gebaut (libstdc++); MSVCs std::sort teilt anders (Grenze 32 statt 16,
// Median aus neun statt drei).
//
// Hier dasselbe Verfahren wie libstdc++: Introsort aus der SGI STL (HP 1994,
// SGI 1996 - deren freie Lizenz erlaubt Nutzung und Weitergabe; die Schritte
// sind seither unveraendert): Teilen mit dem Median aus first + 1, Mitte und
// last - 1, unbewachtes Teilen nach Hoare, Grenze 16 Elemente, Tiefe
// 2 * floor(log2 n), darunter Heapsort; zum Schluss Einfuegen (die ersten 16
// bewacht, der Rest unbewacht). Die Vergleiche in derselben Reihenfolge, damit
// auch ein nicht ganz sauberer Vergleich dasselbe Ergebnis gibt.
// std::ranges::sort ist bei libstdc++ dasselbe Verfahren mit Vergleich und
// Projektion zusammen (bits/ranges_algo.h).
//
// Eingesetzt wird es in Kopien der betroffenen Dateien (src/q3map2/
// CMakeLists.txt: std::sort -> bhed_gcc::sort, std::ranges::sort ->
// bhed_gcc::ranges_sort); die Quellen von NRC bleiben unveraendert.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace bhed_gcc {

namespace detail {

template <class It>
using Diff = typename std::iterator_traits<It>::difference_type;

inline int lg( long long n ){   // floor(log2 n), n > 0
	int r = 0;
	while ( n > 1 ) {
		n >>= 1;
		++r;
	}
	return r;
}

template <class It, class Comp>
void unbewachtEinfuegen( It last, Comp& comp ){   // __unguarded_linear_insert
	auto wert = std::move( *last );
	It naechster = last;
	--naechster;
	while ( comp( wert, *naechster ) ) {
		*last = std::move( *naechster );
		last = naechster;
		--naechster;
	}
	*last = std::move( wert );
}

template <class It, class Comp>
void einfuegen( It first, It last, Comp& comp ){   // __insertion_sort
	if ( first == last ) {
		return;
	}
	for ( It i = first + 1; i != last; ++i ) {
		if ( comp( *i, *first ) ) {
			auto wert = std::move( *i );
			std::move_backward( first, i, i + 1 );
			*first = std::move( wert );
		}
		else {
			unbewachtEinfuegen( i, comp );
		}
	}
}

template <class It, class Comp>
void schlussEinfuegen( It first, It last, Comp& comp ){   // __final_insertion_sort
	constexpr Diff<It> grenze = 16;
	if ( last - first > grenze ) {
		einfuegen( first, first + grenze, comp );
		for ( It i = first + grenze; i != last; ++i ) {   // __unguarded_insertion_sort
			unbewachtEinfuegen( i, comp );
		}
	}
	else {
		einfuegen( first, last, comp );
	}
}

template <class It, class T, class Comp>
void heapHoch( It first, Diff<It> loch, Diff<It> oben, T wert, Comp& comp ){   // __push_heap
	Diff<It> eltern = ( loch - 1 ) / 2;
	while ( loch > oben && comp( *( first + eltern ), wert ) ) {
		*( first + loch ) = std::move( *( first + eltern ) );
		loch = eltern;
		eltern = ( loch - 1 ) / 2;
	}
	*( first + loch ) = std::move( wert );
}

template <class It, class T, class Comp>
void heapRichten( It first, Diff<It> loch, Diff<It> laenge, T wert, Comp& comp ){   // __adjust_heap
	const Diff<It> oben = loch;
	Diff<It> zweites = loch;
	while ( zweites < ( laenge - 1 ) / 2 ) {
		zweites = 2 * ( zweites + 1 );
		if ( comp( *( first + zweites ), *( first + ( zweites - 1 ) ) ) ) {
			--zweites;
		}
		*( first + loch ) = std::move( *( first + zweites ) );
		loch = zweites;
	}
	if ( ( laenge & 1 ) == 0 && zweites == ( laenge - 2 ) / 2 ) {
		zweites = 2 * ( zweites + 1 );
		*( first + loch ) = std::move( *( first + ( zweites - 1 ) ) );
		loch = zweites - 1;
	}
	heapHoch( first, loch, oben, std::move( wert ), comp );
}

template <class It, class Comp>
void heapBauen( It first, It last, Comp& comp ){   // __make_heap
	const Diff<It> laenge = last - first;
	if ( laenge < 2 ) {
		return;
	}
	Diff<It> eltern = ( laenge - 2 ) / 2;
	for ( ;; ) {
		auto wert = std::move( *( first + eltern ) );
		heapRichten( first, eltern, laenge, std::move( wert ), comp );
		if ( eltern == 0 ) {
			return;
		}
		--eltern;
	}
}

template <class It, class Comp>
void heapNehmen( It first, It last, It ergebnis, Comp& comp ){   // __pop_heap
	auto wert = std::move( *ergebnis );
	*ergebnis = std::move( *first );
	heapRichten( first, Diff<It>( 0 ), last - first, std::move( wert ), comp );
}

template <class It, class Comp>
void heapsort( It first, It last, Comp& comp ){   // __partial_sort( first, last, last ): __heap_select + __sort_heap
	heapBauen( first, last, comp );
	while ( last - first > 1 ) {
		--last;
		heapNehmen( first, last, last, comp );
	}
}

template <class It, class Comp>
void medianNachVorn( It ergebnis, It a, It b, It c, Comp& comp ){   // __move_median_to_first
	if ( comp( *a, *b ) ) {
		if ( comp( *b, *c ) ) {
			std::iter_swap( ergebnis, b );
		}
		else if ( comp( *a, *c ) ) {
			std::iter_swap( ergebnis, c );
		}
		else {
			std::iter_swap( ergebnis, a );
		}
	}
	else if ( comp( *a, *c ) ) {
		std::iter_swap( ergebnis, a );
	}
	else if ( comp( *b, *c ) ) {
		std::iter_swap( ergebnis, c );
	}
	else {
		std::iter_swap( ergebnis, b );
	}
}

template <class It, class Comp>
It unbewachtTeilen( It first, It last, It pivot, Comp& comp ){   // __unguarded_partition
	for ( ;; ) {
		while ( comp( *first, *pivot ) ) {
			++first;
		}
		--last;
		while ( comp( *pivot, *last ) ) {
			--last;
		}
		if ( !( first < last ) ) {
			return first;
		}
		std::iter_swap( first, last );
		++first;
	}
}

template <class It, class Comp>
void introsortSchleife( It first, It last, int tiefe, Comp& comp ){   // __introsort_loop
	constexpr Diff<It> grenze = 16;
	while ( last - first > grenze ) {
		if ( tiefe == 0 ) {
			heapsort( first, last, comp );
			return;
		}
		--tiefe;
		const It mitte = first + ( last - first ) / 2;
		medianNachVorn( first, first + 1, mitte, last - 1, comp );
		const It schnitt = unbewachtTeilen( first + 1, last, first, comp );
		introsortSchleife( schnitt, last, tiefe, comp );
		last = schnitt;
	}
}

}  // namespace detail

template <class It, class Comp>
void sort( It first, It last, Comp comp ){
	if ( first != last ) {
		detail::introsortSchleife( first, last, detail::lg( static_cast<long long>( last - first ) ) * 2, comp );
		detail::schlussEinfuegen( first, last, comp );
	}
}

template <class It>
void sort( It first, It last ){
	bhed_gcc::sort( first, last, std::less<>() );
}

struct RangesSort {
	template <class R, class Comp = std::ranges::less, class Proj = std::identity>
	void operator()( R&& r, Comp comp = {}, Proj proj = {} ) const {
		auto vergleich = [&comp, &proj]( auto&& a, auto&& b ) -> bool {
			return std::invoke( comp, std::invoke( proj, a ), std::invoke( proj, b ) );
		};
		bhed_gcc::sort( std::ranges::begin( r ), std::ranges::end( r ), vergleich );
	}
};
inline constexpr RangesSort ranges_sort{};

}  // namespace bhed_gcc

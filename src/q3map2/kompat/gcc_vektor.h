// gcc_vektor.h - std::vector::emplace_back, das beim Vergroessern verschiebt wie
// libstdc++ (Stufe 3 Punkt 2, Anpassung an MSVC Nummer 8).
//
// q3map2 haelt die Karte in std::vector<entity_t> entities (q3map2.h). Jedes
// entity_t hat seine Pinsel in einer std::list<brush_t>, und jeder Pinsel zeigt
// mit brush_t::original auf sich selbst (map.cpp FinishBrush: b.original = &b);
// die Teilstuecke im BSP-Baum tragen diesen Zeiger weiter, und writebsp.cpp
// EmitLeaf schreibt b.original->outputNum in den Lump leafbrushes.
//
// Waechst entities beim Laden (map.cpp ParseMapEntity: entities.emplace_back()),
// zieht der Vektor die schon geladenen Entities um. std::vector verschiebt dabei
// nur, wenn der Verschiebe-Konstruktor noexcept ist, sonst kopiert er. Bei
// libstdc++ ist std::list noexcept verschiebbar - die Listenknoten bleiben, wo
// sie sind, die Zeiger stimmen. Bei MSVC ist er es nicht (er legt einen neuen
// Wachknoten an: std::is_nothrow_move_constructible_v<std::list<int>> ist 0,
// ebenso bei std::set und std::map), also kopiert der Vektor die Listen, gibt die
// alten frei - und brush_t::original zeigt ins Leere. Folge ohne diese Anpassung:
// Muell im Lump leafbrushes (Kollision im Spiel kaputt) und Abstuerze in -light.
//
// emplace_back hier vergroessert vorher selbst, mit std::move fuer jedes Element:
// dann wandern die Listenknoten mit wie bei libstdc++. Die neue Groesse wie
// libstdc++ (_M_check_len: alte Groesse + max(alte Groesse, 1)).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace bhed_gcc {

template <class T, class A>
T& emplace_back( std::vector<T, A>& v ){
	if ( v.size() == v.capacity() ) {
		std::vector<T, A> groesser( v.get_allocator() );
		groesser.reserve( v.size() + std::max<std::size_t>( v.size(), 1 ) );
		for ( T& e : v ) {
			groesser.emplace_back( std::move( e ) );   // reserviert: kein Umzug mehr
		}
		v.swap( groesser );
	}
	return v.emplace_back();
}

}  // namespace bhed_gcc

/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BILDLISTE2D_BESCH_H
#define __BILDLISTE2D_BESCH_H

#include "bildliste_besch.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      2 dimensionales Bilder-Array
 *
 *  Kindknoten:
 *	0   1. Bildliste
 *	1   2. Bildliste
 *	... ...
 */
class bildliste2d_besch_t : public obj_besch_t {
	friend class imagelist2d_reader_t;

	uint16  count;

public:
	bildliste2d_besch_t() : count(0) {}

	uint16 get_count() const { return count; }

	bildliste_besch_t const* get_liste(uint16 i)          const { return i < count ? get_child<bildliste_besch_t>(i)              : 0; }
	bild_besch_t      const* get_bild(uint16 i, uint16 j) const { return i < count ? get_child<bildliste_besch_t>(i)->get_bild(j) : 0; }
};

#endif

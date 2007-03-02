/*
 *
 *  ware_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __WARE_BESCH_H
#define __WARE_BESCH_H

#include "obj_besch_std_name.h"
#include "../simcolor.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Text Ma�einheit
 */
class ware_besch_t : public obj_besch_std_name_t {
	friend class good_writer_t;
	friend class good_reader_t;
	friend class warenbauer_t;

	/*
	* The base value is the one for multiplier 1000.
	*/
	uint16 value;
	uint16 base_value;

	/**
	* Category of the good
	* @author Hj. Malthaner
	*/
	uint8 catg;

	/**
	* total index, all ware with same cagt index will be compatible,
	* including special freight
	* assigned during registration
	* @author prissi
	*/
	uint8 catg_index;

	// used for inderect index (saves 3 bytes per ware_t!)
	// assinged during registration
	uint8 ware_index;

	COLOR_VAL color;

	/**
	* Bonus for fast transport given in percent!
	* @author Hj. Malthaner
	*/
	uint16 speed_bonus;

	/**
	* Weight in KG per unit of this good
	* @author Hj. Malthaner
	*/
	uint16 weight_per_unit;

public:
	// the measure for that good (crates, people, bags ... )
	const char *gib_mass() const
	{
		return static_cast<const text_besch_t *>(gib_kind(2))->gib_text();
	}

	uint16 gib_preis() const { return value; }

	/**
	* @return speed bonus value of the good
	* @author Hj. Malthaner
	*/
	uint16 gib_speed_bonus() const { return speed_bonus; }

	/**
	* @return Category of the good
	* @author Hj. Malthaner
	*/
	uint8 gib_catg() const { return catg; }

	/**
	* @return Category of the good
	* @author Hj. Malthaner
	*/
	uint8 gib_catg_index() const { return catg_index; }

	/**
	* @return internal index (just a number, passenger, then mail, then something ... )
	* @author prissi
	*/
	uint8 gib_index() const { return ware_index; }

	/**
	* @return weight in KG per unit of the good
	* @author Hj. Malthaner
	*/
	uint16 gib_weight_per_unit() const { return weight_per_unit; }

	/**
	* @return Name of the category of the good
	* @author Hj. Malthaner
	*/
	const char * gib_catg_name() const;

	/**
	* Checks if this good can be interchanged with the other, in terms of
	* transportability.
	*
	* Inline because called very often
	*
	* @author Hj. Malthaner
	*/
	bool is_interchangeable(const ware_besch_t *other) const
	{
		return catg_index == other->gib_catg_index();
	}

	/**
	* @return color for good table and waiting bars
	* @author Hj. Malthaner
	*/
	COLOR_VAL gib_color() const { return color; }
};

#endif

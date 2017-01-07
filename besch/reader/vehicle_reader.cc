#include <stdio.h>
#include "../../simdebug.h"
#include "../../simconst.h"
#include "../../bauer/vehikelbauer.h"
#include "../sound_besch.h"
#include "../vehikel_besch.h"
#include "../intro_dates.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"



void vehicle_reader_t::register_obj(obj_desc_t *&data)
{
	vehikel_besch_t *desc = static_cast<vehikel_besch_t *>(data);
	vehikelbauer_t::register_desc(desc);
	obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool vehicle_reader_t::successfully_loaded() const
{
	return vehikelbauer_t::successfully_loaded();
}


obj_desc_t *vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	vehikel_besch_t *desc = new vehikel_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 1) {
		// Versioned node, version 1

		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint16(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->gear = decode_uint8(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);

		desc->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if(version == 2) {
		// Versioned node, version 2

		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint16(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->gear = decode_uint8(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
		desc->engine_type = decode_uint8(p);

		desc->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if (version==3   ||  version==4  ||  version==5) {
		// Versioned node, version 3 with retire date
		// version 4 identical, just other values for the waytype
		// version 5 just uses the new scheme for data calculation

		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint16(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint8(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
		desc->engine_type = decode_uint8(p);
	}
	else if (version==6) {
		// version 5 just 32 bit for power and 16 Bit for gear

		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint32(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
	}
	else if (version==7) {
		// different length of cars ...

		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint32(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
	}
	else if (version==8) {
		// multiple freight images...
		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint32(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
		desc->freight_image_type = decode_uint8(p);
	}
	else if (version==9) {
		// new: fixed_cost, loading_time, axle_load
		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->loading_time = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->axle_load = decode_uint16(p);
		desc->leistung = decode_uint32(p);
		desc->running_cost = decode_uint16(p);
		desc->fixed_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
		desc->freight_image_type = decode_uint8(p);
	}
	else if (version==10) {
		// new: weight in kgs
		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->loading_time = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint32(p);
		desc->axle_load = decode_uint16(p);
		desc->leistung = decode_uint32(p);
		desc->running_cost = decode_uint16(p);
		desc->fixed_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
		desc->freight_image_type = decode_uint8(p);
	}
	else if (version==11) {
		// new: fix cost as uint32
		desc->cost = decode_uint32(p);
		desc->zuladung = decode_uint16(p);
		desc->loading_time = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint32(p);
		desc->axle_load = decode_uint16(p);
		desc->leistung = decode_uint32(p);
		desc->running_cost = decode_uint16(p);
		desc->fixed_cost = decode_uint32(p);

		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wt = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->vorgaenger = decode_uint8(p);
		desc->nachfolger = decode_uint8(p);
		desc->freight_image_type = decode_uint8(p);
	}
	else {
		if(  version!=0  ) {
			dbg->fatal( "vehicle_reader_t::read_node()","Do not know how to handle version=%i", version );
		}
		// old node, version 0

		desc->wt = (sint8)v;
		desc->zuladung = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->topspeed = decode_uint16(p);
		desc->gewicht = decode_uint16(p);
		desc->leistung = decode_uint16(p);
		desc->running_cost = decode_uint16(p);
		desc->sound = (sint8)decode_sint16(p);
		desc->vorgaenger = (sint8)decode_uint16(p);
		desc->nachfolger = (sint8)decode_uint16(p);

		desc->intro_date = DEFAULT_INTRO_DATE*16;
		desc->obsolete_date = (DEFAULT_RETIRE_DATE*16);
		desc->gear = 64;
	}

	// correct the engine type for old vehicles
	if(version<2) {
		// steam eangines usually have a sound of 3
		// electric engines will be overridden further down ...
		desc->engine_type = (desc->sound==3) ? vehikel_besch_t::steam : vehikel_besch_t::diesel;
	}

	//change the vehicle type
	if(version<4) {
		if(desc->wt==4) {
			desc->engine_type = vehikel_besch_t::electric;
			desc->wt = 1;
		}
		// convert to new standard
		static const waytype_t convert_from_old[8]={road_wt, track_wt, water_wt, air_wt, invalid_wt, monorail_wt, invalid_wt, tram_wt };
		desc->wt = convert_from_old[desc->wt];
	}

	// before version 5 dates were based on base 12 ...
	if(version<5) {
		uint16 date=desc->intro_date;
		desc->intro_date = (date/16)*12 + (date%16);
		date=desc->obsolete_date;
		desc->obsolete_date = (date/16)*12 + (date%16);
	}

	// before the length was always 1/8 (=half a tile)
	if(version<7) {
		desc->len = CARUNITS_PER_TILE/2;
	}

	// adjust length for different offset step sizes (which may arise in future)
	desc->len *= OBJECT_OFFSET_STEPS/CARUNITS_PER_TILE;

	// before version 8 vehicles could only have one freight image in each direction
	if(version<8) {
		desc->freight_image_type=0;
	}

	if(version<9) {
		desc->fixed_cost = 0;
		desc->axle_load = 0;
		desc->loading_time = 1000;
	}

	// old weights were tons
	if(version<10) {
		desc->gewicht *= 1000;
	}

	if(desc->sound==LOAD_SOUND) {
		uint8 len=decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for(uint8 i=0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		desc->sound = (sint8)sound_besch_t::get_sound_id(wavname);
DBG_MESSAGE("vehicle_reader_t::register_obj()","sound %s to %i",wavname,desc->sound);
	}
	else if(desc->sound>=0  &&  desc->sound<=MAX_OLD_SOUNDS) {
		sint16 old_id = desc->sound;
		desc->sound = (sint8)sound_besch_t::get_compatible_sound_id((sint8)old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,desc->sound);
	}

	DBG_DEBUG("vehicle_reader_t::read_node()",
		"version=%d "
		"way=%d zuladung=%d cost=%d topspeed=%d gewicht=%g axle_load=%d leistung=%d "
		"betrieb=%d sound=%d vor=%d nach=%d "
		"date=%d/%d gear=%d engine_type=%d len=%d",
		version,
		desc->wt,
		desc->zuladung,
		desc->cost,
		desc->topspeed,
		desc->gewicht/1000.0,
		desc->axle_load,
		desc->leistung,
		desc->running_cost,
		desc->sound,
		desc->vorgaenger,
		desc->nachfolger,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		desc->gear,
		desc->engine_type,
		desc->len);

	return desc;
}

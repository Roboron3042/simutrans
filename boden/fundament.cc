/*
 * Untergrund f�r Geb�ude in Simutrans.
 * �berarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "../simconst.h"

#include "../besch/grund_besch.h"
#include "../dataobj/loadsave.h"

#include "grund.h"
#include "fundament.h"


fundament_t::fundament_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) )
{
	rdwr(file);
	slope = slope_t::flat;
}


fundament_t::fundament_t(koord3d pos, slope_t::type hang ) : grund_t(pos)
{
	set_bild( IMG_LEER );
	if(hang) {
		pos = get_pos();
		pos.z += slope_t::max_diff(hang);
		set_pos( pos );
	}
	slope = slope_t::flat;
}


void fundament_t::calc_bild_internal(const bool calc_only_snowline_change)
{
	set_bild( grund_besch_t::get_ground_tile(this) );

	if(  !calc_only_snowline_change  ) {
		grund_t::calc_back_bild( get_disp_height(), 0 );
	}
}

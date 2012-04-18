#include "../simevent.h"
#include "../simcolor.h"
#include "../simconvoi.h"
#include "../simworld.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../simfab.h"
#include "../simcity.h"
#include "karte.h"
#include "fahrplan_gui.h"

#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/fahrplan.h"
#include "../dataobj/powernet.h"
#include "../dataobj/ribi.h"

#include "../boden/wege/schiene.h"
#include "../dings/leitung2.h"
#include "../utils/cbuffer_t.h"
#include "../simgraph.h"
#include "../player/simplay.h"


sint32 reliefkarte_t::max_departed=0;
sint32 reliefkarte_t::max_arrived=0;
sint32 reliefkarte_t::max_cargo=0;
sint32 reliefkarte_t::max_convoi_arrived=0;
sint32 reliefkarte_t::max_passed=0;
sint32 reliefkarte_t::max_tourist_ziele=0;

reliefkarte_t * reliefkarte_t::single_instance = NULL;
karte_t * reliefkarte_t::welt = NULL;
reliefkarte_t::MAP_MODES reliefkarte_t::mode = MAP_TOWN;
reliefkarte_t::MAP_MODES reliefkarte_t::last_mode = MAP_TOWN;
bool reliefkarte_t::is_visible = false;

// color for the land
const uint8 reliefkarte_t::map_type_color[MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND] =
{
	97, 99, 19, 21, 23,
	160, 161, 162, 163, 164, 165, 166, 167, 205, 206, 207, 173, 175, 214
};

const uint8 reliefkarte_t::severity_color[MAX_SEVERITY_COLORS] =
{
	106, 2, 85, 86, 29, 30, 171, 71, 39, 132
};

// Kenfarben fuer die Karte
#define STRASSE_KENN      (208)
#define SCHIENE_KENN      (185)
#define CHANNEL_KENN      (147)
#define MONORAIL_KENN      (153)
#define RUNWAY_KENN      (28)
#define POWERLINE_KENN      (55)
#define HALT_KENN         COL_RED
#define BUILDING_KENN      COL_GREY3


// helper function for line segment_t
bool reliefkarte_t::line_segment_t::operator == (const line_segment_t & k) const
{
	return start == k.start  &&  end == k.end  &&  sp == k.sp  &&  fpl->similar( sp->get_welt(), k.fpl, sp );
}

// Ordering based on first start then end coordinate
bool reliefkarte_t::LineSegmentOrdering::operator()(const reliefkarte_t::line_segment_t& a, const reliefkarte_t::line_segment_t& b) const
{
	if(  a.start.x == b.start.x  ) {
		// same start ...
		return a.end.x < b.end.x;
	}
	return a.start.x < b.start.x;
}


static COLOR_VAL colore = 0;
static uint8 counter_rail = 0;


// add the schedule to the map (if there is a valid one)
void reliefkarte_t::add_to_schedule_cache( convoihandle_t cnv, bool with_waypoints )
{
	// make sure this is valid!
	if(  !cnv.is_bound()  ) {
		return;
	}

	static COLOR_VAL rail_colors[] = {2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58, 62, 66, 70, 74, 78, 82, 86, 90, 94, 98, 102,
						   106, 110, 114, 118, 122, 126, 130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182, 186, 190,
						   194, 198, 202, 206, 210, 214, 218, 222, 226, 230, 234
						  };

	// otherwise normal color scheme
	if(  cnv->get_schedule()->get_waytype() == track_wt  ||  cnv->get_schedule()->get_waytype() == maglev_wt  ||  cnv->get_schedule()->get_waytype() == monorail_wt  ) {
		colore = rail_colors[counter_rail] + 1;

		if( ++counter_rail > 57 ) {
			counter_rail = 0;
		}
	}
	else {
		colore = (colore >= 208 ? 0 : colore+1);
	}

	// ok, add this schedule to map
	// from here on we have a valid convoi
	int stops = 0;
	koord old_stop, first_stop, temp_stop;
	bool last_diagonal = false;

	FOR(  minivec_tpl<linieneintrag_t>, cur, cnv->get_schedule()->eintrag  ) {

		//cycle on stops
		//try to read station's coordinates if there's a station at this schedule stop
		halthandle_t station = haltestelle_t::get_halt( welt, cur.pos, cnv->get_besitzer() );
		if( station.is_bound() ) {
			stop_cache.append_unique( station );
			temp_stop = station->get_basis_pos();
			stops ++;
		}
		else if(  with_waypoints  ) {
			temp_stop = cur.pos.get_2d();
			stops ++;
		}

		if(  stops>1  ) {
			last_diagonal ^= true;
			if(  (temp_stop.x-old_stop.x)*(temp_stop.y-old_stop.y) == 0  ) {
				last_diagonal = false;
			}
			schedule_cache.insert_unique_ordered( line_segment_t( temp_stop, old_stop, cnv->get_schedule(), cnv->get_besitzer(), colore, last_diagonal ), LineSegmentOrdering() );
			old_stop = temp_stop;
		}
		else {
			first_stop = temp_stop;
			old_stop = temp_stop;
		}
	}

	// connect to start
	if(  stops > 2  ) {
		last_diagonal ^= true;
		schedule_cache.insert_unique_ordered( line_segment_t( first_stop, old_stop, cnv->get_schedule(), cnv->get_besitzer(), colore, last_diagonal ), LineSegmentOrdering() );
	}
}



// some rountes for the relief map with schedules
static void display_airport ( const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color )
{
	int x = xx + 5;
	int y = yy - 11;

	if ( y < 0 ) {
		y = 0;
	}

	const char symbol[] = {
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', '.', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '.', 'X',
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
		'X', 'X', '.', '.', '.', 'X', '.', '.', '.', 'X', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
	};

	for ( int i = 0; i < 11; i++ ) {
		for ( int j = 0; j < 11; j++ ) {
			if ( symbol[i + j * 11] == 'X' ) {
				display_vline_wh_clip( x + i, y + j, 1,  color, true );
			}
		}
	}
}

static void display_harbor ( const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color )
{
	int x = xx + 5;
	int y = yy - 11 + 13;	//to not overwrite airline symbol

	if ( y < 0 ) {
		y = 0;
	}

	const char symbol[] = {
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '.', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', 'X', 'X', '.', '.', 'X', '.', '.', 'X', 'X', 'X',
		'X', 'X', 'X', 'X', '.', 'X', '.', 'X', 'X', 'X', 'X',
		'X', '.', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '.', 'X',
		'X', '.', '.', 'X', 'X', 'X', 'X', 'X', '.', '.', 'X',
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
	};

	for ( int i = 0; i < 11; i++ ) {
		for ( int j = 0; j < 11; j++ ) {
			if ( symbol[i + j * 11] == 'X' ) {
				display_vline_wh_clip( x + i, y + j, 1,  color, true );
			}
		}
	}
}
// those will be replaced by pak images later ...!


static void display_thick_line ( short x1, short y1, short x2, short y2, short col, bool dotting, short dot_full, short dot_empty, short thickness )
{
	if ( abs ( x1 - x2 ) > abs ( y1 - y2 ) ) {
		//more horizontal than vertical -> more lines in vertical
		for ( int i = 0; i < thickness - 1; i++ ) {
			if ( !dotting ) {
				display_direct_line ( x1, y1 + i, x2, y2 + i, col );
			}
			else {
				display_direct_line_dotted ( x1, y1 + i, x2, y2 + i, dot_full, dot_empty, col );
			}
		}
	}
	else {
		for ( int i = 0; i < thickness - 1; i++ ) {
			if ( !dotting ) {
				display_direct_line ( x1 + i, y1, x2 + i, y2, col );
			}
			else {
				display_direct_line_dotted ( x1 + i, y1, x2 + i, y2, dot_full, dot_empty, col );
			}
		}
	}
}


static void line_segment_draw( waytype_t type, koord start, koord end, bool diagonal, COLOR_VAL colore )
{
	if(  type ==  air_wt  ) {
		draw_bezier( start.x, start.y, end.x, end.y, 50, 50, 50, 50, colore, 5, 5 );
		draw_bezier( start.x + 1, start.y + 1, end.x + 1, end.y + 1, 50, 50, 50, 50, colore, 5, 5 );
	}
	else {
		uint8 thickness = 3;
		bool dotted = false;
		switch(  type  ) {
			case track_wt:
			case monorail_wt:
			case maglev_wt:
				thickness = 5;
				break;
			case road_wt:
				thickness = 2;
				break;
			case tram_wt:
				thickness = 3;
				break;
			default:
				thickness = 3;
				dotted = true;
		}
		// start.x is always <= end.x ...
		const int delta_y = end.y-start.y;
		if(  (start.x-end.x)*delta_y == 0  ) {
			// horizontal/vertical line
			display_thick_line( start.x, start.y, end.x, end.y, colore, dotted, 5, 3, thickness );
		}
		else {
			// two segment
			koord mid;
			int signum_y = delta_y/abs(delta_y);
			if(  diagonal  ) {
				// start with diagonal
				if(  abs(delta_y) > end.x-start.x  ) {
					mid.x = end.x;
					mid.y = start.y + (end.x-start.x)*signum_y;
				}
				else {
					mid.x = start.x + abs(delta_y);
					mid.y = end.y;
				}
				display_thick_line( start.x, start.y, mid.x, mid.y, colore, dotted, 5, 3, thickness );
				display_thick_line( mid.x, mid.y, end.x, end.y, colore, dotted, 5, 3, thickness );
			}
			else {
				// end with diagonal
				const int delta_y = end.y-start.y;
				if(  abs(delta_y) > end.x-start.x  ) {
					mid.x = start.x;
					mid.y = end.y - (end.x-start.x)*signum_y;
				}
				else {
					mid.x = end.x - abs(delta_y);
					mid.y = start.y;
				}
				display_thick_line( start.x, start.y, mid.x, mid.y, colore, dotted, 5, 3, thickness );
				display_thick_line( mid.x, mid.y, end.x, end.y, colore, dotted, 5, 3, thickness );
			}
		}
	}
}


// converts karte koordinates to screen corrdinates
void reliefkarte_t::karte_to_screen( koord &k ) const
{
	// must be down before/after, of one would loose bits ...
	if(  zoom_in==1  ) {
		k.x = k.x*zoom_out;
		k.y = k.y*zoom_out;
	}
	if(isometric) {
		// 45 rotate view
		sint32 x = welt->get_groesse_y()*zoom_out + (sint32)(k.x-k.y) - 1;
		k.y = k.x/2+k.y/2;
		k.x = x;
	}
	if(  zoom_out==1  ) {
		k.x = k.x/zoom_in;
		k.y = k.y/zoom_in;
	}
}


// and retransform
inline void reliefkarte_t::screen_to_karte( koord &k ) const
{
	k = koord( (k.x*zoom_in)/zoom_out, (k.y*zoom_in)/zoom_out );
	if(isometric) {
		k.y *= 2;
		k.x = (sint16)(((sint32)k.x+(sint32)k.y-(sint32)welt->get_groesse_y())/2);
		k.y = k.y - k.x;
	}
}


uint8 reliefkarte_t::calc_severity_color(sint32 amount, sint32 max_value)
{
	if(max_value!=0) {
		// color array goes from light blue to red
		sint32 severity = amount * MAX_SEVERITY_COLORS / max_value;
		if(severity>=MAX_SEVERITY_COLORS) {
			severity = MAX_SEVERITY_COLORS-1;
		}
		else if(severity < 0) {
			severity = 0;
		}
		return reliefkarte_t::severity_color[severity];
	}
	return reliefkarte_t::severity_color[0];
}


void reliefkarte_t::set_relief_color_clip( sint16 x, sint16 y, uint8 color )
{
	if(  0<=x  &&  (uint16)x < relief->get_width()  &&  0<=y  &&  (uint16)y < relief->get_height()  ) {
		relief->at( x, y ) = color;
	}
}


void reliefkarte_t::set_relief_farbe(koord k, const int color)
{
	// if map is in normal mode, set new color for map
	// otherwise do nothing
	// result: convois will not "paint over" special maps
	if (relief==NULL  ||  !welt->ist_in_kartengrenzen(k)) {
		return;
	}

	karte_to_screen(k);
	k -= cur_off;

	if(isometric) {
		// since isometric is distorted
		const sint32 xw = zoom_in>=2 ? 1 : 2*zoom_out;
		switch(  xw  ) {
			case 1:
				set_relief_color_clip( k.x, k.y, color );
				break;
			case 2:
				set_relief_color_clip( k.x, k.y, color );
				set_relief_color_clip( k.x+1, k.y, color );
				break;
			case 4:
				for(  int x=1;  x<3; x++  ) {
					set_relief_color_clip( k.x+x, k.y, color );
				}
				for(  int x=0;  x<4;  x++  ) {
					set_relief_color_clip( k.x+x, k.y+1, color );
				}
				break;
			case 6:
				for(  int x=2;  x<5; x++  ) {
					set_relief_color_clip( k.x+x, k.y, color );
					set_relief_color_clip( k.x+x, k.y+2, color );
				}
				for(  int x=0;  x<6;  x++  ) {
					set_relief_color_clip( k.x+x, k.y+1, color );
				}
				break;
			case 8:
				for(  int x=2;  x<6; x++  ) {
					set_relief_color_clip( k.x+x, k.y+0, color );
					set_relief_color_clip( k.x+x, k.y+2, color );
				}
				for(  int x=0;  x<8;  x++  ) {
					set_relief_color_clip( k.x+x, k.y+1, color );
				}
				break;
		}
	}
	else {
		for(  sint32 x = max(0,k.x);  x < zoom_out+k.x  &&  (uint32)x < relief->get_width();  x++  ) {
			for(  sint32 y = max(0,k.y);  y < zoom_out+k.y  &&  (uint32)y < relief->get_height();  y++  ) {
				relief->at(x, y) = color;
			}
		}
	}
}


void reliefkarte_t::set_relief_farbe_area(koord k, int areasize, uint8 color)
{
	koord p;
	if(isometric) {
		k -= koord( areasize/2, areasize/2 );
		for(  p.x = 0;  p.x<areasize;  p.x++  ) {
			for(  p.y = 0;  p.y<areasize;  p.y++  ) {
				set_relief_farbe( k+p, color );
			}
		}
	}
	else {
		karte_to_screen(k);
		areasize *= zoom_out;
		k -= koord( areasize/2, areasize/2 );
		k.x = clamp( k.x, 0, get_groesse().x-areasize-1 );
		k.y = clamp( k.y, 0, get_groesse().y-areasize-1 );
		k -= cur_off;
		for(  p.x = max(0,k.x);  (uint16)p.x < areasize+k.x  &&  (uint16)p.x < relief->get_width();  p.x++  ) {
			for(  p.y = max(0,k.y);  (uint16)p.y < areasize+k.y  &&  (uint16)p.y < relief->get_height();  p.y++  ) {
				relief->at(p.x, p.y) = color;
			}
		}
	}
}


/**
 * calculates ground color for position (hoehe - grundwasser).
 * @author Hj. Malthaner
 */
uint8 reliefkarte_t::calc_hoehe_farbe(const sint16 hoehe, const sint16 grundwasser)
{
	return map_type_color[clamp( (hoehe-grundwasser)+MAX_MAP_TYPE_WATER-1, 0, MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND )];
}


/**
 * Updated Kartenfarbe an Position k
 * @author Hj. Malthaner
 */
uint8 reliefkarte_t::calc_relief_farbe(const grund_t *gr)
{
	uint8 color = COL_BLACK;

#ifdef DEBUG_ROUTES
	/* for debug purposes only ...*/
	if(welt->ist_markiert(gr)) {
		color = COL_PURPLE;
	}else
#endif
	if(gr->get_halt().is_bound()) {
		color = HALT_KENN;
	}
	else {
		switch(gr->get_typ()) {
			case grund_t::brueckenboden:
				color = MN_GREY3;
				break;
			case grund_t::tunnelboden:
				color = MN_GREY0;
				break;
			case grund_t::monorailboden:
				color = MONORAIL_KENN;
				break;
			case grund_t::fundament:
				{
					// object at zero is either factory or house (or attraction ... )
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
						color = COL_GREY3;
					}
					else {
						color = fab->get_kennfarbe();
					}
				}
				break;
			case grund_t::wasser:
				{
					// object at zero is either factory or boat
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->get_grund_hang()&1);
#else
						sint16 height = (gr->get_grund_hang()%3);
#endif
						color = calc_hoehe_farbe((welt->lookup_hgt(gr->get_pos().get_2d())/Z_TILE_STEP)+height, welt->get_grundwasser()/Z_TILE_STEP);
						//color = COL_BLUE;	// water with boat?
					}
					else {
						color = fab->get_kennfarbe();
					}
				}
				break;
			// normal ground ...
			default:
				if(gr->hat_wege()) {
					switch(gr->get_weg_nr(0)->get_waytype()) {
						case road_wt: color = STRASSE_KENN; break;
						case tram_wt:
						case track_wt: color = SCHIENE_KENN; break;
						case monorail_wt: color = MONORAIL_KENN; break;
						case water_wt: color = CHANNEL_KENN; break;
						case air_wt: color = RUNWAY_KENN; break;
						default:	// silence compiler!
							break;
					}
				}
				else {
					const leitung_t* lt = gr->find<leitung_t>();
					if(lt!=NULL) {
						color = POWERLINE_KENN;
					}
					else {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->get_grund_hang()&1);
#else
						sint16 height = (gr->get_grund_hang()%3);
#endif
						color = calc_hoehe_farbe((gr->get_hoehe()/Z_TILE_STEP)+height, welt->get_grundwasser()/Z_TILE_STEP);
					}
				}
				break;
		}
	}
	return color;
}


void reliefkarte_t::calc_map_pixel(const koord k)
{
	// we ignore requests, when nothing visible ...
	if(!is_visible) {
		return;
	}

	// always use to uppermost ground
	const planquadrat_t *plan=welt->lookup(k);
	if(plan==NULL  ||  plan->get_boden_count()==0) {
		return;
	}
	const grund_t *gr=plan->get_boden_bei(plan->get_boden_count()-1);

	if(  mode!=MAP_PAX_DEST  &&  gr->get_convoi_vehicle()  ) {
		set_relief_farbe( k, VEHIKEL_KENN );
		return;
	}

	// first use ground color
	set_relief_farbe( k, calc_relief_farbe(gr) );

	switch(mode) {
		// show passenger coverage
		// display coverage
		case MAP_PASSENGER:
			if(  plan->get_haltlist_count()>0  ) {
				halthandle_t halt = plan->get_haltlist()[0];
				if (halt->get_pax_enabled() && !halt->get_pax_connections().empty()) {
					set_relief_farbe( k, halt->get_besitzer()->get_player_color1() + 3 );
				}
			}
			break;

		// show mail coverage
		// display coverage
		case MAP_MAIL:
			if(  plan->get_haltlist_count()>0  ) {
				halthandle_t halt = plan->get_haltlist()[0];
				if (halt->get_post_enabled() && !halt->get_mail_connections().empty()) {
					set_relief_farbe( k, halt->get_besitzer()->get_player_color1() + 3 );
				}
			}
			break;

		// show usage
		case MAP_FREIGHT:
			// need to init the maximum?
			if(max_cargo==0) {
				max_cargo = 1;
				calc_map();
			}
			else if(gr->hat_wege()) {
				// now calc again ...
				sint32 cargo=0;

				// maximum two ways for one ground
				const weg_t *w=gr->get_weg_nr(0);
				if(w) {
					cargo = w->get_statistics(WAY_STAT_GOODS);
					const weg_t *w=gr->get_weg_nr(1);
					if(w) {
						cargo += w->get_statistics(WAY_STAT_GOODS);
					}
					if(cargo>0) {
						if(cargo>max_cargo) {
							max_cargo = cargo;
						}
						set_relief_farbe(k, calc_severity_color(cargo, max_cargo));
					}
				}
			}
			break;

		// show station status
		case MAP_STATUS:
			{
				halthandle_t halt = gr->get_halt();
				if(  halt.is_bound()  ) {
					const spieler_t* owner = halt->get_besitzer();
					if(  owner == welt->get_active_player()  ||  owner == welt->get_spieler(1)  ) {
						set_relief_farbe_area(k, 3, halt->get_status_farbe());
					}
				}
			}
			break;

		// show frequency of convois visiting a station
		case MAP_SERVICE:
			// need to init the maximum?
			if(max_convoi_arrived==0) {
				max_convoi_arrived = 1;
				calc_map();
			}
			else {
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if(  owner == welt->get_active_player()  ||  owner == welt->get_spieler(1)  ) {
						// get number of last month's arrived convois
						sint32 arrived = (sint32)halt->get_finance_history(1, HALT_CONVOIS_ARRIVED);
						if(arrived>max_convoi_arrived) {
							max_convoi_arrived = arrived;
						}
						set_relief_farbe_area(k, 3, calc_severity_color(arrived, max_convoi_arrived));
					}
				}
			}
			break;

		// show traffic (=convois/month)
		case MAP_TRAFFIC:
			// need to init the maximum?
			if(max_passed==0) {
				max_passed = 1;
				calc_map();
			}
			else if(gr->hat_wege()) {
				// now calc again ...
				sint32 passed=0;

				// maximum two ways for one ground
				const weg_t *w=gr->get_weg_nr(0);
				if(w) {
					passed = w->get_statistics(WAY_STAT_CONVOIS);
					const weg_t *w=gr->get_weg_nr(1);
					if(w) {
						passed += w->get_statistics(WAY_STAT_CONVOIS);
					}
					if (passed>0) {
						if(passed>max_passed) {
							max_passed = passed;
						}
						passed = ((passed*3)<<(MAX_SEVERITY_COLORS-2)/max_passed) + 1;
						int log_passed = MAX_SEVERITY_COLORS-1;
						while( (1<<log_passed) > passed) {
							log_passed --;
						}
						set_relief_farbe_area(k, 1, calc_severity_color(log_passed+1, MAX_SEVERITY_COLORS ));
					}
					else {
						set_relief_farbe_area(k, 1, reliefkarte_t::severity_color[0] );
					}
				}
			}
			break;

		// show sources of passengers
		case MAP_ORIGIN:
			// need to init the maximum?
			if(max_arrived==0) {
				max_arrived = 1;
				calc_map();
			}
			else if (gr->get_halt().is_bound()) {
				halthandle_t halt = gr->get_halt();
				// only show player's haltestellen
				const spieler_t* owner = halt->get_besitzer();
				if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
					 sint32 arrived = (sint32)( halt->get_finance_history(1, HALT_DEPARTED) - halt->get_finance_history(1, HALT_ARRIVED) );
					if(arrived>max_arrived) {
						max_arrived = arrived;
					}
					const uint8 color = calc_severity_color( arrived, max_arrived );
					set_relief_farbe_area(k, 3, color);
				}
			}
			break;

		// show destinations for passengers
		case MAP_DESTINATION:
			// need to init the maximum?
			if(max_departed==0) {
				max_departed = 1;
				calc_map();
			}
			else {
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
						sint32 departed = (sint32)( halt->get_finance_history(1, HALT_ARRIVED) - halt->get_finance_history(1, HALT_DEPARTED) );
						if(departed>max_departed) {
							max_departed = departed;
						}
						const uint8 color = calc_severity_color( departed, max_departed );
						set_relief_farbe_area(k, 3, color );
					}
				}
			}
			break;

		// show waiting goods
		case MAP_WAITING:
			{
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if(  owner == welt->get_active_player()  ||  owner == welt->get_spieler(1)  ) {
						// we need to sum up only for seperate capacities
						sint32 const total_capacity = welt->get_settings().is_seperate_halt_capacities() ? halt->get_capacity(0) + halt->get_capacity(1) + halt->get_capacity(2) : halt->get_capacity(0);
						const uint8 color = calc_severity_color( (sint32)halt->get_finance_history(0, HALT_WAITING), total_capacity );
						set_relief_farbe_area(k, 3, color );
					}
				}
			}
			break;

		// show tracks: white: no electricity, red: electricity, yellow: signal
		case MAP_TRACKS:
			// show track
			if (gr->hat_weg(track_wt)) {
				const schiene_t * sch = (const schiene_t *) (gr->get_weg(track_wt));
				if(sch->is_electrified()) {
					set_relief_farbe(k, COL_RED);
				}
				else {
					set_relief_farbe(k, COL_WHITE);
				}
				// show signals
				if(sch->has_sign()  ||  sch->has_signal()) {
					set_relief_farbe(k, COL_YELLOW);
				}
			}
			break;

		// show max speed (if there)
		case MAX_SPEEDLIMIT:
			{
				sint32 speed=gr->get_max_speed();
				if(speed) {
					set_relief_farbe(k, calc_severity_color(gr->get_max_speed(), 450));
				}
			}
			break;

		// find power lines
		case MAP_POWERLINES:
			{
				const leitung_t* lt = gr->find<leitung_t>();
				if(lt!=NULL) {
					set_relief_farbe(k, calc_severity_color(lt->get_net()->get_demand(),lt->get_net()->get_supply()) );
				}
			}
			break;

		case MAP_FOREST:
			if(  gr->get_top()>1  &&  gr->obj_bei(gr->get_top()-1)->get_typ()==ding_t::baum  ) {
				set_relief_farbe(k, COL_GREEN );
			}
			break;

		case MAP_PAX_DEST:
			if(  city  ) {
				const koord p = koord(
					((k.x * PAX_DESTINATIONS_SIZE) / welt->get_groesse_x()),
					((k.y * PAX_DESTINATIONS_SIZE) / welt->get_groesse_y())
				);
				const uint8 color = city->get_pax_destinations_new()->get(p);
				if( color != 0 ) {
				}
			}
			break;

		case MAP_OWNER:
			// show ownership
			{
				if(  gr->is_halt()  ) {
					set_relief_farbe(k, gr->get_halt()->get_besitzer()->get_player_color1()+3);
				}
				else if(  weg_t *weg = gr->get_weg_nr(0)  ) {
					set_relief_farbe(k, weg->get_besitzer()==NULL ? COL_ORANGE : weg->get_besitzer()->get_player_color1()+3 );
				}
				if(  gebaeude_t *gb = gr->find<gebaeude_t>()  ) {
					if(  gb->get_besitzer()!=NULL  ) {
						set_relief_farbe(k, gb->get_besitzer()->get_player_color1()+3 );
					}
				}
				break;
			}

		default:
			break;
	}
}


void reliefkarte_t::calc_map_groesse()
{
	koord size( welt->get_groesse_x(), 0 );
	koord down( welt->get_groesse_x(), welt->get_groesse_y() );
	karte_to_screen( size );
	karte_to_screen( down );
	size.y = down.y;
	if(  isometric  ) {
		size.x += zoom_out*2;
	}
	set_groesse( size ); // of the gui_komponete to adjust scroll bars
	needs_redraw = true;
}


void reliefkarte_t::calc_map()
{
	// only use bitmap size like screen size
	koord relief_size( min( get_groesse().x, new_size.x ), min( get_groesse().y, new_size.y ) );
	// actually the following line should reduce new/deletes, but does not work properly
	if(  relief==NULL  ||  (sint16)relief->get_width()!=relief_size.x  ||  (sint16)relief->get_height()!=relief_size.y  ) {
		delete relief;
		relief = new array2d_tpl<unsigned char> (relief_size.x,relief_size.y);
	}
	cur_off = new_off;
	cur_size = new_size;
	needs_redraw = false;
	is_visible = true;

	// redraw the map
	if(  !isometric  ) {
		koord k;
		koord start_off = koord( (cur_off.x*zoom_in)/zoom_out, (cur_off.y*zoom_in)/zoom_out );
		koord end_off = start_off+koord( (relief->get_width()*zoom_in)/zoom_out+1, (relief->get_height()*zoom_in)/zoom_out+1 );
		for(  k.y=start_off.y;  k.y<end_off.y;  k.y+=zoom_in  ) {
			for(  k.x=start_off.x;  k.x<end_off.x;  k.x+=zoom_in  ) {
				calc_map_pixel(k);
			}
		}
	}
	else {
		// always the whole map ...
		if(isometric) {
			relief->init( COL_BLACK );
		}
		koord k;
		for(  k.y=0;  k.y < welt->get_groesse_y();  k.y++  ) {
			for(  k.x=0;  k.x < welt->get_groesse_x();  k.x++  ) {
				calc_map_pixel(k);
			}
		}
	}

	// since we do iterate the tourist info list, this must be done here
	// find tourist spots
	if(mode==MAP_TOURIST) {
		const weighted_vector_tpl<gebaeude_t *> &ausflugsziele = welt->get_ausflugsziele();
		// find the current maximum
		max_tourist_ziele = 1;
		FOR(weighted_vector_tpl<gebaeude_t*>, const i, ausflugsziele) {
			int const pax = i->get_passagier_level();
			if (max_tourist_ziele < pax) {
				max_tourist_ziele = pax;
			}
		}
		// draw them
		FOR(weighted_vector_tpl<gebaeude_t*>, const g, ausflugsziele) {
			koord pos = g->get_pos().get_2d();
			set_relief_farbe_area( pos, 7, calc_severity_color(g->get_passagier_level(), max_tourist_ziele));
		}
		return;
	}

	// since we do iterate the factory info list, this must be done here
	if(mode==MAP_FACTORIES) {
		FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
			koord const pos = f->get_pos().get_2d();
			set_relief_farbe_area( pos, 9, COL_BLACK );
			set_relief_farbe_area(pos, 7, f->get_kennfarbe());
		}
		return;
	}

	if(mode==MAP_DEPOT) {
		FOR(slist_tpl<depot_t*>, const d, depot_t::get_depot_list()) {
			if (d->get_besitzer() == welt->get_active_player()) {
				koord const pos = d->get_pos().get_2d();
				// offset of one to avoid
				static uint8 depot_typ_to_color[19]={ COL_ORANGE, COL_YELLOW, COL_RED, 0, 0, 0, 0, 0, 0, COL_PURPLE, COL_DARK_RED, COL_DARK_ORANGE, 0, 0, 0, 0, 0, 0, COL_LIGHT_RED };
				set_relief_farbe_area(pos, 7, depot_typ_to_color[d->get_typ() - ding_t::bahndepot]);
			}
		}
		return;
	}
}


reliefkarte_t::reliefkarte_t()
{
	relief = NULL;
	zoom_out = 1;
	zoom_in = 1;
	isometric = false;
	mode = MAP_TOWN;
	city = NULL;
	is_show_schedule = false;
	is_show_fab = false;
	cur_off = new_off = cur_size = new_size = koord(0,0);
	needs_redraw = true;
}


reliefkarte_t::~reliefkarte_t()
{
	if(relief != NULL) {
		delete relief;
	}
}


reliefkarte_t *reliefkarte_t::get_karte()
{
	if(single_instance == NULL) {
		single_instance = new reliefkarte_t();
	}
	return single_instance;
}


void reliefkarte_t::set_welt(karte_t *welt)
{
	this->welt = welt;			// Welt fuer display_win() merken
	if(relief) {
		delete relief;
		relief = NULL;
	}
	needs_redraw = true;
	is_visible = false;

	if(welt) {
		calc_map_groesse();
		max_departed = max_arrived = max_cargo = max_convoi_arrived = max_passed = max_tourist_ziele = 0;
		last_schedule_counter = welt->get_schedule_counter()-1;
	}
}


void reliefkarte_t::set_mode(MAP_MODES new_mode)
{
	mode = new_mode;
	needs_redraw = true;
}


void reliefkarte_t::neuer_monat()
{
	needs_redraw = true;
}


// these two are the only gui_container specific routines


// handle event
bool reliefkarte_t::infowin_event(const event_t *ev)
{
	koord k( ev->mx, ev->my );
	screen_to_karte( k );

	// get factory under mouse cursor
	last_world_pos = k;

	// recenter
	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		welt->set_follow_convoi( convoihandle_t() );
		int z = 0;
		if(welt->ist_in_kartengrenzen(k)) {
			z = welt->min_hgt(k);
		}
		welt->change_world_position(koord3d(k,z));
		return true;
	}

	return false;
}


// helper function for redraw: factory connections
const fabrik_t* reliefkarte_t::draw_fab_connections(const uint8 colour, const koord pos) const
{
	const fabrik_t* const fab = fabrik_t::get_fab(welt, last_world_pos);
	if(fab) {
		koord fabpos = fab->get_pos().get_2d();
		karte_to_screen( fabpos );
		fabpos += pos;
		const vector_tpl<koord>& lieferziele = event_get_last_control_shift() & 1 ? fab->get_suppliers() : fab->get_lieferziele();
		FOR(vector_tpl<koord>, lieferziel, lieferziele) {
			const fabrik_t * fab2 = fabrik_t::get_fab(welt, lieferziel);
			if (fab2) {
				karte_to_screen( lieferziel );
				const koord end = lieferziel+pos;
				display_direct_line(fabpos.x, fabpos.y, end.x, end.y, colour);
				display_fillbox_wh_clip(end.x, end.y, 3, 3, ((welt->get_zeit_ms() >> 10) & 1) == 0 ? COL_RED : COL_WHITE, true);

				koord boxpos = end + koord(10, 0);
				const char * name = translator::translate(fab2->get_name());
				int name_width = proportional_string_width(name)+8;
				boxpos.x = clamp( boxpos.x, pos.x, pos.x+get_groesse().x-name_width );
				display_ddd_proportional_clip(boxpos.x, boxpos.y, name_width, 0, 5, COL_WHITE, name, true);
			}
		}
	}
	return fab;
}


// show the schedule on the minimap
void reliefkarte_t::set_current_cnv( convoihandle_t c )
{
	current_cnv = c;
	schedule_cache.clear();
	stop_cache.clear();
	colore = 0;
	counter_rail = 0;
	add_to_schedule_cache( current_cnv, true );
	last_schedule_counter = welt->get_schedule_counter()-1;
}


// draw the map
void reliefkarte_t::zeichnen(koord pos)
{
	// sanity check, needed for overlarge maps
	if(  (new_off.x|new_off.y)<0  ) {
		new_off = cur_off;
	}
	if(  (new_size.x|new_size.y)<0  ) {
		new_size = cur_size;
	}

	if(  last_mode != mode  ) {
		needs_redraw = true;
		last_schedule_counter = welt->get_schedule_counter()-1;
		last_mode = mode;
	}

	if(  needs_redraw  ||  cur_off!=new_off  ||  cur_size!=new_size  ) {
		calc_map();
		needs_redraw = false;
	}

	if(relief==NULL) {
		return;
	}

	if(  mode==MAP_PAX_DEST  &&  city!=NULL  ) {
		const unsigned long current_pax_destinations = city->get_pax_destinations_new_change();
		if(  pax_destinations_last_change > current_pax_destinations  ) {
			// new month started.
			calc_map();
		}
		else if(  pax_destinations_last_change < current_pax_destinations  ) {
			// new pax_dest in city.
			const sparse_tpl<uint8> *pax_dests = city->get_pax_destinations_new();
			koord pos, min, max;
			uint8 color;
			for(  uint16 i = 0;  i < pax_dests->get_data_count();  i++  ) {
				pax_dests->get_nonzero( i, pos, color );
				min = koord((pos.x*welt->get_groesse_x())/PAX_DESTINATIONS_SIZE,
				            (pos.y*welt->get_groesse_y())/PAX_DESTINATIONS_SIZE);
				max = koord(((pos.x+1)*welt->get_groesse_x())/PAX_DESTINATIONS_SIZE,
				            ((pos.y+1)*welt->get_groesse_y())/PAX_DESTINATIONS_SIZE);
				for( pos.x = min.x;  pos.x < max.x;  pos.x++  ) {
					for( pos.y = min.y;  pos.y < max.y;  pos.y++  ) {
						set_relief_farbe(pos, color);
					}
				}
			}
		}
		pax_destinations_last_change = city->get_pax_destinations_new_change();
	}

	if(  (uint16)cur_size.x > relief->get_width()  ) {
		display_fillbox_wh_clip( pos.x+new_off.x+relief->get_width(), new_off.y+pos.y, 32767, relief->get_height(), COL_BLACK, true);
	}
	if(  (uint16)cur_size.y > relief->get_height()  ) {
		display_fillbox_wh_clip( pos.x+new_off.x, pos.y+new_off.y+relief->get_height(), 32767, 32767, COL_BLACK, true);
	}
	display_array_wh( cur_off.x+pos.x, new_off.y+pos.y, relief->get_width(), relief->get_height(), relief->to_array());

	if(  !current_cnv.is_bound()  &&  (
		mode == MAP_LINES  ||  (is_show_schedule  &&  (mode == MAP_PASSENGER  ||  mode == MAP_MAIL  ||  mode == MAP_FREIGHT)  )
		)  ) {
		vector_tpl<linehandle_t> linee;

		if(  last_schedule_counter != welt->get_schedule_counter()  ) {
			// rebuilt cache
			last_schedule_counter = welt->get_schedule_counter();
			schedule_cache.clear();
			stop_cache.clear();
			colore = 0;
			counter_rail = 0;

			for(  int np = 0;  np < MAX_PLAYER_COUNT;  np++  ) {
				//cycle on players
				if(  welt->get_spieler( np )  &&  welt->get_spieler( np )->simlinemgmt.get_line_count() > 0   ) {

					welt->get_spieler( np )->simlinemgmt.get_lines( simline_t::line, &linee );
					for(  uint32 j = 0;  j < linee.get_count();  j++  ) {
						//cycle on lines

						if(  mode!=MAP_LINES  ) {
							// does this line has a matching freight
							if(  mode==MAP_PASSENGER  ) {
								if(  !linee[j]->get_goods_catg_index().is_contained( warenbauer_t::INDEX_PAS )  ) {
									// no pasengers
									continue;
								}
							}
							else if(  mode==MAP_MAIL  ) {
								if(  !linee[j]->get_goods_catg_index().is_contained( warenbauer_t::INDEX_MAIL )  ) {
									// no pasengers
									continue;
								}
							}
							else {
								uint8 i=0;
								for(  ;  i < linee[j]->get_goods_catg_index().get_count();  i++  ) {
									if(  linee[j]->get_goods_catg_index()[i] > warenbauer_t::INDEX_NONE  ) {
										break;
									}
								}
								if(  i >= linee[j]->get_goods_catg_index().get_count()  ) {
									// not any freight here ...
									continue;
								}
							}
						}

						// ware matches; now find at least a running convoi on this line ...
						convoihandle_t cnv;
						for(  int k = 0;  k < linee[j]->count_convoys();  k++  ) {
							convoihandle_t test_cnv = linee[j]->get_convoy(k);
							if(  test_cnv.is_bound()  ) {
								int state = test_cnv->get_state();
								if( state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
									cnv = test_cnv;
									break;
								}
							}
						}
						if(  !cnv.is_bound()  ) {
							continue;
						}
						int state = cnv->get_state();
						if( state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
							add_to_schedule_cache( cnv, false );
						}
					}
				}
			}

			// now add all unboad convois
			FOR( vector_tpl<convoihandle_t>, cnv, welt->convoys() ) {
				if(  !cnv.is_bound()  ||  cnv->get_line().is_bound()  ) {
					// not there or already part of a line
					continue;
				}
				int state = cnv->get_state();
				if( state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
					if(  mode!=MAP_LINES  ) {
						// does this line has a matching freight
						if(  mode==MAP_PASSENGER  ) {
							if(  !cnv->get_goods_catg_index().is_contained( warenbauer_t::INDEX_PAS )  ) {
								// no pasengers
								continue;
							}
						}
						else if(  mode==MAP_MAIL  ) {
							if(  !cnv->get_goods_catg_index().is_contained( warenbauer_t::INDEX_MAIL )  ) {
								// no pasengers
								continue;
							}
						}
						else {
							uint8 i=0;
							for(  ;  i < cnv->get_goods_catg_index().get_count();  i++  ) {
								if(  cnv->get_goods_catg_index()[i] > warenbauer_t::INDEX_NONE  ) {
									break;
								}
							}
							if(  i >= cnv->get_goods_catg_index().get_count()  ) {
								// not any freight here ...
								continue;
							}
						}
					}
					add_to_schedule_cache( cnv, false );
				}
			}
		}

	}
	//end MAP_LINES

	bool showing_schedule = false;
	if(  is_show_schedule  ||  mode==MAP_LINES  ) {
		showing_schedule = !schedule_cache.empty();
	}
	else {
		schedule_cache.clear();
		stop_cache.clear();
		colore = 0;
		counter_rail = 0;
		last_schedule_counter = welt->get_schedule_counter()-1;
	}

	// draw city limit
	if(  mode==MAP_CITYLIMIT  ) {

		const PLAYER_COLOR_VAL color = 127;

		// get city list
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		// for all cities
		FOR(weighted_vector_tpl<stadt_t*>, const stadt, staedte) {
			koord k[4];
			k[0] = stadt->get_linksoben(); // top left
			k[2] = stadt->get_rechtsunten(); // bottom right

			// calculate and draw the rotated coordinates

			k[1] =  koord(k[0].x, k[2].y); // bottom left
			k[3] =  koord(k[2].x, k[0].y); // top right

			k[0] += koord(0, -1); // top left
			karte_to_screen(k[0]);
			k[0] = k[0] + pos;

			karte_to_screen(k[1]); // bottom left
			k[1] = k[1] + pos;

			k[2] += koord(1, 0); // bottom right
			karte_to_screen(k[2]);
			k[2] += pos;

			k[3] += koord(1, -1); // top right
			karte_to_screen(k[3]);
			k[3] += pos;

			display_direct_line(k[0].x, k[0].y, k[1].x, k[1].y, color);
			display_direct_line(k[1].x, k[1].y, k[2].x, k[2].y, color);
			display_direct_line(k[2].x, k[2].y, k[3].x, k[3].y, color);
			display_direct_line(k[3].x, k[3].y, k[0].x, k[0].y, color);
		}
	}

	int offset = 0;
	koord last_start(0,0), last_end(0,0);
	if(  showing_schedule  ) {
		// white background
		display_blend_wh( cur_off.x+pos.x, new_off.y+pos.y, relief->get_width(), relief->get_height(), COL_WHITE, 75 );

		// DISPLAY STATIONS AND AIRPORTS: moved here so station spots are not overwritten by lines drawn
		FOR(  vector_tpl<line_segment_t>, seg, schedule_cache  ) {

			COLOR_VAL color = seg.colorcount;
			if(  event_get_last_control_shift()==2  ) {
				// on control use only player colors
				color = seg.sp->get_player_color1()+1;
			}
			koord k1(seg.start);
			karte_to_screen( k1 );
			k1 += pos;
			koord k2(seg.end);
			karte_to_screen( k2 );
			k2 += pos;
			if(  last_start==k1  &&  last_end==k2  ) {
				offset += 4;
			}
			else {
				offset = 0;
			}
			last_start = k1;
			last_end = k2;
			// may be other order, when isometric ...
			bool diagonal = seg.start_diagonal;
			if(  k1.x>k2.x  ) {
				koord temp = k1;
				k1 = k2;
				k2 = temp;
				diagonal ^= 1;
			}
			// shift start and end offset correctly
			if(  offset  ) {
				koord dir1 = koord( ribi_t::rotate90l( ribi_typ( k1, k2 ) ) );
				koord dir2 = k1.x!=k2.x ? koord( 1, 0 ) : koord( 0, 1 );
				if(  diagonal  ) {
					k1 += dir2*offset;
					k2 += dir1*offset;
				}
				else {
					k1 += dir1*offset;
					k2 += dir2*offset;
				}
			}
			// and finally draw ...
			line_segment_draw( seg.fpl->get_waytype(), k1, k2, diagonal, color );
		}

		//DISPLAY STATIONS AND AIRPORTS: moved here so station spots are not overwritten by lines drawn
		FOR(  vector_tpl<halthandle_t>, station, stop_cache  ) {

			const int stype = station->get_station_type();
			const COLOR_VAL color = station->get_besitzer()->get_player_color1()+1;

			koord temp_stop = station->get_basis_pos();
			karte_to_screen( temp_stop );
			temp_stop = temp_stop + pos;

			// invalid=0, loadingbay=1, railstation = 2, dock = 4, busstop = 8, airstop = 16, monorailstop = 32, tramstop = 64, maglevstop=128, narrowgaugestop=256
			if(  stype > 0  ) {
				if(  stype & ~(haltestelle_t::loadingbay | haltestelle_t::busstop | haltestelle_t::tramstop)  ) {
					// pax or mail exchange => larger
					if(  station->is_transfer( 0 )  ||  station->is_transfer( 1 )  ) {
						display_filled_circle( temp_stop.x, temp_stop.y, 5, color+3 );
					}
					else {
						display_filled_circle( temp_stop.x, temp_stop.y, 3, color+3 );
					}
				}
				else {
					display_fillbox_wh_clip ( temp_stop.x - 1, temp_stop.y - 1, 3, 3, color+3, false );
				}

				if(  stype & haltestelle_t::airstop  ) {
					display_airport( temp_stop.x, temp_stop.y, color+3 );
				}

				if(  stype & haltestelle_t::dock  ) {
					display_harbor( temp_stop.x, temp_stop.y, color+3 );
				}
			}

			if(  koord_distance( last_world_pos, station->get_basis_pos() ) <= 2  ) {
				// draw stop name with an index if close to mouse
				display_ddd_proportional_clip( temp_stop.x + 10, temp_stop.y + 7, proportional_string_width( station->get_name() ) + 8, 0, color+1, COL_WHITE, station->get_name(), true );
			}
		}

	}

	// if we do not do this here, vehicles would erase the town names
	// ADD: if CRTL key is pressed, temporary show the name
	if(  mode==MAP_TOWN  ||  event_get_last_control_shift()==2  ) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		const COLOR_VAL col = showing_schedule ? COL_BLACK : COL_WHITE;

		FOR( weighted_vector_tpl<stadt_t*>, const stadt, staedte ) {
			koord p = stadt->get_pos();
			const char * name = stadt->get_name();

			int w = proportional_string_width(name);
			karte_to_screen( p );
			p.x = clamp( p.x, 0, get_groesse().x-w );
			p += pos;
			display_proportional_clip( p.x, p.y, name, ALIGN_LEFT, col, true );
		}
	}

	// zoom/resize "selection box" in map
	// this must be rotated by 45 degree (sin45=cos45=0,5*sqrt(2)=0.707...)
	const sint16 raster=get_tile_raster_width();

	// calculate and draw the rotated coordinates
	koord ij = welt->get_world_position();
	const koord diff = koord( display_get_width()/(2*raster), display_get_height()/raster );

	koord view[4];
	view[0] = ij + koord( -diff.y+diff.x, -diff.y-diff.x );
	view[1] = ij + koord( -diff.y-diff.x, -diff.y+diff.x );
	view[2] = ij + koord( diff.y-diff.x, diff.y+diff.x );
	view[3] = ij + koord( diff.y+diff.x, diff.y-diff.x );
	for(  int i=0;  i<4;  i++  ) {
		karte_to_screen( view[i] );
		view[i] += pos;
	}
	for(  int i=0;  i<4;  i++  ) {
		display_direct_line( view[i].x, view[i].y, view[(i+1)%4].x, view[(i+1)%4].y, COL_YELLOW);
	}

	if(  !showing_schedule  ) {
		// Add factory name tooltips and draw factory connections, if on a factory
		const fabrik_t* const fab = (is_show_fab) ?
			draw_fab_connections(event_get_last_control_shift() & 1 ? COL_RED : COL_WHITE, pos)
			:
			fabrik_t::get_fab(welt, last_world_pos);

		if(fab) {
			koord fabpos = fab->get_pos().get_2d();
			karte_to_screen( fabpos );
			koord boxpos = fabpos + koord(10, 0);
			const char * name = translator::translate(fab->get_name());
			int name_width = proportional_string_width(name)+8;
			boxpos.x = clamp( boxpos.x, 0, 0+get_groesse().x-name_width );
			boxpos += pos;
			display_ddd_proportional_clip(boxpos.x, boxpos.y, name_width, 0, 10, COL_WHITE, name, true);
		}
	}
}


void reliefkarte_t::set_city( const stadt_t* _city )
{
	if(  city != _city  ) {
		city = _city;
		if(  _city  ) {
			pax_destinations_last_change = _city->get_pax_destinations_new_change();
		}
		calc_map();
	}
}

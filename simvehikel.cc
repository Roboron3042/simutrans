/*
 * simvehikel.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simvehikel.cc
 *
 * Fahrzeuge in der Welt von Simutrans
 *
 * 01.11.99  getrennt von simdings.cc
 *
 * Hansjoerg Malthaner, Nov. 1999
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "simvehikel.h"

#include "boden/grund.h"
#include "boden/wege/schiene.h"
#include "boden/wege/strasse.h"

#include "bauer/warenbauer.h"

#include "simworld.h"
#include "simdebug.h"

#include "simplay.h"
#include "simhalt.h"
#include "simconvoi.h"
#include "simsound.h"

#include "simimg.h"
#include "simcolor.h"
#include "simgraph.h"
#include "simio.h"
#include "simmem.h"

#include "simline.h"

#include "simintr.h"

#include "dings/wolke.h"
#include "dings/signal.h"
#include "dings/roadsign.h"

#include "gui/karte.h"

#include "besch/ware_besch.h"
#include "besch/skin_besch.h"
#include "besch/roadsign_besch.h"

#include "tpl/inthashtable_tpl.h"


#include "dataobj/fahrplan.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "utils/cstring_t.h"
#include "utils/simstring.h"
#include "utils/cbuffer_t.h"


#include "bauer/vehikelbauer.h"


static const uint8 offset_array[8] = {
//dir_sued, dir_west, dir_suedwest, dir_suedost, dir_nord, dir_ost, dir_nordost, dir_nordwest
0, 0, 0, 0, 1, 1, 1, 1
};


// Helferprozeduren


int get_freight_total(const slist_tpl<ware_t> *fracht)
{
  int menge = 0;

  slist_iterator_tpl<ware_t> iter(fracht);

  while(iter.next()) {
    menge += iter.get_current().menge;
  }

  return menge;
}


/**
 * Unload freight to halt
 * @return sum of unloaded goods
 * @author Hj. Malthaner
 */
int unload_freight(karte_t *welt,
		   halthandle_t halt,
		   slist_tpl<ware_t> *fracht,
		   const ware_besch_t *fracht_typ
		   )
{
  assert(halt.is_bound());

  int sum_menge = 0;

  // static wg. wiederverwendung der nodes
  static slist_tpl<ware_t> kill_queue;

  kill_queue.clear();

  if(halt->is_enabled( fracht_typ )) {
    if(!fracht->is_empty()) {

      slist_iterator_tpl<ware_t> iter (fracht);

      while(iter.next()) {
	ware_t tmp = iter.get_current();

	assert(tmp.gib_ziel() != koord::invalid);
	assert(tmp.gib_zwischenziel() != koord::invalid);

	halthandle_t end_halt = haltestelle_t::gib_halt(welt, tmp.gib_ziel());
	halthandle_t via_halt = haltestelle_t::gib_halt(welt, tmp.gib_zwischenziel());

	// probleme mit fehlerhafter ware
	// vielleicht wurde zwischendurch die
	// Zielhaltestelle entfernt ?

	if(!end_halt.is_bound() || !via_halt.is_bound()) {
	  DBG_MESSAGE("vehikel_t::entladen()",
		       "destination of %d %s is no longer reachable",
		       tmp.menge,
		       translator::translate(tmp.gib_name()));

	  kill_queue.insert(tmp);
	} else if(end_halt == halt || via_halt == halt) {

	  //		    printf("Liefere %d %s nach %s via %s an %s\n",
	  //                           tmp->menge,
	  //			   tmp->name(),
	  //			   end_halt->gib_name(),
	  //			   via_halt->gib_name(),
	  //			   halt->gib_name());

	  // hier sollte nur ordentliche ware verabeitet werden
	  int menge = halt->liefere_an(tmp);
	  sum_menge += menge;

	  kill_queue.insert(tmp);

	  INT_CHECK("simvehikel 937");
	}
      }
    }
  }


  slist_iterator_tpl<ware_t> iter (kill_queue);
  while( iter.next() ) {
    bool ok = fracht->remove(iter.get_current());
    assert(ok);
  }

  return sum_menge;
}


/**
 * Load freight from halt
 * @return loading successful?
 * @author Hj. Malthaner
 */
bool load_freight(karte_t * welt,
		  halthandle_t halt,
		  slist_tpl<ware_t> *fracht,
		  const vehikel_besch_t *besch,
		  fahrplan_t *fpl)
{
	const bool ok = halt->gibt_ab(besch->gib_ware());

	if( ok ) {
		int menge;

		while((menge = get_freight_total(fracht)) < besch->gib_zuladung()) {
			const int hinein = besch->gib_zuladung() - menge;

			ware_t ware = halt->hole_ab(besch->gib_ware(), hinein, fpl);
			if(ware.menge==0) {
				// now empty, but usually, we can get it here ...
				return ok;
			}

			const bool is_pax = (ware.gib_typ()==warenbauer_t::passagiere  ||  ware.gib_typ()==warenbauer_t::post);
			slist_iterator_tpl<ware_t> iter (fracht);

			// could this be joined with existing freight?
			while(iter.next()) {
				ware_t &tmp = iter.access_current();

				assert(tmp.gib_ziel() != koord::invalid);
				assert(ware.gib_ziel() != koord::invalid);

				// for pax: join according next stop
				// for all others we *must* use target coordinates
				if(tmp.gib_typ()==ware.gib_typ()  &&  (tmp.gib_zielpos()==ware.gib_zielpos()  ||  (is_pax   &&   haltestelle_t::gib_halt(welt,tmp.gib_ziel())==haltestelle_t::gib_halt(welt,ware.gib_ziel()))  )  ) {
					tmp.menge += ware.menge;
					ware.menge = 0;
					break;
				}
			}

			// if != 0 we could not joi it to existing => load it
			if(ware.menge != 0) {
				fracht->insert(ware);
			}

			INT_CHECK("simvehikel 876");
		}
	}

	return ok;
}



vehikel_basis_t::vehikel_basis_t(karte_t *welt):
    ding_t(welt)
{
}


vehikel_basis_t::vehikel_basis_t(karte_t *welt, koord3d pos):
    ding_t(welt, pos)
{
}


vehikel_basis_t::~vehikel_basis_t()
{
}


void
vehikel_basis_t::verlasse_feld()
{
	if( !welt->lookup(gib_pos())->obj_remove(this, gib_besitzer()) ) {
		// was not removed (not found?)

		dbg->error("vehikel_basis_t::verlasse_feld()","'%s' %p could not be removed from %d %d", gib_name(), this, gib_pos().x, gib_pos().y);
		DBG_MESSAGE("vehikel_basis_t::verlasse_feld()","checking all plan squares");

		koord k;
		bool ok = false;

		for(k.y=0; k.y<welt->gib_groesse_y(); k.y++) {
			for(k.x=0; k.x<welt->gib_groesse_x(); k.x++) {
				grund_t *gr = welt->lookup( k )->gib_boden_von_obj(this);

				if(gr && gr->obj_remove(this, gib_besitzer())) {
					dbg->warning("vehikel_basis_t::verlasse_feld()","remvoved vehicle '%s' %p from %d %d",gib_name(), this, k.x, k.y);
					ok = true;
				}
			}
		}

		if(!ok) {
			dbg->error("vehikel_basis_t::verlasse_feld()","'%s' %p was not found on any map sqaure!",gib_name(), this);
		}
	}
}


void vehikel_basis_t::betrete_feld()
{
	grund_t * gr = welt->lookup(gib_pos());
	gr->obj_add(this);
}


/**
 * Checks if this vehicle must change the square upon next move
 * @author Hj. Malthaner
 */
bool vehikel_basis_t::is_about_to_hop() const
{
    const int neu_xoff = gib_xoff() + gib_dx();
    const int neu_yoff = gib_yoff() + gib_dy();

    const int y_off_2 = 2*neu_yoff;
    const int c_plus  = y_off_2 + neu_xoff;
    const int c_minus = y_off_2 - neu_xoff;

    return ! (c_plus < 32 && c_minus < 32 && c_plus > -32 && c_minus > -32);
}


void vehikel_basis_t::fahre()
{
    const int neu_xoff = gib_xoff() + gib_dx();
    const int neu_yoff = gib_yoff() + gib_dy();

    const int li = -16;
    const int re = 16;
    const int ob = -8;
    const int un = 8;

    // kann vehikel das naechste feld betreten ?
    if (is_about_to_hop()) {

	if( !hop_check() ) {

//	    printf("vehikel %p hop_check failed at (%i,%i,%i)\n", this,gib_pos().x,gib_pos().y,gib_pos().z);fflush(NULL);
	    return;
	}

	hop();
	int yoff = neu_yoff;
	int xoff = neu_xoff;

	if (yoff < 0) {
	    yoff = un;
	}
	else {
	    yoff = ob;
	}

	if (xoff < 0) {
	    xoff = re;
	}
	else {
	    xoff = li;
	}

	setze_xoff( xoff );
	setze_yoff( yoff );
    }
    else {
	setze_xoff( neu_xoff );
	setze_yoff( neu_yoff );
    }
}

ribi_t::ribi
vehikel_basis_t::calc_richtung(koord start, koord ende, sint8 &dx, sint8 &dy) const
{
    ribi_t::ribi richtung = ribi_t::keine;

    const int di = sgn(ende.x - start.x);
    const int dj = sgn(ende.y - start.y);

    if(dj < 0 && di == 0) {
	richtung = ribi_t::nord;
	dx = 2;
	dy = -1;
    } else if(dj > 0 && di == 0) {
	richtung = ribi_t::sued;
	dx = -2;
	dy = 1;
    } else if(di < 0 && dj == 0) {
	richtung = ribi_t::west;
	dx = -2;
	dy = -1;
    } else if(di >0 && dj == 0) {
	richtung = ribi_t::ost;
	dx = 2;
	dy = 1;
    } else if(di > 0 && dj > 0) {
	richtung = ribi_t::suedost;
	dx = 0;
	dy = 2;
    } else if(di < 0 && dj < 0) {
	richtung = ribi_t::nordwest;
	dx = 0;
	dy = -2;
    } else if(di > 0 && dj < 0) {
	richtung = ribi_t::nordost;
	dx = 4;
	dy = 0;
    } else {
	richtung = ribi_t::suedwest;
	dx = -4;
	dy = 0;
    }
    return richtung;
}

int
vehikel_basis_t::calc_height()
{
// hoff wird hier auf die Hoehe an stelle xoff,yoff
// gesetzt

    const koord3d pos ( gib_pos() );
    const int hoff_alt = gib_hoff();

    const planquadrat_t *plan = welt->lookup(pos.gib_2d());
    grund_t *gr = plan->gib_boden_in_hoehe(pos.z);
    if(gr==NULL) {
    	// may fail with planes ...
    	plan->gib_kartenboden();
    }
    int hoff = 0;


    switch(gr->gib_weg_hang()) {
    case 3:	// nordhang
    case 6:	// westhang
	hoff = -vehikel_basis_t::gib_yoff() - 8;
	break;
    case 9:	// osthang
    case 12:    // suedhang
	hoff = vehikel_basis_t::gib_yoff() - 8;
	break;
    }

    if(gr == plan->gib_kartenboden()) {
	if(gr->ist_bruecke() && gr->gib_weg_hang() == hang_t::flach) {
	    hoff = -16;
	}
	else if(gr->ist_tunnel()) {
	    hoff = 0;
		if(!gr->ist_im_tunnel()) {
			// need hiding?
			switch(gr->gib_grund_hang()) {
			    case 3:	// nordhang
			    	if(vehikel_basis_t::gib_yoff()>-7) {
			    		setze_bild(0, IMG_LEER);
			    	}
			    	else {
			    		calc_bild();
			    	}
			    	break;
			    case 6:	// westhang
			    	if(vehikel_basis_t::gib_xoff()>-12) {
			    		setze_bild(0, IMG_LEER);
			    	}
			    	else {
			    		calc_bild();
			    	}
				break;
			    case 9:	// osthang
			    	if(vehikel_basis_t::gib_xoff()<6) {
			    		setze_bild(0, IMG_LEER);
			    	}
			    	else {
			    		calc_bild();
			    	}
			    case 12:    // suedhang
			    	if(vehikel_basis_t::gib_yoff()<7) {
			    		setze_bild(0, IMG_LEER);
			    	}
			    	else {
			    		calc_bild();
			    	}
			}
		}
	}
    }
    // geschwindigkeit berechnen

	hoff = height_scaling(hoff);
    calc_akt_speed(hoff_alt, hoff);

    return hoff;
}


slist_tpl<const vehikel_t *> vehikel_t::list;	// Liste der Vehikel (alle !)


void
vehikel_t::setze_convoi(convoi_t *c)
{
    // c darf NULL sein, wenn das vehikel aus dem Convoi entfernt wird

    cnv = c;
}


void
vehikel_t::setze_offsets(int x, int y)
{
    setze_xoff( x );
    setze_yoff( y );
}


/**
 * Remove freight that no longer can reach it's destination
 * i.e. becuase of a changed schedule
 * @author Hj. Malthaner
 */
void vehikel_t::remove_stale_freight()
{
  DBG_DEBUG("vehikel_t::remove_stale_freight()", "called");

  // and now check every piece of ware on board,
  // if its target is somewhere on
  // the new schedule, if not -> remove
  static slist_tpl<ware_t> kill_queue;
  kill_queue.clear();


  if(!fracht.is_empty()) {
    slist_iterator_tpl<ware_t> iter (fracht);

    while(iter.next()) {
      fahrplan_t *fpl = cnv->gib_fahrplan();

      ware_t tmp = iter.get_current();
      bool found = false;

      for (int i = 0; i < fpl->maxi(); i++) {
	if (haltestelle_t::gib_halt(welt, fpl->eintrag.at(i).pos.gib_2d()) ==
	    haltestelle_t::gib_halt(welt, tmp.gib_zwischenziel())) {
	  found = true;
	  break;
	}
      }

      if (!found) {
	kill_queue.insert(tmp);
      }
    }

    slist_iterator_tpl<ware_t> killer (kill_queue);

    while(killer.next()) {
      fracht.remove(killer.get_current());
    }
  }
}


void
vehikel_t::play_sound() const
{
  if(besch->gib_sound() >= 0) {
    const koord pos ( gib_pos().gib_2d() );

    struct sound_info info;
    info.index = besch->gib_sound();
    info.volume = 255;
    info.pri = 0;

    welt->play_sound_area_clipped(pos, info);
  }
}


/**
 * Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
 * der Convoi eine neue Route ermittelt
 * @author Hj. Malthaner
 */
void vehikel_t::neue_fahrt()
{
  route_index = 1;
}


void vehikel_t::starte_neue_route(koord3d k0, koord3d k1)
{
  pos_prev = pos_cur = k0;
  pos_next = k1;

  alte_fahrtrichtung = fahrtrichtung;
  fahrtrichtung = calc_richtung(pos_prev.gib_2d(), pos_next.gib_2d(), dx, dy);

  hoff = 0;

  calc_bild();
}


vehikel_t::vehikel_t(karte_t *welt,
		     koord3d pos,
		     const vehikel_besch_t *besch,
		     spieler_t *sp) : vehikel_basis_t(welt, pos)
{
  this->besch = besch;

  setze_besitzer( sp );
  insta_zeit = welt->get_current_month();
  cnv = NULL;
  speed_limit = -1;

  route_index = 1;

  rauchen = true;
  fahrtrichtung = ribi_t::keine;

	current_friction = 4;
	sum_weight = besch->gib_gewicht();

  ist_erstes = ist_letztes = false;
  alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;

  setze_bild(0, besch->gib_basis_bild());

  list.insert( this );
  // printf("Erzeuge Vehikle %x, Start %d,%d\n",(unsigned)this, x,y);
}


vehikel_t::vehikel_t(karte_t *welt) :
    vehikel_basis_t(welt)
{
  rauchen = true;

  besch = NULL;
  cnv = NULL;

  route_index = 1;
	current_friction = 4;
	sum_weight = 10;

  alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;
}


bool
vehikel_t::hop_check()
{
	// check, ob strecke frei
	const grund_t *bd = welt->lookup(pos_next);
	if(bd==NULL  && gib_wegtyp()!=weg_t::luft) {
		// weg nicht existent -  wurde wohl abgerissen
		if(ist_erstes ) {
			// dann suchen wir eben einen anderen Weg!
			cnv->suche_neue_route();
		}
		return false;
	}

	if(ist_erstes) {
		if(ist_befahrbar(bd)) {
			int restart_speed = 1;

			// ist_weg_frei() berechnet auch die Geschwindigkeit
			// mit der spaeter weitergefahren wird
			if(!ist_weg_frei(restart_speed)) {

				// convoi anhalten, wenn strecke nicht frei
				cnv->warten_bis_weg_frei(restart_speed);

				// nicht weiterfahren
				return false;
			}
			return true;
		}
		else {
			// weg nicht befahrbar  -  wurde wohl abgerissen
			// dann suchen wir eben einen anderen Weg!
			cnv->suche_neue_route();
		}
		return false;
	}
	return true;
}


void
vehikel_t::ziel_erreicht()
{
  // printf("%d %d\n",n, route.gib_max_n());

  if(ist_erstes) {
    cnv->ziel_erreicht(this);	// Ja, bereit melden
  }
}


void
vehikel_t::verlasse_feld()
{
    vehikel_basis_t::verlasse_feld();

    if(ist_letztes) {
        reliefkarte_t::gib_karte()->recalc_relief_farbe(gib_pos().gib_2d());
    }
}

void
vehikel_t::betrete_feld()
{
    vehikel_basis_t::betrete_feld();

    if(ist_erstes) {
        reliefkarte_t::gib_karte()->setze_relief_farbe(gib_pos().gib_2d(), VEHIKEL_KENN);
    }
}


void
vehikel_t::hop()
{
  //    printf("vehikel %p hop\n", this);

  if(cnv) {
    // Fahrtkosten
    cnv->add_running_cost(-besch->gib_betriebskosten());
  }

  verlasse_feld();

  route_index ++;

  pos_prev = pos_cur;
  pos_cur = pos_next;  // naechstes Feld
  pos_next = cnv->advance_route(route_index);

	alte_fahrtrichtung = fahrtrichtung;
	// this is a required hack for aircrafts! Aircrafts can turn on a single square, and this confuses the previous calculation!
	// author: hsiegeln
	if (pos_prev.gib_2d() == pos_next.gib_2d()) {
		fahrtrichtung = calc_richtung(pos_cur.gib_2d(), pos_next.gib_2d(), dx, dy);
	}
	else {
		fahrtrichtung = calc_richtung(pos_prev.gib_2d(), pos_next.gib_2d(), dx, dy);
	}

  calc_bild();

  welt->markiere_dirty(gib_pos());
  setze_pos( pos_cur );


  const weg_t * weg = welt->lookup(gib_pos())->gib_weg(gib_wegtyp());
  setze_speed_limit( weg ? kmh_to_speed(weg->gib_max_speed()) : -1 );


  /*
  printf("Neue Route.pos %d,%d,%d\n",
	 pos_next.x,
	 pos_next.y,
	 pos_next.z);
  */

  betrete_feld();
}


void
vehikel_t::setze_speed_limit(int l)
{
    speed_limit = l;

    if(speed_limit != -1 && cnv->gib_akt_speed() > speed_limit) {
	cnv->setze_akt_speed_soll(speed_limit);
    }
}



/* calculates the current friction coefficient based on the curent track
 * falt, slope, curve ...
 * @author prissi, HJ
 */
void
vehikel_t::calc_akt_speed(const int h_alt, const int h_neu)
{
	// even track
	current_friction = 1;
	// or a hill?
	if(h_neu != h_alt) {
		if(h_neu < h_alt) {
			// hill up, since height offsets are negative: heavy deccelerate
			current_friction = 64;
		}
		else {
			// hill down: accelrate
			current_friction = -32;
		}
	}

	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) {
		current_friction = 8;
	}

	if(ist_erstes) {
		// just to accelerate: The actual speed takes care of all vehicles in the convoi
 	        const int akt_speed = gib_speed();
	        if(speed_limit != -1 && akt_speed > speed_limit) {
		  cnv->setze_akt_speed_soll(speed_limit);
		} else {
		  cnv->setze_akt_speed_soll(akt_speed);
		}
	}
}



void
vehikel_t::rauche()
{
  // raucht ueberhaupt ?
  if(rauchen && besch->gib_rauch()) {

    // printf("%d %d\n", cnv->gib_akt_speed(), speed);

    // Hajo: only produce smoke when heavily accelerating
    //       or steam engine
    int akt_speed = gib_speed();
    if(speed_limit != -1 && akt_speed > speed_limit) {
      akt_speed = speed_limit;
    }

    if(cnv->gib_akt_speed() < (akt_speed >> 1) ||
       besch->get_engine_type() == vehikel_besch_t::steam) {

      grund_t * gr = welt->lookup( pos_cur );
      // nicht im tunnel ?
      if(gr && !gr->ist_im_tunnel() ) {
	sync_wolke_t *abgas =  new sync_wolke_t(welt,
						pos_cur,
						gib_xoff(),
						gib_yoff(),
						besch->gib_rauch()->gib_bild_nr(0));

	if( ! gr->obj_pri_add(abgas, PRI_MITTEL) ) {
	  delete abgas;
	} else {
	  welt->sync_add( abgas );
	}
      }
    }
  }
}


void
vehikel_t::fahre()
{
    vehikel_basis_t::fahre();

    if(ist_erstes) {
	// testen, ob ziel erreicht
	if(pos_next == pos_cur) {
	    if(dy < 0) {
		if(gib_yoff() <= 0) {
		    ziel_erreicht();
		}
	    } else {
		if(gib_yoff() >= 5) {
		    ziel_erreicht();
		}
	    }
	}
    }
//    printf("vehikel %p fahre\n", this);
}


/**
 * Payment is done per hop. It iterates all goods and calculates
 * the income for the last hop. This method must be called upon
 * every stop.
 * @return income total for last hop
 * @author Hj. Malthaner
 */
int vehikel_t::calc_gewinn(koord3d start, koord3d end) const
{
    const long dist = abs(end.x - start.x) + abs(end.y - start.y);

    const long ref_speed = welt->get_average_speed( gib_wegtyp() );
    const long speed_base = speed_to_kmh(cnv->gib_min_top_speed()) - ref_speed;

    sint64 value = 0;
    slist_iterator_tpl <ware_t> iter (fracht);

    while( iter.next() ) {
      const ware_t & ware = iter.get_current();

#if 0
      double price = (ware.gib_typ()->gib_preis() * dist * ware.menge) / 3.0;

      // Hajo: add speed bonus
      price += 0.001 * price * (speed_base * ware.gib_typ()->gib_speed_bonus());
      // Hajo: sum up new price
      value += price;
    }

    // Hajo: Rounded value, in cents
    return (int)(value+0.5);
#else
		// prissi
		const sint32 grundwert128 = ware.gib_typ()->gib_preis()<<7;
		const sint32 grundwert_bonus = (ware.gib_typ()->gib_preis()*(1000l+speed_base*ware.gib_typ()->gib_speed_bonus()));
		const sint32 price = (grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus ) * dist * ware.menge;

		// sum up new price
		value += price;
	}

    // Hajo: Rounded value, in cents
    // prissi: Why on earth 1/3???
    return (value+1500l)/3000l;
#endif
}


const char *vehikel_t::gib_fracht_mass() const
{
    return gib_fracht_typ()->gib_mass();
};


int vehikel_t::gib_fracht_menge() const
{
  return get_freight_total(&fracht);
}


/**
 * Berechnet Gesamtgewicht der transportierten Fracht in KG
 * @author Hj. Malthaner
 */
int vehikel_t::gib_fracht_gewicht() const
{
  int weight = 0;

  slist_iterator_tpl<ware_t> iter(fracht);

  while(iter.next()) {
    weight +=
      iter.get_current().menge *
      iter.get_current().gib_typ()->gib_weight_per_unit();
  }

  return weight;
}


const char * vehikel_t::gib_fracht_name() const
{
    return gib_fracht_typ()->gib_name();
}


void vehikel_t::gib_fracht_info(cbuffer_t & buf)
{
    if(fracht.is_empty()) {
	buf.append("  ");
	buf.append(translator::translate("leer"));
	buf.append("\n");
    } else {

	slist_iterator_tpl<ware_t> iter (fracht);

	while(iter.next()) {
	    ware_t ware = iter.get_current();
	    const char * name = "Error in Routing";

	    halthandle_t halt = haltestelle_t::gib_halt(welt, ware.gib_ziel());
	    if(halt.is_bound()) {
		name = halt->gib_name();
	    }

	    buf.append("   ");
	    buf.append(ware.menge);
	    buf.append(translator::translate(ware.gib_mass()));
	    buf.append(" ");
	    buf.append(translator::translate(ware.gib_name()));
	    buf.append(" > ");
	    buf.append(name);
	    buf.append("\n");
	}
    }
}


void
vehikel_t::loesche_fracht()
{
    fracht.clear();
}


bool
vehikel_t::beladen(koord , halthandle_t halt)
{
	const bool ok= load_freight(welt, halt, &fracht, besch, cnv->gib_fahrplan());
	sum_weight =  (gib_fracht_gewicht()+499)/1000 + besch->gib_gewicht();

	// bild hat sich ge�ndert
	// set_flag(dirty);
	if(ok) {
		calc_bild();
	}
	return ok;
}


/**
 * fahrzeug an haltestelle entladen
 * @author Hj. Malthaner
 */
void vehikel_t::entladen(koord, halthandle_t halt)
{
	// printf("Vehikel %p entladen\n", this);
	int menge = unload_freight(welt, halt, &fracht, gib_fracht_typ());
	// add delivered goods to statistics
	cnv->book(menge, CONVOI_TRANSPORTED_GOODS);
	// add delivered goods to halt's statistics
	halt->book(menge, HALT_ARRIVED);
	// recalculate vehicles load (here is enough, because this routine is alsways called after beladen!?
	sum_weight =  (gib_fracht_gewicht()+499)/1000 + besch->gib_gewicht();
}


void vehikel_t::sync_step()
{
  // ohne convoi macht der step keinen Sinn
  assert(cnv != NULL);

  // printf("vehikel %p sync_step\n", this);

  // Funktionsaufruf vermeiden, wenn m�glich
  // if ist schneller als Aufruf!
  if(hoff) {
    setze_yoff( gib_yoff() - hoff );
  }

  fahre();

  hoff = calc_height();

  // Funktionsaufruf vermeiden, wenn m�glich
  // if ist schneller als Aufruf!
  if(hoff) {
    setze_yoff( gib_yoff() + hoff );
  }
}



/**
 * Ermittelt fahrtrichtung
 * @author Hj. Malthaner
 */
ribi_t::ribi
vehikel_t::richtung()
{
  ribi_t::ribi neu = calc_richtung(pos_prev.gib_2d(),
				   pos_next.gib_2d(),
				   dx, dy);

  if(neu == ribi_t::keine) {
    // sonst ausrichtung des Vehikels beibehalten
    return fahrtrichtung;
  } else {
    return neu;
  }
}


void
vehikel_t::rdwr(loadsave_t *file)
{
    int fracht_count = fracht.count();
    if(fracht_count==0) {
    	fracht_count++;
   }

    ding_t::rdwr(file);

	if(file->get_version()<86006) {
		// parameter werden in der deklarierten reihenfolge gespeichert
		long l;
		file->rdwr_long(insta_zeit, "\n");
		file->rdwr_long(l, " ");
		dx = l;
		file->rdwr_long(l, "\n");
		dy = l;
		file->rdwr_long(l, "\n");
		hoff = l;
		file->rdwr_long(speed_limit, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_enum(alte_fahrtrichtung, "\n");
		file->rdwr_delim("Wre: ");
		file->rdwr_long(fracht_count, " ");
		file->rdwr_long(l, "\n");
		route_index = l;
		if(file->is_loading()) {
			long zeit = (welt->gib_zeit_ms()-insta_zeit) >> karte_t::ticks_bits_per_tag;
//DBG_MESSAGE("vehicle_t::rdwr()","bought at tick count %i i.e. %i month(s) before.",insta_zeit,zeit);
			insta_zeit = welt->get_current_month()-zeit;
		}
	}
	else {
		// prissi: changed several data types to save runtime memory
		file->rdwr_long(insta_zeit, "\n");
		file->rdwr_byte(dx, " ");
		file->rdwr_byte(dy, "\n");
		file->rdwr_short(hoff, "\n");
		file->rdwr_long(speed_limit, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_enum(alte_fahrtrichtung, "\n");
		file->rdwr_delim("Wre: ");
		file->rdwr_long(fracht_count, " ");
		file->rdwr_short(route_index, "\n");
	}

    pos_prev.rdwr(file);
    pos_cur.rdwr(file);
    pos_next.rdwr(file);

    const char *s = NULL;

    if(file->is_saving()) {
	s = besch->gib_name();
    }
    file->rdwr_str(s, " ");
    if(file->is_loading()) {
	besch = vehikelbauer_t::gib_info(s);

	if(besch == 0) {
	  dbg->warning("vehikel_t::rdwr()","no vehicle pak for '%s' search for something similar", s);
	}
	guarded_free(const_cast<char *>(s));
    }

    if(file->is_saving()) {
    	  if(fracht.count()==0) {
    	  	// create dummy freight for savegame compatibility
    	  	ware_t ware( besch->gib_ware() );
    	  	ware.menge = 0;
    	  	ware.max = besch->gib_zuladung();
    	  	ware.setze_ziel( gib_pos().gib_2d() );
    	  	ware.setze_zwischenziel( gib_pos().gib_2d() );
    	  	ware.setze_zielpos( gib_pos().gib_2d() );
    	  	ware.rdwr(file);
    	  }
        slist_iterator_tpl<ware_t> iter(fracht);
        while(iter.next()) {
	    ware_t ware = iter.get_current();
	    ware.rdwr(file);
        }
    }
    else {
        for(int i=0; i<fracht_count; i++) {
	    ware_t ware(file);
	    if(ware.menge>0) {
		    fracht.insert(ware);
	}
  }
    }

    file->rdwr_bool(ist_erstes, " ");
    file->rdwr_bool(ist_letztes, " ");

    if(file->is_loading()) {
        list.insert( this );
        if(besch) {
		calc_bild();
	// full weight after loading
		sum_weight =  (gib_fracht_gewicht()+499)/1000 + besch->gib_gewicht();
	}
    }
}


int vehikel_t::calc_restwert() const
{
	// after 20 year, it has only half value
    return (int)((double)besch->gib_preis() * pow(0.997, (welt->get_current_month() - gib_insta_zeit())));
}




void
vehikel_t::zeige_info()
{
    if(cnv != NULL) {
	cnv->zeige_info();
    } else {
	dbg->warning("vehikel_t::zeige_info()",
	             "cnv is null, can't open convoi window!");
    }
}


void vehikel_t::info(cbuffer_t & buf) const
{
  if(cnv) {
    cnv->info(buf);
  }
}


/**
 * debug info into buffer
 * @author Hj. Malthaner
 */
char * vehikel_t::debug_info(char *buf) const
{
  buf += sprintf(buf, "ist_erstes = %d, ist_letztes = %d\n",
		 ist_erstes, ist_letztes);

  return buf;
}


/**
 * Debug info nach stderr
 * @author Hj. Malthaner
 * @date 26-Aug-03
 */
void vehikel_t::dump() const
{
  char buf[16000];

  debug_info(buf);

  fprintf(stderr, buf);
}



const char *
vehikel_t::ist_entfernbar(const spieler_t *)
{
    return "Fahrzeuge koennen so nicht entfernt werden";
}


/**
 * Destructor. Frees aggregated members.
 * @author Hj. Malthaner
 */
vehikel_t::~vehikel_t()
{
  const koord3d k ( gib_pos() );

  if(welt->lookup(k) &&
     welt->lookup(k)->obj_ist_da(this) ) {

    // Hajo: Everything is ok,
    // the destructor of the base class will remove the object
    // from the map

  }

  list.remove(this);
}


/*--------------------------- Fahrdings ------------------------------*/


automobil_t::automobil_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cn) :
    vehikel_t(welt, pos, besch, sp)
{
    cnv = cn;
}

automobil_t::automobil_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
    rdwr(file, true);
}


ribi_t::ribi
automobil_t::gib_ribi(const grund_t *gr) const
{
	weg_t *weg = gr->gib_weg(weg_t::strasse);

	if(weg) {
		return weg->gib_ribi();
	}
	return ribi_t::keine;
}


bool
automobil_t::ist_befahrbar(const grund_t *bd) const
{
	if(bd->gib_weg(weg_t::strasse)==NULL) {
		return false;
	}
	// check for signs
	const roadsign_t *rs = dynamic_cast<roadsign_t *>(bd->obj_bei(0));
	if(rs!=NULL) {
		if(rs->gib_besch()->gib_min_speed()>0  &&  rs->gib_besch()->gib_min_speed()>kmh_to_speed(gib_besch()->gib_geschw())) {
			return false;
		}
	}
	return true;
}



// how expensive to go here (for way search)
// author prissi
int
automobil_t::gib_kosten(const grund_t *gr,const uint32 max_speed) const
{
	int costs;

	// first favor faster ways
	const weg_t *w=gr->gib_weg(weg_t::strasse);
	uint32 max_tile_speed = w ? w->gib_max_speed() : 999;

	// add cost for going (with maximum spoeed, cost is 1 ...
	costs = ( (max_speed<=max_tile_speed) ? 4 :  (max_speed*4+max_tile_speed)/max_tile_speed );

	// assume all traffic (and even road signs etc.) is not good ...
	costs += gr->obj_count();

	// effect of hang ...
	if(gr->gib_weg_hang()!=0) {
		// we do not need to check whether up or down, since everything that goes up must go down
		// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
		costs += 15;
	}
	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool automobil_t::ist_ziel(const grund_t *gr) const
{
	//  just check, if we reached a free stop position of this halt
	if(gr->gib_halt()==target_halt) {
		if(gr->suche_obj(ding_t::automobil)==NULL) {
//DBG_MESSAGE("is_target()","success at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
			return true;
		}
	}
	return false;
}


bool
automobil_t::ist_weg_frei(int &restart_speed)
{
	const grund_t *gr = welt->lookup( pos_next );
	if(gr==NULL) {
		return false;
	}

	if(gr->obj_count()>200) {
		// too many cars here
		return false;
	}

	// pruefe auf Schienenkreuzung
	if(gr->gib_weg(weg_t::schiene) && gr->gib_weg(weg_t::strasse)) {
		// das ist eine Kreuzung, ist sie frei ?

		if(gr->suche_obj_ab(ding_t::waggon,PRI_RAIL_AND_ROAD)) {
			restart_speed = 8;
			return false;
		}
	}

	// check for traffic lights (only relevant for the first car in a convoi)
	if(ist_erstes) {
		ding_t *dt = gr->obj_bei(0);
		if(dt  &&  dt->gib_typ()==ding_t::roadsign) {
			const roadsign_t *rs = (roadsign_t *)dt;
			// since at the corner, our direct may be diagonal, we make it straight
			const int richtung = ribi_typ(gib_pos().gib_2d(),pos_next.gib_2d());

			// check, if we reached a choose point
			if(rs->is_free_route(richtung)) {
				route_t *rt=cnv->get_route();
				grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));

				// is our target occupied?
				if(target  &&  target->gib_halt().is_bound()  &&  target->suche_obj(ding_t::automobil)) {
					// if we fail, we will wait in a step, much more simulation friendly
					if(cnv->get_state()!=convoi_t::WAITING_FOR_CLEARANCE) {
						restart_speed = 0;
						return false;
					}
					// check if there is a free position
					// this is much faster than waysearch
					target_halt = target->gib_halt();
					if(!target_halt->find_free_position(weg_t::strasse,ding_t::automobil)) {
						return false;
					}
					// now it make sense to search a route
					route_t target_rt;
					sint8 d;	//dummy
					if(!target_rt.find_route( welt, pos_next, this, 50, richtung, 33 )) {
						// nothing empty or not route with less than 33 tiles
						return false;
					}
DBG_MESSAGE("automobil_t::ist_weg_frei()","found free stop near %i,%i,%i",target_rt.position_bei(target_rt.gib_max_n()).x,target_rt.position_bei(target_rt.gib_max_n()).y, target_rt.position_bei(target_rt.gib_max_n()).z );
	//				rt->kopiere( &target_rt );
	//				rt->insert(pos_cur);
	//				route_index = 0;
					rt->remove_koord_from(route_index);
					rt->append( &target_rt );
				}
			}
			else if(rs->gib_besch()->is_traffic_light()  &&  (rs->get_dir()&richtung)==0) {
				setze_fahrtrichtung(richtung);
				// wait here
				return false;
			}
		}
	}

	// calculate new direction
	sint8 dx, dy;	// dummies
	const uint8 next_fahrtrichtung = this->calc_richtung(pos_cur.gib_2d(), pos_next.gib_2d(), dx, dy);

	bool frei = true;

	// suche vehikel
	const uint8 top = gr->gib_top();
	for(  uint8 pos=0;  pos<top  && frei;  pos++ ) {
		ding_t *dt = gr->obj_bei(pos);
		if(dt) {
			uint8 other_fahrtrichtung=255;

			// check for car
			if(dt->gib_typ()==ding_t::automobil) {
				automobil_t *at = (automobil_t *)dt;
				// check, if this is not ourselves
				if(at->cnv!=cnv) {
					other_fahrtrichtung = at->gib_fahrtrichtung();
				}
			}

			// check for city car
			if(dt->gib_typ()==ding_t::verkehr) {
				vehikel_basis_t *v = (vehikel_basis_t *)dt;
				other_fahrtrichtung = v->gib_fahrtrichtung();
			}

			// ok, there is another car ...
			if(other_fahrtrichtung!=255) {

				if(other_fahrtrichtung==next_fahrtrichtung  ||  other_fahrtrichtung==gib_fahrtrichtung() ) {
					// this goes into the same direction as we, so stopp and save a restart speed
					frei = false;
					restart_speed = 512;

				} else if(ribi_t::ist_orthogonal(other_fahrtrichtung,gib_fahrtrichtung() )) {

					// there is a car orthogonal to us
					frei = false;
					restart_speed = 8;
				}
			}
		}
	}

	return frei;
}


void
automobil_t::betrete_feld()
{
	// this will automatically give the right order
	grund_t *gr = welt->lookup( gib_pos() );

	uint8 offset;
	if(gr->gib_weg(weg_t::schiene)) {
		offset = (gib_fahrtrichtung()<4)^(umgebung_t::drive_on_left==false) ? PRI_ROAD_S_W_SW_SE : PRI_ROAD_AND_RAIL_N_E_NE_NW;
	}
	else {
		offset = (gib_fahrtrichtung()<4)^(umgebung_t::drive_on_left==false) ? PRI_ROAD_S_W_SW_SE : PRI_ROAD_N_E_NE_NW;
	}
	bool ok = (gr->obj_pri_add(this, offset) != 0);

	if(ist_erstes) {
		reliefkarte_t::gib_karte()->setze_relief_farbe(gib_pos().gib_2d(), VEHIKEL_KENN);
	}

	if(!ok) {
		dbg->error("automobil_t::betrete_feld()","vehicel '%s' %p could not be added to %d, %d, %d",gib_pos().x, gib_pos().y, gib_pos().z);
	}

	const int cargo = gib_fracht_menge();
	gr->gib_weg(weg_t::strasse)->book(cargo, WAY_STAT_GOODS);
	if (ist_erstes)  {
		gr->gib_weg(weg_t::strasse)->book(1, WAY_STAT_CONVOIS);
	}
}


void
automobil_t::calc_bild()
{
    if(welt->lookup(pos_cur) &&
       welt->lookup(pos_cur)->ist_im_tunnel() ) {	// tunnel ?
	setze_bild(0, IMG_LEER);
    } else {
	setze_bild(0, besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()),
					 fracht.is_empty()));
    }
}


fahrplan_t * automobil_t::erzeuge_neuen_fahrplan() const
{
  return new autofahrplan_t();
}


void
automobil_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	}
	else {
		rdwr(file, true);
	}
}

void
automobil_t::rdwr(loadsave_t *file, bool force)
{
    assert(force == true);

    if(file->is_saving() && cnv != NULL && !force) {
        file->wr_obj_id(-1);
    } else {
        vehikel_t::rdwr(file);
		// try to find a matching vehivle
		if(file->is_loading()  &&  besch==NULL) {
			const ware_besch_t *w= (fracht.count()>0) ? fracht.at(0).gib_typ() : warenbauer_t::passagiere;
			DBG_MESSAGE("automobil_t::rdwr()","try to find a fitting vehicle for %s.",  w->gib_name() );
			besch = vehikelbauer_t::vehikel_search(weg_t::strasse, 0xFFFFFFFFul, ist_erstes?50:0, speed_to_kmh(get_speed_limit()), w );
			if(besch) {
				DBG_MESSAGE("automobil_t::rdwr()","replaced by %s",besch->gib_name());
				// still wrong load ...
				calc_bild();
			}
		}
    }
}


ribi_t::ribi
waggon_t::gib_ribi(const grund_t *gr) const
{
    weg_t *weg = gr->gib_weg(weg_t::schiene);

    if(weg) {
	return weg->gib_ribi();
    }
    return ribi_t::keine;
}


bool
waggon_t::ist_befahrbar(const grund_t *bd) const
{
  const schiene_t * sch = dynamic_cast<const schiene_t *> (bd->gib_weg(weg_t::schiene));

  // Hajo: diesel and steam engines can use electrifed track as well.
  // also allow driving on foreign tracks ...
  const bool ok = sch != 0 && (besch->get_engine_type() == vehikel_besch_t::electric ? sch->ist_elektrisch() : true);

  return ok;
}




// how expensive to go here (for way search)
// author prissi
int
waggon_t::gib_kosten(const grund_t *gr,const uint32 max_speed) const
{
	// first favor faster ways
	const weg_t *w=gr->gib_weg(weg_t::schiene);
	uint32 max_tile_speed = w ? w->gib_max_speed() : 999;
	// add cost for going (with maximum spoeed, cost is 1 ...
	int costs = ( (max_speed<=max_tile_speed) ? 4 :  (max_speed*4+max_tile_speed)/max_tile_speed );

	// effect of hang ...
	if(gr->gib_weg_hang()!=0) {
		// we do not need to check whether up or down, since everything that goes up must go down
		// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
		costs += 25;
	}

	return costs;
}



bool
waggon_t::ist_blockwechsel(koord3d k1, koord3d k2) const
{
  const schiene_t * sch0 =
    (const schiene_t *) welt->lookup( k1 )->gib_weg(gib_wegtyp());

  const schiene_t * sch1 =
    (const schiene_t *) welt->lookup( k2 )->gib_weg(gib_wegtyp());

  return sch0->gib_blockstrecke() != sch1->gib_blockstrecke();
}


bool
waggon_t::ist_weg_frei(int & restart_speed)
{
	const grund_t *gr = welt->lookup( pos_next );

	if(gr->obj_count()>200) {
		// too many objects here
		return false;
	}

	if(!ist_blockwechsel(pos_cur, pos_next)) {
		restart_speed = -1;
		return true;
	}
	else {
		const schiene_t * sch1 = (const schiene_t *) welt->lookup( pos_next )->gib_weg(gib_wegtyp());

		bool frei = sch1->ist_frei();
		// ok, next is free; check for presignal
		if(frei) {
			signal_t * sig = sch1->gib_blockstrecke()->gib_signal_bei(pos_next);
			if(sig  &&  sig->gib_typ()==ding_t::presignal) {
				frei = frei && is_next_block_free((presignal_t *)sig);
			}
		}

		if(frei) {
			restart_speed = -1;
		}
		else {
			restart_speed = 8;
		}

		return frei;
	}
}

bool
waggon_t::is_next_block_free(presignal_t * sig) const
{
	const schiene_t * sch0 = (const schiene_t *) welt->lookup( pos_next )->gib_weg(gib_wegtyp());
	const route_t * route = cnv->get_route();
	assert(sch0!=NULL);

	// find next blocksegment enroute
	for (int i = route_index+1; i < route->gib_max_n(); i++) {

		const schiene_t * sch1 = (const schiene_t *) welt->lookup( route->position_bei(i))->gib_weg(gib_wegtyp());
		if(sch1==NULL) {
			dbg->error("waggon_t::is_next_block_free()","invalid route");
			return true;
		}
		assert(sch0->gib_blockstrecke().is_bound());
		assert(sch1->gib_blockstrecke().is_bound());

		if(sch0->gib_blockstrecke() != sch1->gib_blockstrecke()) {
			// next blocksegment found!
			sig->set_next_block_pos( sch1->gib_blockstrecke() );
			return sch1->gib_blockstrecke()->ist_frei();
		}
	}
	// no next-next block found enroute!
	return true;
}

void
waggon_t::verlasse_feld()
{
  vehikel_t::verlasse_feld();

  if(ist_blockwechsel(pos_cur, pos_next)) {
    welt->lookup(gib_pos())->verlasse(this);
  }
}

void
waggon_t::betrete_feld()
{
	grund_t * gr = welt->lookup(gib_pos());
	static ding_t *arr[256];
	bool ok=false;
	const uint8 offset=gr->gib_weg(weg_t::strasse)!=0 ? PRI_RAIL_AND_ROAD : PRI_RAIL;

	if(ist_blockwechsel(pos_prev, pos_cur)) {
		gr->betrete(this);
	}

	// Hajo: scan for waggons
	uint8 i, idx=0;
	arr[idx++] = this;
	for(i=offset; i<gr->gib_top(); i++) {
		ding_t *dt = gr->obj_bei(i);
		if(dt != NULL && dt->gib_typ()==ding_t::waggon) {
			arr[idx++] = gr->obj_takeout(i);
		}
	}

	// sort them
	if(idx>1) {
		// Hajo: sort by y-offset
		for(i=0; i<idx; i++) {
			int best = -1;
			int max_y = -999;

			for(int j=0; j<idx; j++) {
				if(arr[j] && arr[j]->gib_yoff() > max_y) {
					max_y = arr[j]->gib_yoff();
					best = j;
				}
			}
			if(best != -1) {
				gr->obj_pri_add(arr[best],offset);
				arr[best] = 0;
			}
			else {
				dbg->error("waggon_t::betrete_feld()","sorting failed!");
			}
		}
	}
	else {
		ok = (gr->obj_pri_add(this, offset) != 0);
	}

	if(ist_erstes) {
		reliefkarte_t::gib_karte()->setze_relief_farbe(gib_pos().gib_2d(), VEHIKEL_KENN);
	}

	const int cargo = gib_fracht_menge();
	gr->gib_weg(weg_t::schiene)->book(cargo, WAY_STAT_GOODS);
	if (ist_erstes) {
		gr->gib_weg(weg_t::schiene)->book(1, WAY_STAT_CONVOIS);
	}
}


waggon_t::waggon_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
    rdwr(file, true);
}


waggon_t::waggon_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cn) :
    vehikel_t(welt, pos, besch, sp)
{
    cnv = cn;
}

waggon_t::~waggon_t()
{
}


void waggon_t::calc_bild()
{
    if(welt->lookup( pos_cur ) &&
       welt->lookup( pos_cur )->ist_im_tunnel() ) {	// tunnel ?
	setze_bild(0, IMG_LEER);
    } else  {
	setze_bild(0, besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()), fracht.is_empty()));
    }
}


fahrplan_t * waggon_t::erzeuge_neuen_fahrplan() const
{
  return new zugfahrplan_t();
}


void
waggon_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	}
	else {
		rdwr(file, true);
	}
}

void
waggon_t::rdwr(loadsave_t *file, bool force)
{
	static const vehikel_besch_t *last_besch;
	assert(force == true);

	if(file->is_saving() && cnv != NULL && !force) {
		file->wr_obj_id(-1);
		last_besch = NULL;
	}
	else {
		vehikel_t::rdwr(file);
		// try to find a matching vehivle
		if(file->is_loading()  &&  besch==NULL) {
			int power = (fracht.count()==0  ||  fracht.at(0)==warenbauer_t::nichts) ? 500 : 0;
			const ware_besch_t *w= (fracht.count()>0) ? fracht.at(0).gib_typ() : warenbauer_t::passagiere;
			DBG_MESSAGE("waggon_t::rdwr()","try to find a fitting vehicle %s.", power>0 ? "engine": w->gib_name() );
			if(!ist_erstes  &&  last_besch!=NULL  &&  last_besch->gib_ware()==w  &&
				(
					(power>0  &&  last_besch->gib_leistung()>0)  ||  (power==0  &&  last_besch->gib_leistung()>=0)
				)
			) {
				// same as previously ...
				besch = last_besch;
			}
			else {
				// we have to search
				last_besch = besch = vehikelbauer_t::vehikel_search(weg_t::schiene, 0xFFFFFFFFul, ist_erstes ? 500 : power, speed_to_kmh(get_speed_limit()), power>0 ? NULL : w, false );
			}
			if(besch) {
DBG_MESSAGE("waggon_t::rdwr()","replaced by %s",besch->gib_name());
				calc_bild();
			}
		}
    }
}


schiff_t::schiff_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cn) :
    vehikel_t(welt, pos, besch, sp)
{
    cnv = cn;
}

schiff_t::schiff_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
	rdwr(file, true);
}


ribi_t::ribi
schiff_t::gib_ribi(const grund_t *gr) const
{
	return gr->gib_weg_ribi(weg_t::wasser);
}


bool
schiff_t::ist_befahrbar(const grund_t *bd) const
{
    return bd->ist_wasser()  ||  bd->gib_weg(weg_t::wasser);
}

/* Since slopes are handled different for ships
 * @author prissi
 */
void
schiff_t::calc_akt_speed(const int h_alt, const int h_neu)
{
	// even track
	current_friction = 1;
	// or a hill?
	if(h_neu != h_alt) {
		// hill up or down => in lock => deccelarte
		current_friction = 128;
	}

	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) {
		current_friction = 2;
	}

	if(ist_erstes) {
		// just to accelerate: The actual speed takes care of all vehicles in the convoi
 	        const int akt_speed = gib_speed();
	        if(get_speed_limit() != -1 && akt_speed > get_speed_limit()) {
		  cnv->setze_akt_speed_soll(get_speed_limit());
		} else {
		  cnv->setze_akt_speed_soll(akt_speed);
		}
	}
}



bool
schiff_t::ist_weg_frei(int &restart_speed)
{
    restart_speed = -1;
    return true;
}


void
schiff_t::calc_bild()
{
    setze_bild(0, besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()), fracht.is_empty()));
}


fahrplan_t * schiff_t::erzeuge_neuen_fahrplan() const
{
  return new schifffahrplan_t();
}


void
schiff_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	}
	else {
		rdwr(file, true);
	}
}

void
schiff_t::rdwr(loadsave_t *file, bool force)
{
    assert(force == true);

    if(file->is_saving() && cnv != NULL && !force) {
 		file->wr_obj_id(-1);
    } else {
		vehikel_t::rdwr(file);
		// try to find a matching vehivle
		if(file->is_loading()  &&  besch==NULL) {
			DBG_MESSAGE("schiff_t::rdwr()","try to find a fitting vehicle for %s.", fracht.count()>0 ? fracht.at(0).gib_name() : "passagiere" );
			besch = vehikelbauer_t::vehikel_search(weg_t::wasser, 0xFFFFFFFFul, 100, 40, fracht.count()>0?fracht.at(0).gib_typ():warenbauer_t::passagiere );
			if(besch) {
				calc_bild();
			}
		}
    }
}


/**** from here on planes ***/


// this routine is called by find_route, to determined if we reached a destination
bool aircraft_t::ist_ziel(const grund_t *gr) const
{
	if(state!=looking_for_parking) {

		// search for the end of the runway
		const weg_t *w=gr->gib_weg(weg_t::luft);
		if(w  &&  w->gib_max_speed()>=250) {

//DBG_MESSAGE("aircraft_t::ist_ziel()","testing at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
			// ok here is a runway
			ribi_t::ribi ribi= w->gib_ribi();
#ifdef USE_DIFFERENT_WIND
//DBG_MESSAGE("aircraft_t::ist_ziel()","ribi=%i target_ribi=%i",ribi,approach_dir);
			if(ribi_t::ist_einfach(ribi)  &&  (ribi&approach_dir)!=0) {
#else
			if(ribi_t::ist_einfach(ribi)  &&  (ribi&ribi_t::nordost)!=0) {
#endif
				// pointing in our direction
				// here we should check for length, but we assume everything is ok
//DBG_MESSAGE("aircraft_t::ist_ziel()","success at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
				return true;
			}
		}
	}
	else {
		// otherwise we just check, if we reached a free stop position of this halt
		if(gr->gib_halt()==target_halt) {
			if(gr->suche_obj(ding_t::aircraft)==NULL) {
//DBG_MESSAGE("is_target()","success at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
				return true;
			}
		}
	}
//DBG_MESSAGE("is_target()","failed at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
	return false;
}



// for flying thingies, everywhere is good ...
// another function only called during route searching
ribi_t::ribi
aircraft_t::gib_ribi(const grund_t *gr) const
{
	weg_t *weg = gr->gib_weg(weg_t::luft);
	if(state==flying) {
		koord3d pos=gr->gib_pos();
		// do we need straight departing vector?
		if(abs(pos.x-search_start.x)+abs(pos.y-search_start.y)<7) {
			weg_t *w=welt->lookup(search_start)->gib_weg(weg_t::luft);
			if(w!=NULL) {
				if(pos.x==search_start.x  ||  pos.y==search_start.y) {
					return ribi_t::rueckwaerts(w->gib_ribi());
				}
				// because we are not on a straight line here ...
				return ribi_t::keine;
			}
			// here we start in mid-air; thus we will ignore everything
			return ribi_t::alle;
		}
		// do we need straight approach vector?
		else if(abs(pos.x-search_end.x)+abs(pos.y-search_end.y)<7) {
			if(pos.x==search_end.x  ||  pos.y==search_end.y) {
				weg_t *w = welt->lookup(search_end)->gib_weg(weg_t::luft);
				if(w!=NULL) {
					return w->gib_ribi();
				}
			}
			// becuase we are not on a straight line here ...
			return ribi_t::keine;
		}
		return ribi_t::alle;
	}
	if(weg) {
		return weg->gib_ribi();
	}
	return ribi_t::keine;
}



// how expensive to go here (for way search)
// author prissi
int
aircraft_t::gib_kosten(const grund_t *gr,const uint32 ) const
{
	// first favor faster ways
	const weg_t *w=gr->gib_weg(weg_t::luft);
	int costs = 0;

	if(state==flying) {
		if(w==NULL) {
			costs += 1;
		}
		else {
			if(w->gib_max_speed()<250) {
				costs += 25;
			}
		}
	}
	else {
		// only, if not flying ...
		assert(w);

		if(w->gib_max_speed()<250) {
			costs += 3;
		}
		else {
			costs += 2;
		}
	}

	return costs;
}



// whether the ground is drivable or not depends on the current state of the airplane
bool
aircraft_t::ist_befahrbar(const grund_t *bd) const
{
	switch (state) {
		case taxiing:
		case looking_for_parking:
//DBG_MESSAGE("ist_befahrbar()","at %i,%i",bd->gib_pos().x,bd->gib_pos().y);
			return bd->gib_weg(weg_t::luft)!=NULL;

		case landing:
		case departing:
		case flying:
		{
			// allow only runways
			const weg_t *w=bd->gib_weg(weg_t::luft);
			if(w  &&  w->gib_max_speed()<250) {
				return false;
			}
			// prissi: here a height check could avoid too height montains
			return true;
		}
	}
	return false;
}




bool
aircraft_t::ist_weg_frei(int & restart_speed)
{
	restart_speed = -1;

	grund_t *gr = welt->lookup( pos_next );
	if(gr==NULL) {
		return false;
	}

	if(gr->obj_count()>200) {
		// too many objects here
		return false;
	}

	if(route_index==suchen) {
		state = looking_for_parking;
	}

	if(state==looking_for_parking) {
		// here we search a free position or else fail ...
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","searching for free stop");
		route_t *rt=cnv->get_route();
		grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));

		// is our target occupied
		if(target  &&  target->gib_halt().is_bound()  &&  target->suche_obj(ding_t::aircraft)) {
			// if we fail, we will wait in a step, much more simulation friendly
			if(cnv->get_state()!=convoi_t::WAITING_FOR_CLEARANCE) {
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","not free => WAIT MODE");
				restart_speed = 0;
				return false;
			}
			// check if there is a free position
			// this is much faster than waysearch (target_halt is class variable, sicne the target searcher needs this too)
			target_halt = target->gib_halt();
			if(!target_halt->find_free_position(weg_t::luft,ding_t::aircraft)) {
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","all occupied => WAIT MODE");
				return false;
			}
			// now it make sense to search a route
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","some free: find route");
			route_t target_rt;
			if(!target_rt.find_route( welt, pos_cur, this, 500, ribi_t::alle, 100 )) {
				// now we will wait in a step, thus time enough to calculate a new route
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","found no route to free one");
				return false;
			}
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","found free stop near %i,%i,%i",target_rt.position_bei(target_rt.gib_max_n()).x,target_rt.position_bei(target_rt.gib_max_n()).y, target_rt.position_bei(target_rt.gib_max_n()).z );
			rt->kopiere( &target_rt );
			rt->insert(pos_cur);
			route_index = 0;
			suchen = touchdown = takeoff = 0x7FFFFFFFul;
		}
		// an now go there (even if it is occupied in the meantime)
		state = taxiing;
		return true;
	}

	if(gr->gib_halt().is_bound()  &&  gr->suche_obj(ding_t::aircraft)) {
		// the next step is a parking position. We do not enter, if occupied!
		restart_speed = 8;
		return false;
	}

	return true;
}



// this must also change the internal modes for the calculation
void
aircraft_t::betrete_feld()
{
	vehikel_t::betrete_feld();

//DBG_MESSAGE("aircraft_t::betrete_feld()","%i,%i,%i   state=%i",gib_pos().x,gib_pos().y,gib_pos().z,state);

	if(route_index==takeoff) {
		state = departing;
		// stop before take off
		cnv->setze_akt_speed_soll(0);
	}
	else if(state==flying  &&  route_index+6>=touchdown) {
		const short landehoehe=cnv->get_route()->position_bei(touchdown).z+16*(touchdown-route_index);
		if(landehoehe<=flughoehe) {
			state = landing;
			target_height = cnv->get_route()->position_bei(touchdown).z;
		}
	}
	else {
		weg_t *w=welt->lookup(gib_pos())->gib_weg(weg_t::luft);
		if(w) {
			const int cargo = gib_fracht_menge();
			w->book(cargo, WAY_STAT_GOODS);
			if (ist_erstes) {
				w->book(1, WAY_STAT_CONVOIS);
			}
		}
	}
}





aircraft_t::aircraft_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
	rdwr(file, true);
}


aircraft_t::aircraft_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cn) :
    vehikel_t(welt, pos, besch, sp)
{
	cnv = cn;
	state = taxiing;
}

aircraft_t::~aircraft_t()
{
	grund_t * gr = welt->lookup(gib_pos());
	if (!gr) gr = welt->lookup(gib_pos().gib_2d())->gib_kartenboden();
	gr->obj_remove(this, gib_besitzer());
}


void aircraft_t::calc_bild()
{
	setze_bild(0, besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()), fracht.is_empty()));
}


fahrplan_t * aircraft_t::erzeuge_neuen_fahrplan() const
{
  return new airfahrplan_t();
}


void
aircraft_t::rdwr(loadsave_t *file)
{
    if(file->is_saving() && cnv != NULL) {
 	file->wr_obj_id(-1);
    } else {
	rdwr(file, true);
    }
}

void
aircraft_t::rdwr(loadsave_t *file, bool force)
{
	// make sure, we are not called by loading via dingliste directly from a tile!
	assert(force == true);

	if(file->is_saving() && cnv != NULL && !force) {
		file->wr_obj_id(-1);
	}
	else {
		vehikel_t::rdwr(file);
		file->rdwr_enum(state, " ");
		file->rdwr_short(flughoehe, " ");
		file->rdwr_short(target_height, "\n");
		file->rdwr_long(suchen," ");
		file->rdwr_long(touchdown," ");
		file->rdwr_long(takeoff,"\n");
	}
}



#ifdef USE_DIFFERENT_WIND
// well lots of code to make sure, we have at least two diffrent directions for the runway search
uint8
aircraft_t::get_approach_ribi( koord3d start, koord3d ziel )
{
	uint8 dir = ribi_typ( (koord)((ziel-start).gib_2d()) );	// reverse
	// make sure, there are at last two directions to choose, or you might en up with not route
	if(ribi_t::ist_einfach(dir)) {
		dir |= (dir<<1);
		if(dir>16) {
			dir += 1;
		}
	}
	return dir&0x0F;
}
#endif



// main routine: searches the new route in up to three steps
// must also take care of stops under traveling and the like
bool
aircraft_t::calc_route(karte_t * welt, koord3d start, koord3d ziel, uint32 max_speed, route_t *route )
{
	DBG_MESSAGE("aircraft_t::calc_route()","search route from %i,%i,%i to %i,%i,%i state=%i",start.x,start.y,start.z,ziel.x,ziel.y,ziel.z);

	const weg_t *w=welt->lookup(start)->gib_weg(weg_t::luft);
	bool start_in_the_air = (w==NULL);

	suchen = takeoff = touchdown = 0x7ffffffful;
	if(!start_in_the_air) {

		// see, if we find a direct route: We are finished
		state = taxiing;
		if(route->calc_route( welt, start, ziel, this, max_speed )) {
			// ok, we can taxi to our location
			return true;
		}
	}

	if(start_in_the_air  ||  (w->gib_max_speed()>=250  &&  ribi_t::ist_einfach(w->gib_ribi())) ) {
		// we start here, if we are in the air or at the end of a runway
		search_start = start;
		start_in_the_air = true;
		route->clear();
		DBG_MESSAGE("aircraft_t::calc_route()","start in air at %i,%i,%i",search_start.x,search_start.y,search_start.z);
	}
	else {
		// not found and we are not on the takeoff tile (where the route search will fail too) => we try to calculate a complete route, starting with the way to the runway

		// second: find start runway end
		state = taxiing;
#ifdef USE_DIFFERENT_WIND
		approach_dir = get_approach_ribi( ziel, start );	// reverse
		DBG_MESSAGE("aircraft_t::calc_route()","search runway start near %i,%i,%i with corner in %x",start.x,start.y,start.z, approach_dir);
#else
		DBG_MESSAGE("aircraft_t::calc_route()","search runway start near %i,%i,%i",start.x,start.y,start.z);
#endif
		if(!route->find_route( welt, start, this, max_speed, ribi_t::alle, 100 )) {
			DBG_MESSAGE("aircraft_t::calc_route()","failed");
			return false;
		}
		DBG_MESSAGE("aircraft_t::calc_route()","route to runway found");
		// save the route
		search_start = route->position_bei( route->gib_max_n() );
		DBG_MESSAGE("aircraft_t::calc_route()","start at ground at %i,%i,%i",search_start.x,search_start.y,search_start.z);
	}

	// second: find target runway end
	state = taxiing;
#ifdef USE_DIFFERENT_WIND
	approach_dir = get_approach_ribi( start, ziel );	// reverse
	DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z,approach_dir);
#else
	DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z);
#endif
	route_t end_route;

	if(!end_route.find_route( welt, ziel, this, max_speed, ribi_t::alle, 100 )) {
		DBG_MESSAGE("aircraft_t::calc_route()","failed");
		route->clear();
		return false;
	}

	// save target route
	search_end = end_route.position_bei( end_route.gib_max_n() );
	DBG_MESSAGE("aircraft_t::calc_route()","ziel now %i,%i,%i",search_end.x,search_end.y,search_end.z);

	// ok, we have a runway, now find a route going there
	route_t fly_route;
	state = flying;
	if(!fly_route.calc_route( welt, search_start, search_end, this, max_speed )) {
		DBG_MESSAGE("aircraft_t::calc_route()","failed");
		route->clear();
		return false;
	}

	if(!start_in_the_air) {
		DBG_MESSAGE("aircraft_t::calc_route()","%i,%i,%i->%i,%i,%i",
			route->position_bei(route->gib_max_n()).x, route->position_bei(route->gib_max_n()).y, route->position_bei(route->gib_max_n()).z,
			fly_route.position_bei(0).x, fly_route.position_bei(0).y, fly_route.position_bei(0).z );
		takeoff = route->gib_max_n();
		route->append(&fly_route);
	}
	else {
		route->kopiere(&fly_route);
	}

	// try to find the maximum height
	target_height = 0;
	state = start_in_the_air ? departing : flying;
	for(int i=0;  i<route->gib_max_n();  i++  ) {
		const grund_t *gr=welt->lookup(route->position_bei(i));
		const short z=gr->gib_pos().z;
		bool hat_luftweg=gr->gib_weg(weg_t::luft)!=NULL;
		if(state==departing  &&  !hat_luftweg) {
			state = flying;
		}
		if(state==flying) {
			if(z>target_height) {
				// get maximum flight level and touchdown point
				target_height = z;
			}
			// find the start of the appraoch
			if(hat_luftweg  &&  i+7>route->gib_max_n()) {
				touchdown = i;
				break;
			}
		}
	}
	target_height += 48;

	// no the route rearch point (+1, since it will check before entering the tile ...
	suchen = route->gib_max_n();

	// now we just append the rest
	for( int i=end_route.gib_max_n();  i>=0;  i--  ) {
		route->append(end_route.position_bei(i));
	}

	// finally, we can start
	state = start_in_the_air ? flying : taxiing;
	flughoehe = gib_pos().z+target_height;

	DBG_MESSAGE("aircraft_t::calc_route()","departing=%i  touchdown=%i   suchen=%i   total=%i  state=%i",takeoff, touchdown, suchen, route->gib_max_n(), state );
	return true;
}



// well, the heihgt is of course most important for an aircraft ...
int aircraft_t::calc_height()
{
	int new_hoff;
	const weg_t *w=welt->lookup(pos_next)->gib_weg(weg_t::luft);

	if(state==departing) {
		current_friction = 16;
		cnv->setze_akt_speed_soll(0x7ffffffful);

		// take off, when a) end of runway or b) last tile of runway or c) fast enough
		if(w==NULL  ||  ribi_t::ist_einfach(w->gib_ribi())  ||  cnv->gib_akt_speed()>kmh_to_speed(besch->gib_geschw())/3 ) {
			state = flying;
			flughoehe = gib_pos().z;
			target_height = flughoehe +48;
		}
	}

	if(state==flying  ||  state==landing) {

		// did we have to change our flight height?
		if(state==landing) {
			// target_height was calculated by betrete_feld()!
//			target_height = pos_next.z;
		}
		else if(target_height-pos_next.z>16*5) {
			// sinken
			target_height -= 32;
		}
		else if(target_height-pos_next.z<32) {
			// steigen
			target_height += 32;
		}
		if(flughoehe<target_height) {
			flughoehe ++;
		}
		else if(flughoehe>target_height) {
			flughoehe --;
		}

		if(flughoehe<pos_cur.z) {
			flughoehe = pos_cur.z;
		}
//DBG_MESSAGE("target_height","%i->%i",flughoehe,target_height);

		new_hoff = pos_cur.z-flughoehe;
		calc_akt_speed( new_hoff, gib_hoff() );
	}
	else {
		if(w) {
			setze_speed_limit(kmh_to_speed(w->gib_max_speed()));
		}
		new_hoff = vehikel_t::calc_height();
	}

	return new_hoff;
}




/* calculates the current friction coefficient based on the curent track
 * falt, slope, curve ...
 * @author prissi, HJ
 */
void
aircraft_t::calc_akt_speed(const int h_alt, const int h_neu)
{
	switch(state) {

		case flying:
			current_friction = (h_alt!=h_neu) ? 16 : 1;
			break;

		case landing:
			// break stronger on runway
			current_friction = (h_alt!=h_neu) ? 16 : 32;
			cnv->setze_akt_speed_soll( (h_alt!=h_neu) ? kmh_to_speed(besch->gib_geschw())/2 : kmh_to_speed(besch->gib_geschw())/3 );
			break;

		case taxiing:
		case looking_for_parking:
			// curve: higher friction
			current_friction = (alte_fahrtrichtung != fahrtrichtung) ? 512 : 128;
			// just to accelerate: The actual speed takes care of all vehicles in the convoi
			const int akt_speed = gib_speed();
			if(get_speed_limit() != -1 && akt_speed > get_speed_limit()) {
				cnv->setze_akt_speed_soll(get_speed_limit());
			} else {
				cnv->setze_akt_speed_soll(akt_speed);
			}
			break;
	}
}

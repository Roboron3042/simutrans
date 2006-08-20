/*
 * curiositylist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "curiositylist_stats_t.h"

#include "../simgraph.h"
#include "../simtypes.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simplan.h"

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"

#include "../utils/cbuffer_t.h"

curiositylist_header_t::curiositylist_header_t(curiositylist_stats_t *s) : stats(s)
{
//SDBG_DEBUG("curiositylist_header_t()","constructor");
	setze_groesse(koord(400, 19));
}


curiositylist_header_t::~curiositylist_header_t()
{
//DBG_DEBUG("curiositylist_header_t()","destructor");
}

void curiositylist_header_t::zeichnen(koord pos) const
{
	display_proportional_clip(pos.x+6, pos.y+6, translator::translate("curlist_legend"), ALIGN_LEFT, WEISS, true);
}

curiositylist_stats_t::curiositylist_stats_t(karte_t * w) : welt(w), attractions(10)
{
	get_unique_attractions();
	setze_groesse(koord(210, attractions.get_count()*14 +14));
}

curiositylist_stats_t::~curiositylist_stats_t()
{
	//DBG_DEBUG("curiositylist_stats_t()","destructor");
}

void curiositylist_stats_t::get_unique_attractions()
{
	attractions.clear();
	const weighted_vector_tpl<gebaeude_t *> &ausflugsziele = welt->gib_ausflugsziele();

	for (unsigned int i=0; i<ausflugsziele.get_count(); ++i)
	{
		gebaeude_t *geb = ausflugsziele.at(i);
		// now check for paranoia, first tile on multitile buildings and real attraction
		if (geb==NULL  ||  geb->gib_tile()->gib_offset()!=koord(0,0)  ||  geb->gib_passagier_level()==0) {
			continue;
		}

		bool append = true;
		for (unsigned int j=0; j<attractions.get_count(); ++j) {
			const char *desc = translator::translate(geb->gib_tile()->gib_besch()->gib_name());
			const char *check_desc = translator::translate(attractions.at(j)->gib_tile()->gib_besch()->gib_name());

			append = STRICMP(desc,check_desc)>=0;

			if (!append) {
DBG_MESSAGE("curiositylist_stats_t::get_unique_attractions()","insert %s at (%i,%i)",geb->gib_tile()->gib_besch()->gib_name(),geb->gib_pos().x, geb->gib_pos().y );
				attractions.insert_at(j,geb);
				break;
			}
		}
		if (append) {
DBG_MESSAGE("curiositylist_stats_t::get_unique_attractions()","append %s at (%i,%i)",geb->gib_tile()->gib_besch()->gib_name(),geb->gib_pos().x, geb->gib_pos().y );
			attractions.append(geb,4);
		}
	}
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::infowin_event(const event_t * ev)
{

if (IS_LEFTRELEASE(ev))
{
	const unsigned int line = (ev->cy) / 14;

	if( line < attractions.get_count() ) {
		gebaeude_t * geb = attractions.at(line);
		if(geb) {
			const koord3d pos = geb->gib_pos();
			welt->setze_ij_off(pos.gib_2d() + koord(-5,-5));
			if (event_get_last_control_shift() != 2) {
				geb->zeige_info();
			}
		}
	}
}
} // end of function curiositylist_stats_t::infowin_event(const event_t * ev)


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::zeichnen(koord offset) const
{
	const struct clip_dimension cd = display_gib_clip_wh();
	const int start = cd.y-LINESPACE;
	const int end = cd.yy;

	const char *formatstr=translator::translate("%s (%i,%i) - (pax %i, post %i)");
	static char name[64], buf[128];
	int yoff = offset.y;

	for (unsigned int i=0; i<attractions.get_count()  &&  yoff<end; i++) {
		const gebaeude_t *geb = attractions.get(i);

		int xoff = offset.x;

		// skip invisible lines
		if(yoff<start) {
			yoff += LINESPACE+3;
			continue;
		}

		// is connected? => decide on indicatorcolor
		int indicatorfarbe;
		bool post=false;
		bool pax=false;
		bool all_crowded=true;
		bool some_crowded=false;
		const minivec_tpl<halthandle_t> &halt_list = welt->access(geb->gib_pos().gib_2d())->get_haltlist();
		for(  unsigned h=0;  (post&pax)==0  &&  h<halt_list.get_count();  h++ ) {
			if(halt_list.get(h)->get_pax_enabled()) {
				pax = true;
				if(halt_list.get(h)->get_pax_unhappy()>40) {
					some_crowded |= true;
				}
				else {
					all_crowded = false;
				}
			}
			if(halt_list.get(h)->get_post_enabled()) {
				post = true;
				if(halt_list.get(h)->get_pax_unhappy()>40) {
					some_crowded |= true;
				}
				else {
					all_crowded = false;
				}
			}
		}
		// now decide on color
		if(some_crowded) {
			indicatorfarbe = all_crowded ? ROT : ORANGE;
		}
		else if(pax) {
			indicatorfarbe = post ? 190 : GREEN;
		}
		else {
			indicatorfarbe = post ? BLAU : GELB;
		}
		display_ddd_box_clip(xoff+7, yoff+6, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(xoff+8, yoff+7, 6, 6, indicatorfarbe, true);

		// trim the name
		const char *desc = translator::translate(geb->gib_tile()->gib_besch()->gib_name());
		int chr=0, dest=0;
		while(desc[chr]<=32) {
			chr ++;
		}
		while(dest<64  &&  desc[chr]>=32) {
			name[dest++] = desc[chr++];
		}
		name[dest++] = 0;

		sprintf( buf, formatstr, name, geb->gib_pos().x, geb->gib_pos().y, geb->gib_passagier_level(), geb->gib_post_level() );
		display_proportional_clip(xoff+26,yoff+6,buf,ALIGN_LEFT,SCHWARZ,true);

		if (geb->gib_tile()->gib_besch()->gib_bauzeit() != 0)
		    display_color_img(skinverwaltung_t::intown->gib_bild_nr(0), xoff+16, yoff+6, 0, false, false);

	    yoff +=LINESPACE+3;
	}
}

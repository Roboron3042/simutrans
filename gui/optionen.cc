/*
 * optionen.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* optionen.cc
 *
 * Dialog fuer Spieloptionen
 * Niels Roest, Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simimg.h"
#include "../simwin.h"
#include "../simplay.h"
#include "../simgraph.h"
#include "../simdisplay.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"

#include "optionen.h"
#include "colors.h"
#include "sprachen.h"
#include "spieler.h"
#include "welt.h"
#include "kennfarbe.h"
#include "sound_frame.h"
#include "loadsave_frame.h"
#ifdef spaeter
#include "autosave_gui.h"
#endif

optionen_gui_t::optionen_gui_t(karte_t *welt)
 : infowin_t(welt), buttons(10)
{
    // init buttons
    button_t button_def;

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,65) );
    button_def.text = translator::translate("Sprache");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,81) );
    button_def.text = translator::translate("Farbe");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,97) );
    button_def.text = translator::translate("Helligk.");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,113) );
    button_def.text = translator::translate("Sound");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,129) );
    button_def.text = translator::translate("Spieler(mz)");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,65) );
    button_def.text = translator::translate("Laden");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,81) );
    button_def.text = translator::translate("Speichern");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,97) );
    button_def.text = translator::translate("Neue Karte");
    buttons.append(button_def);

    // 01-Nov-2001      Markus Weber    Added
    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,129) );
    button_def.text = translator::translate("Beenden");
    buttons.append(button_def);
#ifdef spaeter
    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,113) );
    button_def.text = translator::translate("Autosave");
    buttons.append(button_def);
#endif
}

const char *
optionen_gui_t::gib_name() const
{
    return "Einstellungen";
}

/**
 * gibt den Besitzer zur�ck
 *
 * @author Hj. Malthaner
 */
spieler_t* optionen_gui_t::gib_besitzer() const
{
  return welt->gib_spieler(0);
}

/**
 * Jedes Objekt braucht ein Bild.
 *
 * @author Hj. Malthaner
 * @return Die Nummer des aktuellen Bildes f�r das Objekt.
 */
int optionen_gui_t::gib_bild() const
{
  return IMG_LEER;
}


void optionen_gui_t::info(cbuffer_t & buf) const
{
  buf.append(translator::translate("Einstellungen aendern"));
}


koord optionen_gui_t::gib_fenstergroesse() const
{
    return koord(213, 160);
}

void optionen_gui_t::infowin_event(const event_t *ev)
{
    infowin_t::infowin_event(ev);

    if(IS_LEFTCLICK(ev)) {
	if(buttons.at(0).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(1).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(2).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(3).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(4).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(5).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(6).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(7).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(8).getroffen(ev->cx, ev->cy)) {    //01-Nov-2001   Markus Weber    Added
	}
    }



    if(IS_LEFTRELEASE(ev)) {
	if(buttons.at(0).getroffen(ev->cx, ev->cy)) {
	    buttons.at(0).pressed = false;
	    create_win(new sprachengui_t(welt), w_info, magic_sprachengui_t);

	} else if(buttons.at(1).getroffen(ev->cx, ev->cy)) {
	    buttons.at(1).pressed = false;
	    create_win(new farbengui_t(welt), w_info, magic_farbengui_t);

	} else if(buttons.at(2).getroffen(ev->cx, ev->cy)) {
	    buttons.at(2).pressed = false;
	    create_win(new color_gui_t(welt), w_info, magic_color_gui_t);

	} else if(buttons.at(3).getroffen(ev->cx, ev->cy)) {
	    buttons.at(3).pressed = false;
	    create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);

	} else if(buttons.at(4).getroffen(ev->cx, ev->cy)) {
	    buttons.at(4).pressed = false;
	    create_win(new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);

	}  else if(buttons.at(5).getroffen(ev->cx, ev->cy)) {
	    buttons.at(5).pressed = false;

	    destroy_win(this);

	    create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);

	}  else if(buttons.at(6).getroffen(ev->cx, ev->cy)) {
	    buttons.at(6).pressed = false;

	    destroy_win(this);

	    create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);


            // New world-Button
            //-----------------------
        }  else if(buttons.at(7).getroffen(ev->cx, ev->cy)) {
	    buttons.at(7).pressed = false;
	    destroy_all_win();
	    //welt->beenden();                  //02-Nov-2001   Markus Weber    modified
            welt->beenden(false);


            // Quit-Button                      // 01-Nov-2001      Markus Weber    Added
            //-----------------------
	}  else if(buttons.at(8).getroffen(ev->cx, ev->cy)) {
            // quit the game
	    welt->beenden(true);

#ifdef spaeter
	}  else if(buttons.at(9).getroffen(ev->cx, ev->cy)) {
	    buttons.at(9).pressed = false;
	    create_win(new autosave_gui_t(welt), w_info, magic_autosave_t);
#endif
	}
    }
}


vector_tpl<button_t>*
optionen_gui_t::gib_fensterbuttons()
{
  // variable teile aktualsieren

  buttons.at(0).text = translator::translate("Sprache");
  buttons.at(1).text = translator::translate("Farbe");
  buttons.at(2).text = translator::translate("Helligk.");
  buttons.at(3).text = translator::translate("Sound");
  buttons.at(4).text = translator::translate("Spieler(mz)");
  buttons.at(5).text = translator::translate("Laden");
  buttons.at(6).text = translator::translate("Speichern");
  buttons.at(7).text = translator::translate("Neue Karte");
  buttons.at(8).text = translator::translate("Beenden");     // 01-Nov-2001  Markus Weber    Added
#ifdef spaeter
  buttons.at(9).text = translator::translate("Autosave");
#endif
  return &buttons;
}


void optionen_gui_t::zeichnen(koord pos, koord gr)
{
  infowin_t::zeichnen(pos, gr);

  display_divider(pos.x+10, pos.y+55, 191);
}

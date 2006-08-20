/*
 * button.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_button_h
#define gui_button_h

#include "../../ifc/gui_action_creator.h"
#include "../../simcolor.h"


// forward decls
class action_listener_t;
template <class T> class slist_tpl;


#define SQUARE_BUTTON 0
#define ARROW_LEFT 1
#define ARROW_RIGHT 2
#define ARROW_UP 3
#define ARROW_DOWN 4
#define SCROLL_BAR 5

/**
 * Klasse f�r Buttons in Fenstern
 *
 * @author Hj. Malthaner, Niels Roest
 * @date December 2000
 */
class button_t : public gui_komponente_action_creator_t
{
public:
	/* the button with the postfix state do not automatically change their state like the normal button do
	 * the _state buttons must be changed by the caller!
	 *
	 * square: button with text on the right side next to it
	 * box:  button with is used for many selection purposes; can have colored background
	 * roundbox: button for "load" cancel and such options
	 * arrow-buttons: buttons with arrows, cannot have text
	 * repeat arrows: calls the caller until the mouse is released
	 * scrollbar: well you guess it. Not used by gui_frame_t things ...
	 */
	enum type { square=1, box, roundbox, arrowleft, arrowright, arrowup, arrowdown, scrollbar, repeatarrowleft, repeatarrowright,
					   square_state=129, box_state, roundbox_state, arrowleft_state, arrowright_state, arrowup_state, arrowdown_state, scrollbar_state, repeatarrowleft_state, repeatarrowright_state };

private:
	/**
	 * Tooltip ofthis button
	 * @author Hj. Malthaner
	 */
	const char * tooltip;

	enum type type;

	/**
	 * if buttons is disabled show only grey label
	 * @author hsiegeln
	 */
	uint8 b_enabled:1;

public:
	PLAYER_COLOR_VAL background; //@author hsiegeln
	PLAYER_COLOR_VAL foreground;

	/**
	 * Der im Button angezeigte Text
	 * direct acces provided to avoid translations
	 * @author Hj. Malthaner
	 */
	const char * text;

	bool pressed:1;

	button_t(const button_t & other);

	button_t();

	void init(enum type typ, const char *text, koord pos, koord size = koord::invalid);

	void setze_typ(enum type typ);

	const char * gib_text() const {return text;};

	/**
	 * Setzt den im Button angezeigten Text
	 * @author Hj. Malthaner
	 */
	void setze_text(const char * text);

	/**
	 * Sets the tooltip of this button
	 * @author Hj. Malthaner
	 */
	void set_tooltip(const char * tooltip);

	/**
	 * @return true wenn x,y innerhalb der Buttonflaeche liegt, d.h. der
	 * Button getroffen wurde, false wenn x, y ausserhalb liegt
	 * @author Hj. Malthaner
	 */
	bool getroffen(int x,int y);

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	void infowin_event(const event_t *);

	/**
	 * Zeichnet den Button.
	 * @author Niels Roest
	 */
	void zeichnen(koord offset, PLAYER_COLOR_VAL button_farbe) const;

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset) const;

	void operator= (const button_t & other);

	void enable() { b_enabled = true; };

	void disable() { b_enabled = false; };

	bool enabled() { return b_enabled; };
};

#endif

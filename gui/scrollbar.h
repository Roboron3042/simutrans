#ifndef gui_scrollbar_h
#define gui_scrollbar_h

#include "../tpl/slist_tpl.h"
#include "../dataobj/koord.h"
#include "../simevent.h"
#include "button.h"
#include "../ifc/gui_komponente.h"

class scrollbar_listener_t;

/**
 * Scrollbar class
 * scrollbar can be horizontal or vertical
 *
 * @author Niels Roest, additions by Hj. Malthaner
 * @version $Revision: 1.9 $
 */
class scrollbar_t : public gui_komponente_t
{
public:

    enum type {
	vertical, horizontal
    };

private:
    enum type type;


    /**
     * scrollbahn zeichnen? default ist false
     * @author Hansj�rg Malthaner
     */
    bool opaque;


    // the following three values are from host (e.g. list), NOT actual size.
    int knob_offset; // offset from top-left
    int knob_size; // size of scrollbar knob
    int knob_area; // size of area where knob moves in

    /**
     * number of pixels to scroll with arrow button press. default: 11 pixels
     */
    int knob_scroll_amount;

    button_t *buttons[4];
    button_t button_def[4];

    void reposition_buttons();
    void button_press(int number); // arrow button
    void space_press(int updown); // space in slidebar hit
    int slider_drag(int amount); // drags slider. returns dragged amount.

    /**
     * real position of knob in pixels, counting from zero
     */
    int real_knob_position() {
	if (type == vertical) {
	    return button_def[2].pos.y-12;
	} else /* horizontal */ {
	    return button_def[2].pos.x-12;
	}
    }

    slist_tpl <scrollbar_listener_t *> callback_list;

    void call_callback();

public:

    // type is either scrollbar_t::horizontal or scrollbar_t::vertical
    scrollbar_t(enum type type);


    /**
     * Vorzugsweise sollte diese Methode zum Setzen der Gr��e benutzt werden,
     * obwohl groesse public ist.
     * @author Hj. Malthaner
     */
    void setze_groesse(koord groesse);


    /**
     * setzt die Transparenz
     * @author Hj. Malthaner
     */
    void setze_opaque(bool janein) {opaque = janein;};


    void setze_scroll_amount(int sa) {
	knob_scroll_amount = sa;
    };

    /**
     * size is visible size, area is total size in pixels of _parent_.
     */
    void setze_knob(int knob_size, int knob_area);

    // add/remove callback functions
    void add_listener(scrollbar_listener_t *c) {
	callback_list.insert(c);
    };


    int gib_knob_offset() const {return knob_offset;};

    void setze_knob_offset(int v) {knob_offset = v; reposition_buttons();};



    void remove_listener(scrollbar_listener_t *c) {
	callback_list.remove(c);
    };

    void infowin_event(const event_t *ev);


    /**
     * Draw scrollbar. Offset is top left of window.
     * @author Niels Roest
     */
    void zeichnen(koord pos, int button_farbe) const;

    void zeichnen(koord pos) const;
};

#endif

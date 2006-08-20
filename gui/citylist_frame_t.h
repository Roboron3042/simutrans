#ifndef citylist_frame_t_h
#define citylist_frame_t_h

#include "../gui/gui_frame.h"
#include "../gui/citylist_stats_t.h"
#include "../dataobj/translator.h"
#include "ifc/action_listener.h"
#include "button.h"
#include "gui_label.h"
#include "gui_scrollpane.h"

class gui_scrollpane_t;
class citylist_stats_t;
class karte_t;

/**
 * City list window
 * @author Hj. Malthaner
 */
class citylist_frame_t : public gui_frame_t, private action_listener_t
{

 private:
    static const char *sort_text[citylist::SORT_MODES];

    gui_scrollpane_t *scrolly;
    citylist_stats_t *stats;

    gui_label_t sort_label;
    button_t	sortedby;
    button_t	sorteddir;

    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static citylist::sort_mode_t sortby;
    static bool sortreverse;

 public:

    citylist_frame_t(karte_t * welt);
    ~citylist_frame_t();

   /**
     * Komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr);

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    virtual void resize(const koord delta);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen f�r die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "citylist_filter.txt"; }

    static citylist::sort_mode_t gib_sortierung() { return sortby; }
    static void setze_sortierung(const citylist::sort_mode_t& sm) { sortby = sm; }

    static bool gib_reverse() { return sortreverse; }
    static void setze_reverse(const bool& reverse) { sortreverse = reverse; }
};

#endif // citylist_frame_t_h

/*
 * monorail.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_monorail_h
#define boden_wege_monorail_h


#include "schiene.h"


/**
 * Klasse f�r Schienen in Simutrans.
 * Auf den Schienen koennen Z�ge fahren.
 * Jede Schiene geh�rt zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class monorail_t : public schiene_t
{
public:
    /**
     * Basic constructor.
     * @author prissi
     */
    monorail_t(karte_t *welt) : schiene_t(welt) {}

    /**
     * File loading constructor.
     * @author prissi
     */
    monorail_t(karte_t *welt, loadsave_t *file) : schiene_t(welt,file) {}


    /**
     * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
     * @author prissi
     */
    virtual ~monorail_t() {}

    virtual const char *gib_typ_name() const {return "Monorail";}
    virtual typ gib_typ() const {return schiene_monorail;}

    /**
     * @return Infotext zur Schiene
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;
};

#endif

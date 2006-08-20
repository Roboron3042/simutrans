/*
 * leitung.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_leitung_t
#define dings_leitung_t


#include "../ifc/sync_steppable.h"
#include "../dataobj/koord3d.h"
#include "../simdings.h"

class powernet_t;
class spieler_t;
class fabrik_t;

class leitung_t : public ding_t
{
protected:
  /**
   * We are part of this network
   * @author Hj. Malthaner
   */
  powernet_t * net;

  /**
   * Connect this piece of powerline to its neighbours
   * -> this can merge power networks
   * @author Hj. Malthaner
   */
  void verbinde();

  void replace(koord base_pos, powernet_t *alt, powernet_t *neu);

public:
	powernet_t * get_net() const {return net;};
	void set_net(powernet_t *p) {net=p;};

	ribi_t::ribi gib_ribi();

	static fabrik_t * suche_fab_4(koord pos);

	leitung_t(karte_t *welt, loadsave_t *file);
	leitung_t(karte_t *welt, koord3d pos, spieler_t *sp);
	virtual ~leitung_t();

	enum ding_t::typ gib_typ() const {return leitung;};

	const char *gib_name() const {return "Leitung";};

	/**
	* @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const;

	/**
	* Dient zur Neuberechnung des Bildes
	* @author Hj. Malthaner
	*/
	virtual void calc_bild();

	/**
	* Recalculates the images of all neighbouring
	* powerlines and the powerline itself
	*
	* @author Hj. Malthaner
	*/
	void calc_neighbourhood();

	/**
	* Wird nach dem Laden der Welt aufgerufen - �blicherweise benutzt
	* um das Aussehen des Dings an Boden und Umgebung anzupassen
	*
	* @author Hj. Malthaner
	*/
	virtual void laden_abschliessen();

	/**
	* Speichert den Zustand des Objekts.
	*
	* @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	* soll.
	* @author Hj. Malthaner
	*/
	virtual void rdwr(loadsave_t *file);
};



class pumpe_t : public leitung_t, public sync_steppable
{
private:
    bool power_there;
    fabrik_t *fab;

protected:


public:
    pumpe_t(karte_t *welt, loadsave_t *file);
    pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp);
    ~pumpe_t();

    void setze_fabrik(fabrik_t *fab) {this->fab = fab;};

    enum ding_t::typ gib_typ() const {return pumpe;};

    void sync_prepare();
    bool sync_step(long delta_t);

    const char *name() const {return "Pumpe";};
};



class senke_t : public leitung_t, public sync_steppable
{
private:
    int einkommen;
    int max_einkommen;
    fabrik_t *fab;

protected:

public:

    senke_t(karte_t *welt, loadsave_t *file);
    senke_t(karte_t *welt, koord3d pos, spieler_t *sp);
    ~senke_t();
    enum ding_t::typ gib_typ() const {return senke;};

    /**
     * book money and redraw image
     * @author prissi
     */
    virtual bool step(long /*delta_t*/);

    void sync_prepare();
    bool sync_step(long delta_t);

    const char *name() const {return "Senke";};

    void info(cbuffer_t & buf) const;
};

#endif

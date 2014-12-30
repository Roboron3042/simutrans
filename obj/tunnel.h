#ifndef obj_tunnel_h
#define obj_tunnel_h

#include "../simobj.h"
#include "../display/simimg.h"

class tunnel_besch_t;

class tunnel_t : public obj_no_info_t
{
private:
	const tunnel_besch_t *besch;
	image_id bild;
	image_id after_bild;
	uint8 broad_type; // Is this a broad tunnel mouth?

public:
	tunnel_t(loadsave_t *file);
	tunnel_t(koord3d pos, spieler_t *sp, const tunnel_besch_t *besch);

	const char *get_name() const {return "Tunnelmuendung";}
	typ get_typ() const { return tunnel; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const;

	void calc_bild();

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) { if(  !calc_only_season_change  ) { calc_bild(); } return true; };  // depends on snowline only

	void set_bild( image_id b );
	void set_after_bild( image_id b );
	image_id get_bild() const {return bild;}
	image_id get_after_bild() const { return after_bild; }

	const tunnel_besch_t *get_besch() const { return besch; }

	void set_besch( const tunnel_besch_t *_besch ) { besch = _besch; }

	void rdwr(loadsave_t *file);

	void laden_abschliessen();

	void entferne(spieler_t *sp);


	uint8 get_broad_type() const { return broad_type; };
	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);
};

#endif

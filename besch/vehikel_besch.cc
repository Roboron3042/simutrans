#include "vehikel_besch.h"
#include "xref_besch.h"
#include "../network/checksum.h"

void vehikel_besch_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_transport_related_t::calc_checksum(chk);
	chk->input(capacity);
	chk->input(loading_time);
	chk->input(weight);
	chk->input(power);
	chk->input(running_cost);
	chk->input(fixed_cost);
	chk->input(gear);
	chk->input(len);
	chk->input(leader_count);
	chk->input(trailer_count);
	chk->input(engine_type);
	// freight
	const xref_besch_t *xref = get_child<xref_besch_t>(2);
	chk->input(xref ? xref->get_name() : "NULL");
	// vehicle constraints
	for(uint16 i=0; i<leader_count+trailer_count; i++) {
		const xref_besch_t *xref = get_child<xref_besch_t>(6+i);
		chk->input(xref ? xref->get_name() : "NULL");
	}
}

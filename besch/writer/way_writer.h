#ifndef __WAY_WRITER_H
#define __WAY_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"


class way_writer_t : public obj_writer_t {
    static way_writer_t the_instance;

    way_writer_t() { register_writer(true); }
protected:
    virtual cstring_t get_node_name(FILE *fp) const { return name_from_next_node(fp); }
public:

    /**
     * Write a waytype description node
     * @author Hj. Malthaner
     */
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_way; }

    virtual const char *get_type_name() const { return "way"; }
};

#endif

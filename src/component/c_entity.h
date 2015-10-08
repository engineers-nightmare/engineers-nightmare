#pragma once

static unsigned entities_id_ref = 1;

struct c_entity {
    unsigned id;

    bool operator==(c_entity const &other) const {
        return this->id == other.id;
    }

    bool operator<(c_entity const &other) const {
        return this->id < other.id;
    }

    static c_entity spawn() {
        c_entity e = { entities_id_ref++ };
        return e;
    }
};

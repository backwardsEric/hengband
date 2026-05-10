#include "system/floor/town-records.h"
#include "system/floor/town-list.h"
#include "util/enum-converter.h"

TownRecords TownRecords::instance{};

TownRecords &TownRecords::get_instance()
{
    return instance;
}

bool TownRecords::has_visited(TownId id) const
{
    if (enum2i(id) == SECRET_TOWN) {
        return false;
    }

    return this->visited_ids.has(id);
}

void TownRecords::set_visited(TownId id)
{
    if (enum2i(id) == SECRET_TOWN) {
        return;
    }

    this->visited_ids.set(id);
}

void TownRecords::initialize()
{
    this->visited_ids.clear();
    this->visited_ids.set(TownId::OUTPOST);
}

void TownRecords::set_ids(const EnumClassFlagGroup<TownId> &loaded_data)
{
    this->visited_ids = loaded_data;
}

EnumClassFlagGroup<TownId> TownRecords::get_ids() const
{
    return this->visited_ids;
}

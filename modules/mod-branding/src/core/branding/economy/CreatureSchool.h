#ifndef MOD_BRANDING_CORE_ECONOMY_CREATURESCHOOL_H
#define MOD_BRANDING_CORE_ECONOMY_CREATURESCHOOL_H

#include "branding/common/Brand.h"
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace Branding
{
    // Authored creature-entry -> brand school map (§9/§16 faucet). The adapter loads
    // `branding_creature_school` (creature_entry -> brand) into this registry once at startup/reload;
    // on a creature kill it asks Resolve() which school's Fragment the kill may drop. Pure value type,
    // no AzerothCore deps.
    //
    // Authored, not derived: a live Creature* has no reliable "damage school", and the exotic schools
    // (§7.10) have no creature archetype at all -- so the mapping is data. This is also why the same
    // table answers "fire elemental -> Fire" and "void-themed boss -> Void": both are just rows.
    class CreatureSchoolTable
    {
    public:
        // Insert or overwrite the school a creature entry drops (last write wins).
        void Set(uint32_t creatureEntry, BrandId school) { _schools[creatureEntry] = school; }

        // True when an authored row exists for this creature entry.
        bool Has(uint32_t creatureEntry) const { return _schools.find(creatureEntry) != _schools.end(); }

        // Resolve the school for a creature entry. Returns true and writes `out` when a row exists;
        // returns false (leaving `out` untouched) when the creature is unmapped -- the caller drops
        // nothing in that case.
        bool Resolve(uint32_t creatureEntry, BrandId& out) const
        {
            auto it = _schools.find(creatureEntry);
            if (it == _schools.end())
                return false;
            out = it->second;
            return true;
        }

        // Drop all rows (reload safety).
        void Clear() { _schools.clear(); }

        // Number of distinct mapped creature entries.
        std::size_t Size() const { return _schools.size(); }

    private:
        std::unordered_map<uint32_t, BrandId> _schools;
    };
}

#endif // MOD_BRANDING_CORE_ECONOMY_CREATURESCHOOL_H

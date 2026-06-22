#include "EconomyMgr.h"
#include "RewardDelivery.h"
#include "branding/economy/Economy.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"

namespace Branding
{
    EconomyMgr* EconomyMgr::instance()
    {
        static EconomyMgr mgr;
        return &mgr;
    }

    void EconomyMgr::LoadConfig()
    {
        _config.Load();
    }

    void EconomyMgr::LoadRecipes()
    {
        _recipes.Clear();
        if (!_config.Enabled())
            return;

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `materials`, `fragments`, `output_item`, `char_xp`, `school` FROM `branding_recipe`");
        if (!result)
        {
            LOG_INFO("server.loading", ">> Loaded 0 branding recipes (table empty).");
            return;
        }

        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();
            uint32 const id = fields[0].Get<uint32>();
            Recipe recipe;
            recipe.materials = fields[1].Get<uint32>();
            recipe.fragments = fields[2].Get<uint32>();
            recipe.outputItemId = fields[3].Get<uint32>();
            recipe.charXp = fields[4].Get<uint32>();
            // school: a BrandId (per-school Fragment) or >= COUNT (generic Fragment). Clamp anything
            // out of range to COUNT so a bad row degrades to the generic Fragment, never crashes.
            uint32 const school = fields[5].Get<uint32>();
            recipe.school = school < static_cast<uint32>(BrandId::COUNT)
                ? static_cast<BrandId>(school) : BrandId::COUNT;

            if (_recipes.Add(id, recipe))
                ++count;
            else
                LOG_WARN("server.loading", "branding_recipe id {} skipped: output_item is 0.", id);
        } while (result->NextRow());

        LOG_INFO("server.loading", ">> Loaded {} branding recipes.", count);
    }

    void EconomyMgr::LoadCreatureSchools()
    {
        // §9/§16 faucet: authored creature_entry -> brand school map. Repopulated from scratch so
        // `.reload config` picks up edits. Empty is fine -- the faucet simply drops nothing.
        _creatureSchools.Clear();
        if (!_config.Enabled())
            return;

        QueryResult result = WorldDatabase.Query(
            "SELECT `creature_entry`, `brand` FROM `branding_creature_school`");
        if (!result)
        {
            LOG_INFO("server.loading", ">> Loaded 0 branding creature-school rows (table empty).");
            return;
        }

        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();
            uint32 const entry = fields[0].Get<uint32>();
            uint32 const brand = fields[1].Get<uint32>();
            if (brand >= static_cast<uint32>(BrandId::COUNT))
            {
                LOG_WARN("server.loading", "branding_creature_school entry {} skipped: brand {} out of range.",
                    entry, brand);
                continue;
            }
            _creatureSchools.Set(entry, static_cast<BrandId>(brand));
            ++count;
        } while (result->NextRow());

        LOG_INFO("server.loading", ">> Loaded {} branding creature-school mapping(s).", count);
    }

    CraftReport EconomyMgr::Craft(Player* player, uint32_t recipeId)
    {
        CraftReport report;
        if (!_config.Enabled() || !player)
        {
            report.outcome = CraftOutcome::Disabled;
            return report;
        }

        Recipe const* recipe = _recipes.Find(recipeId);
        if (!recipe)
        {
            report.outcome = CraftOutcome::UnknownRecipe;
            return report;
        }

        report.materialsNeeded = recipe->materials;
        report.fragmentsNeeded = recipe->fragments;
        report.outputItemId = recipe->outputItemId;
        report.charXp = recipe->charXp;

        // §16: a schooled recipe draws from its school's Fragment (the "Fire-Branded Fragment" loop)
        // when per-school Fragments are enabled; otherwise it falls back to the generic Fragment.
        uint32_t const fragmentItem =
            (_config.SchoolFragmentsEnabled() && recipe->school < BrandId::COUNT)
                ? _config.SchoolFragmentItem(recipe->school)
                : _config.FragmentItem();

        Resources available;
        available.materials = player->GetItemCount(_config.MaterialItem(), false);
        available.fragments = player->GetItemCount(fragmentItem, false);
        report.materialsHave = available.materials;
        report.fragmentsHave = available.fragments;

        CraftResult const craft = ResolveCraft(*recipe, available);
        if (!craft.crafted)
        {
            report.outcome = CraftOutcome::InsufficientResources;
            return report;   // nothing consumed
        }

        // Consume exact inputs first, then deliver the output. Removing before delivery keeps the
        // sink ahead of the faucet (no double-spend on a delivery edge case).
        if (craft.consumed.materials > 0)
            player->DestroyItemCount(_config.MaterialItem(), craft.consumed.materials, true);
        if (craft.consumed.fragments > 0)
            player->DestroyItemCount(fragmentItem, craft.consumed.fragments, true);

        DeliveryResult const delivered = DeliverItem(player, craft.outputItemId, 1,
            "Branding Crafting", "Your crafted item.");
        if (delivered == DeliveryResult::None)
        {
            report.outcome = CraftOutcome::DeliveryFailed;
            return report;
        }

        if (craft.charXp > 0)
            player->GiveXP(craft.charXp, nullptr);

        report.outcome = CraftOutcome::Crafted;
        return report;
    }

    bool EconomyMgr::TryDropFragment(Player* killer, uint32_t creatureEntry)
    {
        // Gated on the economy + per-school Fragments being on and a configured drop chance; the
        // faucet is inert otherwise (default off). Materials/generic Fragments are not dropped here --
        // this faucet exists specifically to source the per-school Fragments (§9/§16).
        if (!killer || !_config.Enabled() || !_config.SchoolFragmentsEnabled())
            return false;

        float const chance = _config.FragmentDropChance();
        if (chance <= 0.0f)
            return false;

        BrandId school = BrandId::COUNT;
        if (!_creatureSchools.Resolve(creatureEntry, school))
            return false;   // creature not mapped -> no drop

        if (!roll_chance_f(chance))
            return false;

        DeliveryResult const delivered = DeliverItem(killer, _config.SchoolFragmentItem(school), 1,
            "Branding", "A Branded Fragment, still warm from the kill.");
        return delivered != DeliveryResult::None;
    }
}

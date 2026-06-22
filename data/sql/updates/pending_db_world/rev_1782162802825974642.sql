-- mod-branding §9/§16 faucet: authored creature -> brand school map. A creature mapped here may,
-- on death, drop one of its school's per-school Fragments (item SchoolFragmentBaseItemId + brand)
-- when Branding.Economy.SchoolFragments.Enable is on and Branding.Economy.FragmentDropChance > 0.
-- The mapping is data, not derived: a live creature has no reliable damage school, and the exotic
-- schools (§7.10) have no creature archetype -- so "fire elemental -> Fire" and "void boss -> Void"
-- are both just rows. `brand` is the BrandId index (0..14) per src/core/branding/common/Brand.h.
CREATE TABLE IF NOT EXISTS `branding_creature_school` (
    `creature_entry` INT UNSIGNED NOT NULL COMMENT 'creature_template.entry whose kill may drop a Fragment',
    `brand` TINYINT UNSIGNED NOT NULL COMMENT 'BrandId (0..14); the school of the Fragment dropped',
    PRIMARY KEY (`creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='mod-branding creature -> brand school faucet map (§9/§16)';

-- Example mapping (retarget to your realm's creatures). Ragnaros -> Fire (brand 0) shows the
-- classic case; copy the pattern for more, including exotic schools on themed bosses, e.g.
-- (29311, 10) for Herald Volazj -> Void. Ships as one illustrative row; authoring more is the
-- intended faucet-tuning workflow.
DELETE FROM `branding_creature_school` WHERE `creature_entry` IN (11502);
INSERT INTO `branding_creature_school` (`creature_entry`, `brand`) VALUES
    (11502, 0);

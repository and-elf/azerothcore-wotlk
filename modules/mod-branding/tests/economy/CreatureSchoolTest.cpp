#include "branding/economy/CreatureSchool.h"
#include <gtest/gtest.h>

using namespace Branding;

// §9/§16 faucet: the authored creature-entry -> school map that decides which Fragment a kill drops.

TEST(CreatureSchoolTable, StartsEmptyAndResolveMisses)
{
    CreatureSchoolTable table;
    EXPECT_EQ(table.Size(), 0u);
    EXPECT_FALSE(table.Has(11502));
    BrandId out = BrandId::COUNT;
    EXPECT_FALSE(table.Resolve(11502, out));
    EXPECT_EQ(out, BrandId::COUNT);   // untouched on a miss
}

TEST(CreatureSchoolTable, SetThenResolveReturnsSchool)
{
    CreatureSchoolTable table;
    table.Set(11502, BrandId::Fire);   // e.g. a fire boss -> Fire Fragment
    EXPECT_TRUE(table.Has(11502));
    EXPECT_EQ(table.Size(), 1u);

    BrandId out = BrandId::COUNT;
    ASSERT_TRUE(table.Resolve(11502, out));
    EXPECT_EQ(out, BrandId::Fire);
}

TEST(CreatureSchoolTable, MapsExoticSchoolsToo)
{
    // The same table carries exotic schools (no creature archetype): an authored void boss -> Void.
    CreatureSchoolTable table;
    table.Set(29311, BrandId::Void);
    BrandId out = BrandId::COUNT;
    ASSERT_TRUE(table.Resolve(29311, out));
    EXPECT_EQ(out, BrandId::Void);
}

TEST(CreatureSchoolTable, SetOverwritesAndClearEmpties)
{
    CreatureSchoolTable table;
    table.Set(100, BrandId::Frost);
    table.Set(100, BrandId::Shadow);   // last write wins
    EXPECT_EQ(table.Size(), 1u);
    BrandId out = BrandId::COUNT;
    ASSERT_TRUE(table.Resolve(100, out));
    EXPECT_EQ(out, BrandId::Shadow);

    table.Clear();
    EXPECT_EQ(table.Size(), 0u);
    EXPECT_FALSE(table.Resolve(100, out));
}

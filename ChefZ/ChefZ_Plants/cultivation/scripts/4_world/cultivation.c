class Plant_Corn : PlantBase
{
    void Plant_Corn()
    {
        m_FullMaturityTime = 250;
    }
};
class Plant_Chili : PlantBase
{
    void Plant_Chili()
    {
        m_FullMaturityTime = 250;
    }
};

class ChefZ_Plant_Corn_Cob : SeedBase{};
class ChefZ_Plant_ChiliSeeds : SeedBase{};
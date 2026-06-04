#pragma once
#include <cstdint>

namespace growbed::domain
{
    enum class PlantSpecies : uint8_t
    {
        Lettuce = 0,
        Herb    = 1,
        Custom  = 2,
    };

    inline const char* plantName(PlantSpecies s)
    {
        switch (s) {
            case PlantSpecies::Lettuce: return "Lettuce";
            case PlantSpecies::Herb:    return "Herb";
            case PlantSpecies::Custom:  return "Custom";
            default:                    return "Unknown";
        }
    }

    static constexpr uint8_t kPlantSpeciesCount = 3;
}

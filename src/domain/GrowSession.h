#pragma once
#include "domain/PlantSpecies.h"
#include <cstdint>
#include <cstring>

namespace growbed::domain
{
    // ?ë¬¼ ?¬ë°° ?¸ì…˜. NVS???€?¥ë˜???„ì› ?¬íˆ¬????ë³µì›?©ë‹ˆ??
    struct GrowSession
    {
        PlantSpecies species    = PlantSpecies::Lettuce;
        bool         active     = false;
        uint32_t     startEpoch = 0;
        char         sessionId[16] = {};

        bool isValid() const
        {
            return static_cast<uint8_t>(species) < kPlantSpeciesCount
                && startEpoch > 0U
                && sessionId[0] != '\0';
        }
    };
}

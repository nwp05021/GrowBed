#pragma once
#include "policy/IPlantPolicy.h"
#include "policy/LettucePolicy.h"
#include "policy/HerbPolicy.h"
#include "policy/CustomPolicy.h"
#include "domain/PlantSpecies.h"
#include <memory>

namespace growbed::policy
{
    // ?ë¬¼ ì¢…ì— ë§ëŠ” ?•ì±… ?¸ìŠ¤?´ìŠ¤ë¥??ì„±?©ë‹ˆ??
    class PolicyFactory
    {
    public:
        static std::unique_ptr<IPlantPolicy> create(domain::PlantSpecies species)
        {
            switch (species) {
                case domain::PlantSpecies::Lettuce:
                    return std::make_unique<LettucePolicy>();
                case domain::PlantSpecies::Herb:
                    return std::make_unique<HerbPolicy>();
                case domain::PlantSpecies::Custom:
                default:
                    return std::make_unique<CustomPolicy>();
            }
        }
    };
}

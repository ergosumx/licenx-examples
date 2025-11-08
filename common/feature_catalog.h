#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "licenx/licenx.h"

namespace licenx_examples
{
    enum class FeatureState : std::uint8_t
    {
        Disabled = 0,
        Enabled = 1,
        Extended = 2,
    };

    inline FeatureState ToFeatureState(std::uint8_t trit) noexcept
    {
        switch (trit)
        {
        case 2:
            return FeatureState::Extended;
        case 1:
            return FeatureState::Enabled;
        default:
            return FeatureState::Disabled;
        }
    }

    inline std::string_view DescribeFeatureState(FeatureState state) noexcept
    {
        switch (state)
        {
        case FeatureState::Extended:
            return "extended";
        case FeatureState::Enabled:
            return "enabled";
        default:
            return "disabled";
        }
    }

    inline std::optional<licenx::ProductState> GetProductStateSafe()
    {
        try
        {
            return licenx::GetProductState();
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    inline FeatureState ResolveFeature(const licenx::ProductState& product, std::size_t index) noexcept
    {
        if (index >= product.featureStates.size())
        {
            return FeatureState::Disabled;
        }

        return ToFeatureState(product.featureStates[index]);
    }

    struct GpuFeatureCatalog
    {
        static constexpr std::size_t kRenderPreviewIndex = 0;
        static constexpr std::size_t kModelTrainingIndex = 1;

        static std::string DescribeName(std::size_t index)
        {
            switch (index)
            {
            case kRenderPreviewIndex:
                return "GPU Render Preview";
            case kModelTrainingIndex:
                return "GPU Model Training";
            default:
                return "Unknown GPU Feature";
            }
        }
    };

    struct CliFeatureCatalog
    {
        static constexpr std::size_t kFeature1Index = 0;
        static constexpr std::size_t kFeature2Index = 1;
        static constexpr std::size_t kFeature3Index = 2;
        static constexpr std::size_t kFeature4Index = 3;

        static std::string DescribeName(std::size_t index)
        {
            switch (index)
            {
            case kFeature1Index:
                return "CLI Feature 1";
            case kFeature2Index:
                return "CLI Feature 2";
            case kFeature3Index:
                return "CLI Feature 3";
            case kFeature4Index:
                return "CLI Feature 4";
            default:
                return "Unknown CLI Feature";
            }
        }
    };
}

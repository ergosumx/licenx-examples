#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <cstdlib>
#include <system_error>

#include "licenx/licenx.h"

#include "feature_catalog.h"

namespace
{
    struct Options
    {
        std::optional<std::string> licensePath;
        bool feature1{false};
        bool feature2{false};
        bool feature3{false};
        bool feature4{false};
        bool listOnly{false};
    };

    void PrintUsage()
    {
        std::cout << "Usage: licenx_example_cli [options]\n"
                  << "  --license <path>     Path to .licx file (defaults to examples/licenses/cli-full.licx)\n"
                  << "  --feature1           Exercise CLI feature 1\n"
                  << "  --feature2           Exercise CLI feature 2\n"
                  << "  --feature3           Exercise CLI feature 3\n"
                  << "  --feature4           Exercise CLI feature 4\n"
                  << "  --list               Print all feature states and exit\n"
                  << "  --help               Show this message\n";
    }

    Options ParseOptions(int argc, char* argv[])
    {
        Options opts;
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg{argv[i]};
            if (arg == "--license" && i + 1 < argc)
            {
                opts.licensePath = std::string{argv[++i]};
            }
            else if (arg == "--feature1")
            {
                opts.feature1 = true;
            }
            else if (arg == "--feature2")
            {
                opts.feature2 = true;
            }
            else if (arg == "--feature3")
            {
                opts.feature3 = true;
            }
            else if (arg == "--feature4")
            {
                opts.feature4 = true;
            }
            else if (arg == "--list")
            {
                opts.listOnly = true;
            }
            else if (arg == "--help")
            {
                PrintUsage();
                std::exit(0);
            }
            else
            {
                std::cerr << "Unrecognised argument: " << arg << "\n";
                PrintUsage();
                std::exit(1);
            }
        }

        if (!opts.feature1 && !opts.feature2 && !opts.feature3 && !opts.feature4)
        {
            opts.listOnly = true;
        }

        return opts;
    }

    std::string ResolveDefaultLicense(std::string_view executablePath)
    {
        std::error_code ec;
        std::filesystem::path exePath = std::filesystem::absolute(std::filesystem::path{executablePath}, ec);
        if (ec)
        {
            exePath = std::filesystem::current_path();
        }

        auto base = exePath.parent_path();
        return (base / ".." / "licenses" / "cli-full.licx").lexically_normal().string();
    }

    void SetDevOverride()
    {
    #if defined(_WIN32)
        _putenv_s("ERGOX_LICENX_DEV_OVERRIDE", "licenx.dev.override.confirmed");
    #else
        setenv("ERGOX_LICENX_DEV_OVERRIDE", "licenx.dev.override.confirmed", 1);
    #endif
    }

    void PrintDiagnostics(const licenx::Diagnostics& diag)
    {
        std::cout << "Status: " << static_cast<int>(diag.status) << "\n"
                  << "Message: " << diag.message << "\n";
        if (!diag.licensePath.empty())
        {
            std::cout << "License path: " << diag.licensePath << "\n";
        }
        if (!diag.leaseDecision.empty())
        {
            std::cout << "Lease decision: " << diag.leaseDecision << "\n";
        }
    }

    void PrintFeatureState(const std::string& name, licenx_examples::FeatureState state)
    {
        std::cout << name << ": " << licenx_examples::DescribeFeatureState(state) << "\n";
    }

    void ExerciseFeature(const std::string& name, licenx_examples::FeatureState state)
    {
        if (state == licenx_examples::FeatureState::Disabled)
        {
            std::cout << name << ": unavailable – falling back to restricted workflow\n";
            return;
        }

        if (state == licenx_examples::FeatureState::Extended)
        {
            std::cout << name << ": extended mode active – unlocking premium behaviour\n";
            return;
        }

        std::cout << name << ": standard mode active – proceeding normally\n";
    }
}

int main(int argc, char* argv[])
{
    const auto options = ParseOptions(argc, argv);

    SetDevOverride();

    std::cout << "[debug] cli example starting" << std::endl;

    licenx::Config config;
    const std::string licensePath = options.licensePath.value_or(ResolveDefaultLicense(argv[0]));
    config.licensePath = licensePath;

    const licenx::Status initStatus = licenx::Initialize(config);
    if (initStatus != licenx::Status::Ok)
    {
        std::cout << "LicenX initialisation failed – running in restricted mode\n";
        PrintDiagnostics(licenx::GetDiagnostics());
    }

    const auto productState = licenx_examples::GetProductStateSafe();

    auto resolveAndHandle = [&](std::size_t index, std::string name, bool requested)
    {
        const auto state = (productState ? licenx_examples::ResolveFeature(*productState, index)
                                         : licenx_examples::FeatureState::Disabled);
        std::cout << "[debug] " << name << " state=" << static_cast<int>(state)
                  << " requested=" << (requested ? "yes" : "no")
                  << " list=" << (options.listOnly ? "yes" : "no") << "\n";
        if (options.listOnly)
        {
            PrintFeatureState(name, state);
        }
        else if (requested)
        {
            ExerciseFeature(name, state);
        }
    };

    resolveAndHandle(licenx_examples::CliFeatureCatalog::kFeature1Index,
                     licenx_examples::CliFeatureCatalog::DescribeName(licenx_examples::CliFeatureCatalog::kFeature1Index),
                     options.feature1);
    resolveAndHandle(licenx_examples::CliFeatureCatalog::kFeature2Index,
                     licenx_examples::CliFeatureCatalog::DescribeName(licenx_examples::CliFeatureCatalog::kFeature2Index),
                     options.feature2);
    resolveAndHandle(licenx_examples::CliFeatureCatalog::kFeature3Index,
                     licenx_examples::CliFeatureCatalog::DescribeName(licenx_examples::CliFeatureCatalog::kFeature3Index),
                     options.feature3);
    resolveAndHandle(licenx_examples::CliFeatureCatalog::kFeature4Index,
                     licenx_examples::CliFeatureCatalog::DescribeName(licenx_examples::CliFeatureCatalog::kFeature4Index),
                     options.feature4);

    licenx::Shutdown();
    return 0;
}

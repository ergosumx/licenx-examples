# LicenX Reference Examples

The `examples/` folder ships two runnable applications that demonstrate how to gate
features with LicenX and how to degrade gracefully when licences are missing or
insufficient. Both samples assume the **dev** key profile and automatically set the
`ERGOX_LICENX_DEV_OVERRIDE=licenx.dev.override.confirmed` environment variable for
the current process.

## Contents

| Path                       | Description                                                      |
| -------------------------- | ---------------------------------------------------------------- |
| `cli_features/`            | Cross-platform console sample with four feature switches.        |
| `gui_gpu/`                 | Win32 GUI sample showcasing GPU render/training gates.           |
| `common/feature_catalog.h` | Shared helpers for interpreting feature trits.                   |
| `products/`                | JSON specs that drive product-code generation for the demo.      |
| `licenses/`                | Pre-issued dev licences that exercise success and failure paths. |

## Building

Examples are enabled by default. Reconfigure and build targets as needed:

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release --target licenx_example_cli
cmake --build build-vs --config Release --target licenx_example_gpu_gui
```

Set `-DLICENX_BUILD_EXAMPLES=OFF` to skip the targets in environments where they are
not required (for example, non-Windows CI builders).

## Product Codes & Feature Mapping

Feature trits follow the published architecture rules: the first four trits encode
metadata (ordinal), the remaining 34 represent features. The samples use the
following catalogue positions:

| Scenario | Product ID (ordinal) | Feature Index → Meaning                                | Example States                    |
| -------- | -------------------- | ------------------------------------------------------ | --------------------------------- |
| GPU      | 7                    | 0 → Render Preview, 1 → Model Training                 | 0=disabled, 1=enabled, 2=extended |
| CLI      | 8                    | 0 → Feature1, 1 → Feature2, 2 → Feature3, 3 → Feature4 | 0=disabled, 1=enabled, 2=extended |

Derived product numbers:

| Label             | Product Digits       |
| ----------------- | -------------------- |
| `GPU_PRO`         | `124152352653073347` |
| `GPU_RENDER_ONLY` | `122299332464221506` |
| `GPU_EXTENDED`    | `129711413219628870` |
| `GPU_DENIED`      | `116740271897665983` |
| `CLI_FULL`        | `141653098881118512` |
| `CLI_PARTIAL`     | `139594187560172022` |
| `CLI_EXTENDED`    | `149065179636525876` |
| `CLI_DENIED`      | `133417453597332552` |

Each entry can be reproduced by running:

```powershell
# Example: regenerate GPU PRO product digits
.uild-vs\Release\licenx_admin_cli.exe issue `
  --product-spec examples\products\gpu_full.json `
  --license 9000000000000000000000 `
  --expiry 2026-12-31 `
  --private-key .\.certs\dev\id_ed25519.private.hex `
  --out temp.licx
```

The CLI prints the product digits before signing. Remove `temp.licx` afterwards if
all you need is the product number.

## Supplied Licences

| File                   | Product           | Licence Digits           | Expiry     | Behaviour                                       |
| ---------------------- | ----------------- | ------------------------ | ---------- | ----------------------------------------------- |
| `gpu-pro.licx`         | `GPU_PRO`         | `8000000000000000000001` | 2026-12-31 | Enables both GPU features.                      |
| `gpu-render-only.licx` | `GPU_RENDER_ONLY` | `8000000000000000000002` | 2026-06-30 | Render enabled, training disabled.              |
| `gpu-extended.licx`    | `GPU_EXTENDED`    | `8000000000000000000004` | 2026-09-30 | Render extended, training enabled.              |
| `gpu-denied.licx`      | `GPU_DENIED`      | `8000000000000000000003` | 2025-12-31 | Both GPU features disabled.                     |
| `cli-full.licx`        | `CLI_FULL`        | `8100000000000000000001` | 2026-12-31 | All CLI features enabled.                       |
| `cli-partial.licx`     | `CLI_PARTIAL`     | `8100000000000000000002` | 2025-09-30 | Feature 1 & 3 enabled, others disabled.         |
| `cli-extended.licx`    | `CLI_EXTENDED`    | `8100000000000000000003` | 2026-03-31 | Feature 1 & 2 extended, 3 & 4 enabled.          |
| `cli-expired.licx`     | `CLI_FULL`        | `8100000000000000000004` | 2023-12-31 | Intentionally expired (initialisation failure). |

## CLI Sample

Run the console app with a chosen licence and feature switches:

```powershell
# List feature availability under the full licence
.uild-vs\Release\licenx_example_cli.exe --license examples\licenses\cli-full.licx --list

# Exercise individual features with the partial licence
.uild-vs\Release\licenx_example_cli.exe `
  --license examples\licenses\cli-partial.licx `
  --feature1 --feature2 --feature3 --feature4
```

Expected behaviour:

- Successful initialisation prints the product state and evaluates each requested
  feature. Disabled features report a fallback path rather than terminating.
- Using `cli-expired.licx` triggers a restricted-mode message while still producing
  graceful output for each requested flag.

## GUI Sample (Windows)

The GUI demo displays current licence status and exposes two buttons:

```powershell
# Default licence (gpu-pro.licx)
.uild-vs\Release\licenx_example_gpu_gui.exe

# Override licence path
.uild-vs\Release\licenx_example_gpu_gui.exe --license examples\licenses\gpu-denied.licx
```

- **Render Preview** button validates feature index 0.
- **Model Training** button validates feature index 1.

When a feature is disabled or the licence is invalid, the sample surfaces a warning
message while keeping the window responsive. Extended states (value `2`) are called
out explicitly.

## Regenerating Licences

Use the JSON specs under `examples/products/` with `licenx_admin_cli` to mint new
licences. Example for the CLI extended profile:

```powershell
.uild-vs\Release\licenx_admin_cli.exe issue `
  --product-spec examples\products\cli_extended.json `
  --license 8200000000000000000001 `
  --expiry 2026-06-30 `
  --private-key .\.certs\dev\id_ed25519.private.hex `
  --out examples\licenses\cli-extended.licx
```

Adjust licence digits and expiries to craft success/failure scenarios. The samples
consume whatever licence path you pass on the command line or, by default,
`examples/licenses/cli-full.licx` (CLI) and `examples/licenses/gpu-pro.licx` (GUI).

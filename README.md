# FlowGridLayout

A unified flow and grid layout control for Ultimate++ that combines the flexibility of flow layouts with optional grid placement, designed for modern UI patterns like toolbars, ribbons, galleries, and form layouts.

## Features

- **Flow-first layout** with automatic wrapping and primary-axis packing
- **Minimal API** — three enums, simple methods, clear defaults
- **Spacers & Expanders** — flexible spacing primitives that absorb leftover space
- **Clusters** — keep-together blocks that drop as units or allow internal wrapping
- **Virtual mode** — efficient rendering for large datasets (10k+ items) via callbacks
- **Grid placement** — optional explicit row/column positioning for specific items
- **Segmentation** — category dividers and headers for grouped content
- **Scrollbars** — integrated with configurable modes (Auto, Vertical, Horizontal, None)
- **Performance** — O(n) layout, zero per-paint heap allocations

## Quick Start

### Basic Flow Layout (Toolbar)

```cpp
#include "FlowGridLayout.h"

FlowGridLayout toolbar;
toolbar.SetMode(FGLMode::Flow).SetWrap(true);

// Left: title
Label& title = *new Label("My App");
toolbar.Add(title);

// Middle: flexible space
toolbar.AddExpander();

// Right: buttons
for (int i = 0; i < 5; ++i) {
    Button& btn = *new Button();
    btn.SetLabel(Format("Btn%d", i));
    toolbar.Add(btn, -1, true, Size(80, 28));
}

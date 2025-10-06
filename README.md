# `AccordionCtrl`

**Collapsible section control for U++ (Ultimate++) applications.**

`AccordionCtrl` is a customizable container that manages a list of collapsible sections, allowing users to show or hide content panels efficiently. It provides standard accordion behavior with features like single-item expansion, optional animation, and locking sections in a fixed state.

-----

## ✨ Features

  * **Collapsible Sections:** Easily add and manage sections with distinct headers and content bodies.
  * **Single/Multi Expand Modes:** Configure the control to allow multiple sections open simultaneously, or enforce a classic accordion behavior where opening one closes all others (`SingleExpand`).
  * **Per-Section Locking:** Lock individual sections in an open or closed state, preventing user interaction from changing their status.
  * **Built-in Animation:** Smooth open/close animations are enabled by default and are fully configurable, including separate durations for opening and closing.
  * **Keyboard Navigation:** Full support for keyboard interaction (`Up`/`Down`/`Home`/`End` to navigate headers; `Space`/`Enter` to toggle).
  * **Customizable Style:** Uses U++'s **Chameleon** styling system for seamless integration with application themes.
  * **Header Widgets:** Supports adding interactive controls (like `Option` or `Button`) directly into the header pane without interfering with the toggle action.

-----

## 🛠️ Usage

To incorporate `AccordionCtrl` into your application, you typically add sections and populate their body controls:

```cpp
#include <AccordionCtrl/AccordionCtrl.h>
using namespace Upp;

// ... inside a ParentCtrl or TopWindow
AccordionCtrl accordion;
Add(accordion.SizePos()); // Add the control to your window

// Add sections
int section0 = accordion.AddSection("General Settings");
int section1 = accordion.AddSection("Advanced Options");

// Get the body control for a section and add widgets
ParentCtrl& body0 = accordion.BodyCtrl(section0);
body0.Add(EditString().HSizePos(10, 10).TopPos(5, 24)); // Example widget

// Configure behavior
accordion.SingleExpand(true);
accordion.SetAnimationEnabled(true);

// Open a section on startup
accordion.Open(section0, false);
```

### Key Configuration Methods:

| Method | Description |
| :--- | :--- |
| `SingleExpand(bool b)` | If `true`, only one section can be open at a time. |
| `AtLeastOneOpen(bool b)` | If `true`, prevents the last open section from being closed. |
| `SetLocked(int i, bool lock)` | Locks section `i` in its current open/closed state. |
| `SetAnimationEnabled(bool on)` | Toggles smooth animations for opening and closing. |

-----

## 🧪 Demo Status

<img width="758" height="447" alt="image" src="https://github.com/user-attachments/assets/3f20a7ae-cc1e-4123-80f9-289df5127f44" />

The demo showcases various features, including the use of **header widgets** (like the **Lock** checkboxes in the demo) and **animation controls**.

The control is currently **compiling and semi-stable**, but the demo may undergo minor changes as styling and advanced features are refined.

-----

Feel free to open an issue if you encounter any problems or have suggestions\!

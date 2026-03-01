---
name: pebble
description: Search Pebble SDK docs
---

# Pebble SDK Docs Skill

## When to use this skill
Activate whenever writing code for Pebble smartwatches, including watchfaces,
watchapps, or anything using the Pebble C SDK. Use it before writing any SDK
call you aren't 100% certain of — the C API docs are not in your training data.

## How the docs site is structured

Base URL: https://developer.rebble.io

The C API lives at `/docs/c/` and is organized as:
  /docs/c/{Module}/{Submodule}/

Each submodule page contains all functions, types, structs, and enums
for that submodule. Symbols are anchor-linked:
  /docs/c/{Module}/{Submodule}/#{SymbolName}

**Always fetch the submodule page, not the symbol anchor directly.**
The full page contains context about related symbols you'll want.

## Complete C API module tree

Foundation/
  App/
  App_Communication/
  App_Glance/
  AppMessage/
  AppSync/
  AppWorker/
  DataLogging/
  DataStructures/
    UUID/
  Dictation/
  Dictionary/
  Event_Service/
    AccelerometerService/
    AppFocusService/
    BatteryStateService/
    CompassService/
    ConnectionService/
    HealthService/
    TickTimerService/
  Exit_Reason/
  Internationalization/
  Launch_Reason/
  Logging/
  Math/
  Memory_Management/
  Platform/
  Resources/
    File_Formats/
  Rocky/
  Storage/
  Timer/
  Wakeup/
  Wall_Time/
  WatchInfo/

Graphics/
  Draw_Commands/
  Drawing_Paths/
  Drawing_Primitives/
  Drawing_Text/
  Fonts/
  Graphics_Context/
  Graphics_Types/
    Color_Definitions/

User_Interface/
  Animation/
    PropertyAnimation/
  Clicks/
  Layers/
    ActionBarLayer/
    BitmapLayer/
    MenuLayer/
    RotBitmapLayer/
    ScrollLayer/
    SimpleMenuLayer/
    StatusBarLayer/
    TextLayer/
  Light/
  Preferences/
  UnobstructedArea/
  Vibes/
  Window/
    ActionMenu/
    NumberWindow/
  Window_Stack/

Smartstrap/
Worker/

Standard_C/
  Format/
  Locale/
  Math/
  Memory/
  String/
  Time/

## How to look up a symbol

1. Identify which submodule the symbol belongs to using the tree above.
   - Drawing functions → Graphics/Drawing_Primitives/ or Graphics/Graphics_Context/
   - Text rendering → Graphics/Drawing_Text/ or User_Interface/Layers/TextLayer/
   - Time/date → Foundation/Wall_Time/ or Standard_C/Time/
   - Battery → Foundation/Event_Service/BatteryStateService/
   - Button input → User_Interface/Clicks/
   - Persistent storage → Foundation/Storage/

2. Fetch the submodule page:
   https://developer.rebble.io/docs/c/{Module}/{Submodule}/

3. Read the full page. It will contain all typedefs, structs, enums, and
   function signatures with parameters and descriptions.

## Lookup examples

Q: What are the parameters for text_layer_set_text()?
→ Fetch: https://developer.rebble.io/docs/c/User_Interface/Layers/TextLayer/

Q: How does BatteryChargeState work?
→ Fetch: https://developer.rebble.io/docs/c/Foundation/Event_Service/BatteryStateService/

Q: What drawing functions are available?
→ Fetch: https://developer.rebble.io/docs/c/Graphics/Drawing_Primitives/
   and:   https://developer.rebble.io/docs/c/Graphics/Graphics_Context/

Q: How do I subscribe to tick events?
→ Fetch: https://developer.rebble.io/docs/c/Foundation/Event_Service/TickTimerService/

## Other docs sections

Guides (conceptual, tutorial-style):
  https://developer.rebble.io/guides/

JavaScript/Rocky (JS on watch):
  https://developer.rebble.io/docs/rockyjs/

PebbleKit JS (phone-side JS):
  https://developer.rebble.io/docs/pebblekit-js/

## Important caveats

- The Pebble C SDK targets a constrained ARM Cortex-M device. Heap is ~24KB.
  Avoid dynamic allocation patterns that would be fine on normal systems.
- `APP_LOG` is the debug logger, not printf.
- All UI must run on the main app task. AppWorker is a separate background task.
- Platform differences exist between Aplite (B&W), Basalt (color), Chalk
  (round), and Diorite. Use PBL_IF_* macros and check /docs/c/Foundation/Platform/.
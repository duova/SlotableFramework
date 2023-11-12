# SlotableFramework

A framework built in Unreal Engine 5 for character based competitive multiplayer games that aims to offer a designer centric iteration workflow and fluid netcode implementation with minimum performance compromise.

# Basic Concepts

Slotables - An object in an inventory. The framework allows these to take passive or active actions. Passive actions hook into events, while active actions are triggered by inputs. In theory this can represent anything from an in-game item to a status effect to an ability.

Inventory - Collection of slotables. These can be dynamic or static, in terms of how many slotables they can contain. They can also be preset with slotables, which can be locked.

Constituent - Building blocks of a slotable which can be reused to share functionality between slotables. These are themselves supposed to be scriptable by blueprints with targeters, operators, and events.

Forms - An APawn that is capable of holding slotables. This is just a conceptual definition, as technically anything with the right components can be considered a "form".

Documentation outside of in-code commenting is WIP.

# Main Features

- Simplified Multiplayer Ability Blueprint Workflow
- Inventory System
- Predicted Audiovisuals System
- Predicted Resources System
- Health System
- Stats System
- Currency and Shop System
- Predicted Timers
- Lag Compensated Line Traces
- Curve-driven Movement Functions
- Volume-based Net Relevancy
- Blueprint-based Gauntlet-driven Testing Framework

# Gauntlet

The fork of the Unreal Engine source code used to compile this project - which includes SF gauntlet code - can be found here: https://github.com/duova/SfUnrealEngine

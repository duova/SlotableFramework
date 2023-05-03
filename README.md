# SlotableFramework
A framework built in Unreal Engine 5 for character based multiplayer games requiring modularity and quality netcode.

# Basic Concepts

Slotables - An object in an inventory. The framework allows these to take "passive" or "active" actions. Passive actions hook into events, while active actions are triggered by inputs. In theory this can represent anything from an in-game item to a status effect to an ability.

Inventory - Inventories of slotables. These can be dynamic or static, in terms of how many slotables they can contain. They can also be preset with slotables, which can be locked.

Constituent - Building blocks of a slotable which can be reused to share functionality between slotables. These are themselves supposed to be scriptable by blueprints with targeters, operators, and events.

Forms - An APawn that is capable of holding slotables. This is just a conceptual definition, as technically anything with the right components can be considered a "form".

Targeters - Blueprint functions that produce some sort of targeting effect. This could be a projectile or a hitscan or many other effects. The function returns the hit data in some way or another. The goal with targeters is to abstract the most common pieces of ability logic that can be composited to form most abilities, particularly ones involving multiple steps.

Operators - Blueprint functions that produces some sort of change to a target. This could be changing a stat, applying force, or spawning a form. (note that targets can be positions or AActors) Combined with the flexability of blueprints and the targeters, in theory only the bare minimum logic needs to be defined when creating an ability.

Vfx, Animation, and Audio functions - Blueprint functions that serve as an abstraction for audiovisual feature calls. These have prediction and replication in mind. They are designed for humanoid characters - where the usage is as simplified as possible - but can still be used for other rigs.

Other concepts are generally self-explanatory.

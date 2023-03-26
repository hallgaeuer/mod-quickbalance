
## QuickBalanceModule 

This module is intended to provide fixed balance adjustments for raids and dungeons (NOT autobalancing).

It creates a new table `mod_quickbalance_modifier` in the world database containing the following fields (see also sql folder):
* `Map` - Map-ID the scaling should be applied to
* `Creature` - Creature Entry the scaling should be applied to (optional). If empty (0), the scaling will be applied to all creatures in  the `Map`
* `DamageModifier` - All damage received by the player is multiplied by this modifier. So if you want to reduce damage by half, this should be set to `0.5`
* `HealthModifier` - Creature health is multiplied by this modifier
* `ManaModifier` - Creature mana is multiplied by this modifier
* `ArmorModifier` - Not implemented yet, placeholder for future updates
* `Comment` - Not used anywhere in code, just intended for better readability when viewing database rows.

## Hot reloading

You can directly reload changes in the `mod_quickbalance_modifier` table by typing `.reload config` in game or in the worldserver console.  
Might be subject to change in the future to another command, since the database data is technically not a config.

## TODO

Probably needs some more logic to correctly handle heroic modes and whatnot.

## Credits 

Code is inspired by [mod_autobalance](https://github.com/azerothcore/mod-autobalance)

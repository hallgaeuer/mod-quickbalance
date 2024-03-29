
## mod-quickbalance 

This module is intended to provide fixed balance adjustments for raids and dungeons (NOT autobalancing).

It creates multiple new tables in the world database: 
* `mod_quickbalance_modifier_map` Allows to define multiplier values for whole maps.  
* `mod_quickbalance_modifier_creature` Allows to define multiplier values for specific creatures (by creature entry). Does override the map value, if one exists (map + creature modifiers do NOT get multiplied with each other)
* `mod_quickbalance_modifier_spells` Allows to define a damage multiplier for specific spells (by spell id). Does override the map or creature value, if one exists (modifiers do NOT get multiplied with each other)
 
Both `mod_quickbalance_modifier_map` and `mod_quickbalance_modifier_creature` tables share the following common fields:

* `Difficulty` - The difficulty this modifier should be applied to.
* * 0 = Normal Dungeon / 10 man normal raid
* * 1 = Heroic Dungeon / 25 man normal raid
* * 2 = 10 man heroic raid
* * 3 = 25 man heroic raid
* `DamageModifier` - All damage received by the player is multiplied by this modifier. So if you want to reduce damage by half, this should be set to `0.5`
* `HealthModifier` - Creature health is multiplied by this modifier
* `ManaModifier` - Creature mana is multiplied by this modifier
* `ArmorModifier` - Not implemented yet, placeholder for future updates
* `Comment` - Not used anywhere in code, just intended for better readability when viewing database rows.

The table `mod_quickbalance_modifier_spells` currently has the following columns
* `Spell` - ID of the spell to modify
* `DamageModifier` - All damage received by the player through this spell is multiplied by this modifier.
* `Comment` - Not used anywhere in code, just intended for better readability when viewing database rows.

## Hot reloading

You can directly reload changes in the `mod_quickbalance_modifier_*` tables by typing `.reload config` in game or in the worldserver console.  

## TODO

* Implement ArmorModifier

## Credits 

Code is inspired by [mod_autobalance](https://github.com/azerothcore/mod-autobalance)

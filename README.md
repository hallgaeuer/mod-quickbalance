
## QuickBalanceModule 

This module is intended to provide fixed balance adjustments for raids and dungeons (NOT autobalancing).

It creates multiple new tables in the world database: 
* `mod_quickbalance_modifier_map` Allows to define multiplier vlues for whole maps.  
* `mod_quickbalance_modifier_creature` Allows to define multiplier values for specific creatures (by creature entry). Does override the map value, if one exists
 
Each table has the following common fields:

* `DamageModifier` - All damage received by the player is multiplied by this modifier. So if you want to reduce damage by half, this should be set to `0.5`
* `HealthModifier` - Creature health is multiplied by this modifier
* `ManaModifier` - Creature mana is multiplied by this modifier
* `ArmorModifier` - Not implemented yet, placeholder for future updates
* `Comment` - Not used anywhere in code, just intended for better readability when viewing database rows.

## Hot reloading

You can directly reload changes in the `mod_quickbalance_modifier` table by typing `.reload config` in game or in the worldserver console.  
Might be subject to change in the future to another command, since the database data is technically not a config.

## TODO

* Implement ArmorModifier
* Implement logic for another table mod_quickbalance_modifier_spell to easily adjust the damage of specific spells

## Credits 

Code is inspired by [mod_autobalance](https://github.com/azerothcore/mod-autobalance)

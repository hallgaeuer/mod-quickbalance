INSERT IGNORE INTO mod_quickbalance_modifier_creature (CreatureEntry, Difficulty, DamageModifier, HealthModifier, ManaModifier, ArmorModifier, Comment)
SELECT  Creature, 0, DamageModifier, HealthModifier, ManaModifier, ArmorModifier, Comment FROM mod_quickbalance_modifier WHERE Creature > 0;

INSERT IGNORE INTO mod_quickbalance_modifier_map (Map, Difficulty, DamageModifier, HealthModifier, ManaModifier, ArmorModifier, Comment)
SELECT  Map, 0, DamageModifier, HealthModifier, ManaModifier, ArmorModifier, Comment FROM mod_quickbalance_modifier WHERE Map > 0 AND Creature = 0;
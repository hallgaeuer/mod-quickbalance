CREATE TABLE IF NOT EXISTS mod_quickbalance_modifier_map
(
    Map INT DEFAULT 0 NOT NULL,
    Difficulty INT DEFAULT 0 NOT NULL,
    DamageModifier FLOAT DEFAULT 1 NOT NULL,
    HealthModifier FLOAT DEFAULT 1 NOT NULL,
    ManaModifier FLOAT DEFAULT 1 NOT NULL,
    ArmorModifier FLOAT DEFAULT 1 NOT NULL,
    Comment CHAR(100) DEFAULT '',

    PRIMARY KEY (Map, Difficulty)
);

CREATE TABLE IF NOT EXISTS mod_quickbalance_modifier_creature
(
    CreatureEntry INT DEFAULT 0 NOT NULL,
    Difficulty INT DEFAULT 0 NOT NULL,
    DamageModifier FLOAT DEFAULT 1 NOT NULL,
    HealthModifier FLOAT DEFAULT 1 NOT NULL,
    ManaModifier FLOAT DEFAULT 1 NOT NULL,
    ArmorModifier FLOAT DEFAULT 1 NOT NULL,
    Comment CHAR(100) DEFAULT '',

    PRIMARY KEY (CreatureEntry, Difficulty)
);

CREATE TABLE IF NOT EXISTS mod_quickbalance_modifier_spell
(
    Spell INT DEFAULT 0 NOT NULL,
    DamageModifier FLOAT DEFAULT 1 NOT NULL,
    Comment CHAR(100) DEFAULT '',

    PRIMARY KEY (Spell)
);
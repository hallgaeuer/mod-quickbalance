CREATE TABLE IF NOT EXISTS mod_quickbalance_modifier
(
    Map INT DEFAULT 0 NOT NULL,
    Creature INT DEFAULT 0 NOT NULL,
    DamageModifier FLOAT DEFAULT 1 NOT NULL,
    HealthModifier FLOAT DEFAULT 1 NOT NULL,
    ManaModifier FLOAT DEFAULT 1 NOT NULL,
    ArmorModifier FLOAT DEFAULT 1 NOT NULL,
    Comment CHAR(100) DEFAULT '',

    PRIMARY KEY (Map, Creature)
);
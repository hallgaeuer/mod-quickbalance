/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "Configuration/Config.h"
#include "Unit.h"
#include "Chat.h"
#include "Creature.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "World.h"
#include "Map.h"
#include "ScriptMgr.h"
#include <vector>
#include "QuickBalance.h"
#include "ScriptMgrMacros.h"
#include "Group.h"
#include "SharedDefines.h"
#include <boost/functional/hash.hpp>

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Acore::ChatCommands;

struct QuickBalanceMultiplierConfig {
    float DamageMultiplier = 1;
    float HealthMultiplier = 1;
    float ManaMultiplier = 1;
    float ArmorMultiplier = 1;
};

struct QuickBalanceSpellMultiplierConfig {
    float DamageMultiplier = 1;
};

class QuickBalanceCreatureInfo : public DataMap::Base {
public:
    QuickBalanceCreatureInfo() = default;

    QuickBalanceCreatureInfo(float damageMultiplier, float healthMultiplier, float manaMultiplier)
            : DamageMultiplier(damageMultiplier), HealthMultiplier(healthMultiplier), ManaMultiplier(manaMultiplier) {}

    uint32 OriginalHealth = 0;
    uint32 OriginalMana = 0;
    float DamageMultiplier = 1;
    float HealthMultiplier = 1;
    float ManaMultiplier = 1;
    uint32 BalanceDataTime = 0;
};

typedef std::pair<uint32, uint8> int32int8Pair;
typedef std::unordered_map<int32int8Pair, QuickBalanceMultiplierConfig, boost::hash<int32int8Pair>> modifierStorage;

static bool enabled;
static float globalHealthMultiplier, globalDamageMultiplier;
static uint32 balanceDataLoadedTimestamp = 0;
// Modifiers for maps
static modifierStorage mapConfigurations;
// Modifiers for creatures
static modifierStorage creatureConfigurations;
// Modifiers for spells
static std::unordered_map<uint32, QuickBalanceSpellMultiplierConfig> spellConfigurations;

class QuickBalance_WorldScript : public WorldScript {
public:
    QuickBalance_WorldScript()
            : WorldScript("QuickBalance_WorldScript") {
    }

    void OnBeforeConfigLoad(bool /*reload*/) override {
        SetInitialWorldSettings();
    }

    void OnStartup() override {
    }

    void LoadModifiers() {
        LoadMapModifiers();
        LoadCreatureModifiers();
        LoadSpellModifiers();

        balanceDataLoadedTimestamp = (uint32) round(std::time(nullptr));
        LOG_INFO("module", "QuickBalance: Modifier data loaded at {}", balanceDataLoadedTimestamp);
    }

    void LoadMapModifiers() {
        mapConfigurations.clear();
        QueryResult result = WorldDatabase.Query(
                "SELECT Map, Difficulty, DamageModifier, HealthModifier, ManaModifier, ArmorModifier FROM mod_quickbalance_modifier_map");

        if (!result) {
            return;
        }

        do {
            Field *fields = result->Fetch();

            uint32 map = fields[0].Get<uint32>();
            uint32 difficulty = fields[1].Get<uint32>();

            QuickBalanceMultiplierConfig modifiers{
                    fields[2].Get<float>(),
                    fields[3].Get<float>(),
                    fields[4].Get<float>(),
                    fields[5].Get<float>()
            };

            mapConfigurations.insert(
                    std::pair<int32int8Pair, QuickBalanceMultiplierConfig>(
                            int32int8Pair(map, difficulty),
                            modifiers
                    )
            );

        } while (result->NextRow());
    }

    void LoadCreatureModifiers() {
        creatureConfigurations.clear();
        QueryResult result = WorldDatabase.Query(
                "SELECT CreatureEntry, Difficulty, DamageModifier, HealthModifier, ManaModifier, ArmorModifier FROM mod_quickbalance_modifier_creature");

        if (!result) {
            return;
        }

        do {
            Field *fields = result->Fetch();

            uint32 creatureEntry = fields[0].Get<uint32>();
            uint32 difficulty = fields[1].Get<uint32>();

            QuickBalanceMultiplierConfig modifiers{
                    fields[2].Get<float>(),
                    fields[3].Get<float>(),
                    fields[4].Get<float>(),
                    fields[5].Get<float>()
            };

            creatureConfigurations.insert(
                    std::pair<int32int8Pair, QuickBalanceMultiplierConfig>(
                            int32int8Pair(creatureEntry, difficulty),
                            modifiers
                    )
            );

        } while (result->NextRow());
    }

    void LoadSpellModifiers() {
        spellConfigurations.clear();
        QueryResult result = WorldDatabase.Query(
                "SELECT Spell, DamageModifier FROM mod_quickbalance_modifier_spell");

        if (!result) {
            return;
        }

        do {
            Field *fields = result->Fetch();

            uint32 spellId = fields[0].Get<uint32>();

            QuickBalanceSpellMultiplierConfig modifiers{
                    fields[1].Get<float>(),
            };

            spellConfigurations.insert(
                    std::pair<uint32, QuickBalanceSpellMultiplierConfig>(
                            spellId,
                            modifiers
                    )
            );

        } while (result->NextRow());
    }

    void SetInitialWorldSettings() {
        // Initialize stuff and read config
        enabled = sConfigMgr->GetOption<bool>("QuickBalance.enable", 1);
        globalHealthMultiplier = sConfigMgr->GetOption<float>("QuickBalance.rate.health", 1.0f);
        globalDamageMultiplier = sConfigMgr->GetOption<float>("QuickBalance.rate.damage", 1.0f);

        LoadModifiers();
    }
};

class QuickBalance_PlayerScript : public PlayerScript {
public:
    QuickBalance_PlayerScript()
            : PlayerScript("QuickBalance_PlayerScript") {
    }

    void OnLogin(Player *player) override {
        if ((sConfigMgr->GetOption<bool>("QuickBalanceAnnounce.enable", true)) &&
            (sConfigMgr->GetOption<bool>("QuickBalance.enable", true))) {
            ChatHandler(player->GetSession()).PSendSysMessage(
                    "This server is running the |cff4CFF00QuickBalance |rmodule.");
        }
    }
};

class QuickBalance_UnitScript : public UnitScript {
public:
    QuickBalance_UnitScript()
            : UnitScript("QuickBalance_UnitScript", true) {
    }

    uint32 DealDamage(Unit *AttackerUnit, Unit *playerVictim, uint32 damage, DamageEffectType /*damagetype*/) override {
        return damage * _DetermineDamageModifier(playerVictim, AttackerUnit, nullptr);
    }

    void ModifyPeriodicDamageAurasTick(Unit *target, Unit *attacker, uint32 &damage, SpellInfo const *spellInfo) override {
        damage = damage * _DetermineDamageModifier(target, attacker, spellInfo);
    }

    void ModifySpellDamageTaken(Unit *target, Unit *attacker, int32 &damage, SpellInfo const *spellInfo) override {
        damage = damage * _DetermineDamageModifier(target, attacker, spellInfo);
    }

    void ModifyMeleeDamage(Unit *target, Unit *attacker, uint32 &damage) override {
        damage = damage * _DetermineDamageModifier(target, attacker, nullptr);
    }

    float _DetermineDamageModifier(Unit */*target*/, Unit *attacker, SpellInfo const *spellInfo) {
        if (!enabled)
            return 1;

        // Determine modifier by Spell
        if (spellInfo) {
            auto iterator = spellConfigurations.find(spellInfo->Id);
            if (iterator != spellConfigurations.end()) {
                return iterator->second.DamageMultiplier;
            }
        }

        // Determine modifier by creature
        if (!attacker || attacker->GetTypeId() == TYPEID_PLAYER || !attacker->IsInWorld())
            return 1;

        if ((attacker->IsHunterPet() || attacker->IsPet() || attacker->IsSummon()) && attacker->IsControlledByPlayer())
            return 1;

        return attacker->CustomData.GetDefault<QuickBalanceCreatureInfo>("QuickBalanceCreatureInfo")->DamageMultiplier;
    }
};


class QuickBalance_AllCreatureScript : public AllCreatureScript {
public:
    QuickBalance_AllCreatureScript()
            : AllCreatureScript("QuickBalance_AllCreatureScript") {
    }

    // This hook is only really necessary to allow for "Hot reloading" by using "".reload config"
    void OnAllCreatureUpdate(Creature *creature, uint32 /*diff*/) override {
        auto *creatureInfo = creature->CustomData.GetDefault<QuickBalanceCreatureInfo>("QuickBalanceCreatureInfo");

        if (creatureInfo->BalanceDataTime > 0 && creatureInfo->BalanceDataTime != balanceDataLoadedTimestamp) {
            ModifyCreatureAttributes(creature);
        }
    }

    void Creature_SelectLevel(const CreatureTemplate* /*cinfo*/, Creature* creature) override
    {
        ModifyCreatureAttributes(creature);
    }

    void ModifyCreatureAttributes(Creature *creature) {
        if (!enabled) {
            return;
        }

        if (!creature || !creature->GetMap()) {
            LOG_DEBUG("module.Quickbalance", "ModifyCreatureAttributes: Invalid creature or map");
            return;
        }

        // Removed the IsAlive check: (Some?) bosses are not "alive" while they respawn after a wipe (e.g. Kel'Thuzad)
        //if (!creature->IsAlive()) {
        //    LOG_ERROR("module.Quickbalance", "ModifyCreatureAttributes: Creature is not alive");
        //    return;
        //}

        if (!creature->GetMap()->IsDungeon() && !creature->GetMap()->IsBattleground()) {
            LOG_DEBUG("module.Quickbalance", "ModifyCreatureAttributes: Map is not a dungeon");
            return;
        }

        if (((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) &&
             creature->IsControlledByPlayer())) {
            LOG_DEBUG("module.Quickbalance", "ModifyCreatureAttributes: Creature is pet");
            return;
        }

        auto *creatureInfo = creature->CustomData.GetDefault<QuickBalanceCreatureInfo>("QuickBalanceCreatureInfo");

        // Store original max health and mana on first call
        if (creatureInfo->BalanceDataTime == 0) {
            creatureInfo->OriginalHealth = creature->GetMaxHealth();
            creatureInfo->OriginalMana = creature->GetMaxPower(POWER_MANA);
        }

        uint32 mapId = creature->GetMap()->GetEntry()->MapID;
        uint8 difficulty = creature->GetMap()->GetSpawnMode();

        std::optional<QuickBalanceMultiplierConfig> config;

        if (!config.has_value()) {
            int32int8Pair creatureAndDifficulty = std::make_pair(creature->GetEntry(), difficulty);

            auto creatureIterator = creatureConfigurations.find(creatureAndDifficulty);
            if (creatureIterator != creatureConfigurations.end()) {
                config.emplace(creatureIterator->second);
            }
        }

        if (!config.has_value()) {
            int32int8Pair mapAndDifficulty = std::make_pair(mapId, difficulty);

            auto mapIterator = mapConfigurations.find(mapAndDifficulty);
            if (mapIterator != mapConfigurations.end()) {
                config.emplace(mapIterator->second);
            }
        }

        if (!config.has_value()) {
            // Nothing to do. Set flag in creatureinfo to skip the above steps on next calculation
            creatureInfo->BalanceDataTime = balanceDataLoadedTimestamp;

            return;
        }

        creatureInfo->BalanceDataTime = balanceDataLoadedTimestamp;
        creatureInfo->DamageMultiplier = config.value().DamageMultiplier * globalDamageMultiplier;
        creatureInfo->HealthMultiplier = config.value().HealthMultiplier * globalHealthMultiplier;
        creatureInfo->ManaMultiplier = config.value().ManaMultiplier;

        float healthPercentage = creature->GetHealthPct();
        float manaPercentage = creature->GetPowerPct(POWER_MANA);
        Powers prevPowerType = creature->getPowerType();

        uint32 scaledMaxHealth = round((float) creatureInfo->OriginalHealth * creatureInfo->HealthMultiplier);
        uint32 scaledMaxMana = round((float) creatureInfo->OriginalMana * creatureInfo->ManaMultiplier);

        LOG_DEBUG("module.Quickbalance", "ModifyCreatureAttributes: Set health of creature {} ({}) from {} to {}", creature->GetName(), creature->GetGUID().ToString(), creatureInfo->OriginalHealth, scaledMaxHealth);

        creature->SetCreateHealth(scaledMaxHealth);
        creature->SetMaxHealth(scaledMaxHealth);
        creature->ResetPlayerDamageReq();
        creature->SetCreateMana(scaledMaxMana);
        creature->SetMaxPower(POWER_MANA, scaledMaxMana);
        creature->SetModifierValue(UNIT_MOD_ENERGY, BASE_VALUE, (float) 100.0f);
        creature->SetModifierValue(UNIT_MOD_RAGE, BASE_VALUE, (float) 100.0f);
        creature->SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, (float) scaledMaxHealth);
        creature->SetModifierValue(UNIT_MOD_MANA, BASE_VALUE, (float) scaledMaxMana);

        uint32 scaledCurHealth = float(scaledMaxHealth) * (healthPercentage / 100);
        uint32 scaledCurMana = float(scaledMaxMana) * (manaPercentage / 100);

        creature->SetHealth(scaledCurHealth);
        if (prevPowerType == POWER_MANA)
            creature->SetPower(POWER_MANA, scaledCurMana);
        else
            creature->setPowerType(prevPowerType); // fix creatures with different power types

        creature->UpdateAllStats();
    }
};

void AddQuickBalanceScripts() {
    new QuickBalance_WorldScript();
    new QuickBalance_PlayerScript();
    new QuickBalance_UnitScript();
    new QuickBalance_AllCreatureScript();
}

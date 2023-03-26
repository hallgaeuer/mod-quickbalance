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

/*
 * Script Name: QuickBalance
 * Original Authors: KalCorp and Vaughner
 * Maintainer(s): AzerothCore
 * Original Script Name: QuickBalance
 * Description: Based on mod_quickbalance https://github.com/azerothcore/mod-quickbalance
 *     This module is intended to provide more direct controls to the balancing by just directly taking in modifiers AND allowing specific modifiers per create.
 *     This module DOES NOT autobalance. It is intended for servers that want to provide fixed balancing adjustments
 */

#include "Configuration/Config.h"
#include "Unit.h"
#include "Chat.h"
#include "Creature.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "MapMgr.h"
#include "World.h"
#include "Map.h"
#include "ScriptMgr.h"
#include "Language.h"
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

class QuickBalanceMultiplierConfig {
  public:
    QuickBalanceMultiplierConfig(float damageMultiplier, float healthMultiplier, float manaMultiplier, float armorMultiplier)
        : DamageMultiplier(damageMultiplier), HealthMultiplier(healthMultiplier), ManaMultiplier(manaMultiplier), ArmorMultiplier(armorMultiplier) {}

    float DamageMultiplier = 1;
    float HealthMultiplier = 1;
    float ManaMultiplier = 1;
    float ArmorMultiplier = 1;
};

class QuickBalanceCreatureInfo : public DataMap::Base
{
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

typedef std::pair<uint32, uint32> intPair;
static bool enabled;
static uint32 balanceDataLoadedTimestamp = 0;
static std::unordered_map<uint32, QuickBalanceMultiplierConfig> mapConfigurations;
static std::unordered_map<intPair , QuickBalanceMultiplierConfig, boost::hash<intPair>> mapCreatureConfigurations;

class QuickBalance_WorldScript : public WorldScript
{
    public:
    QuickBalance_WorldScript()
        : WorldScript("QuickBalance_WorldScript")
    {
    }

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        SetInitialWorldSettings();
    }
    void OnStartup() override
    {
    }

    void SetInitialWorldSettings()
    {
        // Initialize stuff and read config
        enabled = sConfigMgr->GetOption<bool>("QuickBalance.enable", 1);

        mapConfigurations.clear();
        mapCreatureConfigurations.clear();

        QueryResult result = WorldDatabase.Query("SELECT Map, Creature, DamageModifier, HealthModifier, ManaModifier, ArmorModifier FROM mod_quickbalance_modifier");

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();

                uint32 map = fields[0].Get<uint32>();
                uint32 creature = fields[1].Get<uint32>();
                float damageModifier = fields[2].Get<float>();
                float healthModifier = fields[3].Get<float>();
                float manaModifier = fields[4].Get<float>();
                float armorModifier = fields[5].Get<float>();

                QuickBalanceMultiplierConfig modifiers {damageModifier, healthModifier, manaModifier, armorModifier};

                // Determine what storage to push into
                // Map + Creature is set
                if (map && creature) {
                    mapCreatureConfigurations.insert(
                            std::pair<intPair, QuickBalanceMultiplierConfig>(
                                    intPair(map, creature),
                                    modifiers
                            )
                    );
                }
                else if (fields[0].Get<uint32>()) {
                    mapConfigurations.insert(
                            std::pair<uint32, QuickBalanceMultiplierConfig>(
                                    map,
                                    modifiers
                            )
                    );
                }

                balanceDataLoadedTimestamp = (uint32) round(std::time(nullptr));

                LOG_INFO("module", "QuickBalance: Data loaded at {}", balanceDataLoadedTimestamp);
            } while (result->NextRow());
        }
    }
};

class QuickBalance_PlayerScript : public PlayerScript
{
    public:
        QuickBalance_PlayerScript()
            : PlayerScript("QuickBalance_PlayerScript")
        {
        }

        void OnLogin(Player *player) override
        {
            if ((sConfigMgr->GetOption<bool>("QuickBalanceAnnounce.enable", true)) && (sConfigMgr->GetOption<bool>("QuickBalance.enable", true))) {
                ChatHandler(player->GetSession()).PSendSysMessage("This server is running the |cff4CFF00QuickBalance |rmodule.");
            }
        }
};

class QuickBalance_UnitScript : public UnitScript
{
    public:
    QuickBalance_UnitScript()
        : UnitScript("QuickBalance_UnitScript", true)
    {
    }

    uint32 DealDamage(Unit* AttackerUnit, Unit *playerVictim, uint32 damage, DamageEffectType /*damagetype*/) override
    {
        return _Modifer_DealDamage(playerVictim, AttackerUnit, damage);
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
    {
        damage = _Modifer_DealDamage(target, attacker, damage);
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
    {
        damage = _Modifer_DealDamage(target, attacker, damage);
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        damage = _Modifer_DealDamage(target, attacker, damage);
    }

    void ModifyHealReceived(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override {
        damage = _Modifer_DealDamage(target, attacker, damage);
    }


    uint32 _Modifer_DealDamage(Unit* target, Unit* attacker, uint32 damage)
    {
        if (!enabled)
            return damage;

        if (!attacker || attacker->GetTypeId() == TYPEID_PLAYER || !attacker->IsInWorld())
            return damage;

        if ((attacker->IsHunterPet() || attacker->IsPet() || attacker->IsSummon()) && attacker->IsControlledByPlayer())
            return damage;

        auto *creatureInfo = attacker->CustomData.GetDefault<QuickBalanceCreatureInfo>("QuickBalanceCreatureInfo");
        //LOG_INFO("module", "QuickBalance: DamageMultiplier {}", creatureInfo->DamageMultiplier);

        return damage * creatureInfo->DamageMultiplier;
    }
};


class QuickBalance_AllCreatureScript : public AllCreatureScript
{
public:
    QuickBalance_AllCreatureScript()
        : AllCreatureScript("QuickBalance_AllCreatureScript")
    {
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!enabled)
            return;

        ModifyCreatureAttributes(creature);
    }

    void ModifyCreatureAttributes(Creature* creature)
    {
        if (!creature || !creature->GetMap())
            return;

        if (!creature->IsAlive())
            return;

        if (!creature->GetMap()->IsDungeon() && !creature->GetMap()->IsBattleground())
            return;

        if (((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer()))
        {
            return;
        }

        auto *creatureInfo = creature->CustomData.GetDefault<QuickBalanceCreatureInfo>("QuickBalanceCreatureInfo");

        // Balancing was already applied and no recalculation is needed for this creature
        if (creatureInfo->BalanceDataTime == balanceDataLoadedTimestamp) {
            return;
        }

        // Store original max health and mana on first call
        if (creatureInfo->BalanceDataTime == 0) {
            creatureInfo->OriginalHealth = creature->GetMaxHealth();
            creatureInfo->OriginalMana = creature->GetMaxPower(POWER_MANA);
        }

        CreatureTemplate const *creatureTemplate = creature->GetCreatureTemplate();

        //CreatureTemplate const *creatureTemplate = creature->GetCreatureTemplate();
        InstanceMap* instanceMap = ((InstanceMap*)sMapMgr->FindMap(creature->GetMapId(), creature->GetInstanceId()));
        uint32 mapId = instanceMap->GetEntry()->MapID;

        std::optional<QuickBalanceMultiplierConfig> config;

        if (!config.has_value()) {
            // Specific Map + Creature Multiplier config
            intPair mapAndCreature = std::make_pair(mapId, creatureTemplate->Entry);

            auto mapCreatureIterator = mapCreatureConfigurations.find(mapAndCreature);
            if (mapCreatureIterator != mapCreatureConfigurations.end()) {
                config.emplace(mapCreatureIterator->second);
            }
        }

        if (!config.has_value()) {
            // Whole map multiplier config
            auto mapIterator = mapConfigurations.find(mapId);
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
        creatureInfo->DamageMultiplier = config.value().DamageMultiplier;
        creatureInfo->HealthMultiplier = config.value().HealthMultiplier;
        creatureInfo->ManaMultiplier = config.value().ManaMultiplier;

        float healthPercentage = creature->GetHealthPct();
        float manaPercentage = creature->GetPowerPct(POWER_MANA);
        Powers prevPowerType = creature->getPowerType();

        uint32 scaledMaxHealth = round((float) creatureInfo->OriginalHealth * creatureInfo->HealthMultiplier);
        uint32 scaledMaxMana = round((float) creatureInfo->OriginalMana * creatureInfo->ManaMultiplier);

        creature->SetCreateHealth(scaledMaxHealth);
        creature->SetMaxHealth(scaledMaxHealth);
        creature->ResetPlayerDamageReq();
        creature->SetCreateMana(scaledMaxMana);
        creature->SetMaxPower(POWER_MANA, scaledMaxMana);
        creature->SetModifierValue(UNIT_MOD_ENERGY, BASE_VALUE, (float)100.0f);
        creature->SetModifierValue(UNIT_MOD_RAGE, BASE_VALUE, (float)100.0f);
        creature->SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, (float)scaledMaxHealth);
        creature->SetModifierValue(UNIT_MOD_MANA, BASE_VALUE, (float)scaledMaxMana);

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

void AddQuickBalanceScripts()
{
    new QuickBalance_WorldScript();
    new QuickBalance_PlayerScript();
    new QuickBalance_UnitScript();
    new QuickBalance_AllCreatureScript();
}

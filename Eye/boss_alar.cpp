/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
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

/* ScriptData
SDName: boss_alar
SD%Complete: 0
SDComment: 3 босса на 3 платформах, выход€т по очереди с "$s вступает в бой." потом от куда-то прыгает крокодил "«убасть"
SDCategory: Tempest Keep, The Eye
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_eye.h"

enum eSpells
{
    SPELL_FLAME_BUFFET           = 34121, // Flame Buffet - every 1, 5 secs in phase 1 if there is no victim in melee range and after Dive Bomb in phase 2 with same conditions
    SPELL_FLAME_QUILLS           = 34229, // Randomly after changing position in phase after watching tons of movies, set probability 20%
    SPELL_REBIRTH                = 34342, // Rebirth - beginning of second phase(after losing all health in phase 1)
    SPELL_REBIRTH_2              = 35369, // Rebirth(another, without healing to full HP) - after Dive Bomb in phase 2
    SPELL_MELT_ARMOR             = 35410, // Melt Armor - every 60 sec in phase 2
    SPELL_CHARGE                 = 35412, // Charge - 30 sec cooldown
    SPELL_DIVE_BOMB_VISUAL       = 35367, // Bosskillers says 30 sec cooldown, wowwiki says 30 sec colldown, DBM and BigWigs addons says ~47 sec
    SPELL_DIVE_BOMB              = 35181, // after watching tonns of movies, set cooldown to 40+rand()%5.
    SPELL_BERSERK                = 45078, // 10 minutes after phase 2 starts(id is wrong, but proper id is unknown)

    CREATURE_EMBER_OF_ALAR       = 19551, // Al'ar summons one Ember of Al'ar every position change in phase 1 and two after Dive Bomb. Also in phase 2 when Ember of Al'ar dies, boss loses 3% health.
    SPELL_EMBER_BLAST            = 34133, // When Ember of Al'ar dies, it casts Ember Blast

    CREATURE_FLAME_PATCH_ALAR    = 20602, // Flame Patch - every 30 sec in phase 2
    SPELL_FLAME_PATCH            = 35380, //
};

static float waypoint[6][3] =
{
    {340.15f, 58.65f, 17.71f},
    {388.09f, 31.54f, 20.18f},
    {388.18f, -32.85f, 20.18f},
    {340.29f, -60.19f, 17.72f},
    {332.0f, 0.01f, 39.0f}, // better not use the same xy coord
    {331.0f, 0.01f, -2.39f}
};

enum WaitEventType
{
    WE_NONE     = 0,
    WE_DUMMY    = 1,
    WE_PLATFORM = 2,
    WE_QUILL    = 3,
    WE_DIE      = 4,
    WE_REVIVE   = 5,
    WE_CHARGE   = 6,
    WE_METEOR   = 7,
    WE_DIVE     = 8,
    WE_LAND     = 9,
    WE_SUMMON   = 10
};

class boss_alar : public CreatureScript
{
    public:

        boss_alar()
            : CreatureScript("boss_alar")
        {
        }
        struct boss_alarAI : public ScriptedAI
        {
            boss_alarAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            int8 cur_wp;

            void Reset()
            {
                if (instance)
                    instance->SetData(DATA_ALAREVENT, NOT_STARTED);

                me->setActive(false);
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (instance)
                    instance->SetData(DATA_ALAREVENT, IN_PROGRESS);

                me->SetUnitMovementFlags(MOVEMENTFLAG_DISABLE_GRAVITY); // after enterevademode will be set walk movement
                DoZoneInCombat();
                me->setActive(true);
				instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (instance)
                    instance->SetData(DATA_ALAREVENT, DONE);
            }

            void JustSummoned(Creature* summon)
            {
                if (summon->GetEntry() == CREATURE_EMBER_OF_ALAR)
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        summon->AI()->AttackStart(target);
            }

            void MoveInLineOfSight(Unit* /*who*/) {}

            void AttackStart(Unit* who)
            {
//                if (Phase1)
                    AttackStartNoMove(who);
               // else
                    ScriptedAI::AttackStart(who);
            }

            void DamageTaken(Unit* /*killer*/, uint32 &damage)
            {
            }

            void SpellHit(Unit*, const SpellInfo* spell)
            {
            }

            void MovementInform(uint32 type, uint32 /*id*/)
            {
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->isInCombat()) // sometimes isincombat but !incombat, faction bug?
                    return;

                DoMeleeAttackIfReady();
            }

            void DoMeleeAttackIfReady()
            {
                if (me->isAttackReady() && !me->IsNonMeleeSpellCasted(false))
                {
                    if (me->IsWithinMeleeRange(me->getVictim()))
                    {
                        me->AttackerStateUpdate(me->getVictim());
                        me->resetAttackTimer();
                    }
                    else
                    {
                        Unit* target = NULL;
                        target = me->SelectNearestTargetInAttackDistance(5);
                        if (target)
                            me->AI()->AttackStart(target);
                        else
                        {
                            DoCast(me, SPELL_FLAME_BUFFET, true);
                            me->setAttackTimer(BASE_ATTACK, 1500);
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_alarAI(creature);
        }
};

                        //if (Unit* Alar = Unit::GetUnit(*me, instance->GetData64(DATA_ALAR)))
   

void AddSC_boss_alar()
{
    new boss_alar();
    //new mob_ember_of_alar();
    //new mob_flame_patch_alar();
}


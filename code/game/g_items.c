/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2009-2011 Brian Labbie and Dave Richardson.

http://sourceforge.net/projects/alturt/

This file is part of Alturt source code.

Alturt source code is free software: you can redistribute it
and/or modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Alturt source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with Alturt source code.  If not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/
//
#include "g_local.h"

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/


#define RESPAWN_ARMOR           25
#define RESPAWN_HEALTH          35
#define RESPAWN_AMMO            40
#define RESPAWN_HOLDABLE        60
#define RESPAWN_MEGAHEALTH      35//120
#define RESPAWN_POWERUP         120


//======================================================================

int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
        int                     quantity;
        int                     i;
        gclient_t       *client;

        if ( !other->client->ps.powerups[ent->item->giTag] ) {
                // round timing to seconds to make multiple powerup timers
                // count in sync
                other->client->ps.powerups[ent->item->giTag] =1;
                       // level.time - ( level.time % 1000 );
        }

        if ( ent->count ) {
                quantity = ent->count;
        } else {
                quantity = ent->item->quantity;
        }

        other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

        // give any nearby players a "denied" anti-reward
        for ( i = 0 ; i < level.maxclients ; i++ ) {
                vec3_t          delta;
                float           len;
                vec3_t          forward;
                trace_t         tr;

                client = &level.clients[i];
                if ( client == other->client ) {
                        continue;
                }
                if ( client->pers.connected == CON_DISCONNECTED ) {
                        continue;
                }
                if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
                        continue;
                }

    // if same team in team game, no sound
    // cannot use OnSameTeam as it expects to g_entities, not clients
        if ( g_gametype.integer >= GT_TEAM && other->client->sess.sessionTeam == client->sess.sessionTeam  ) {
      continue;
    }

                // if too far away, no sound
                VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
                len = VectorNormalize( delta );
                if ( len > 192 ) {
                        continue;
                }

                // if not facing, no sound
                AngleVectors( client->ps.viewangles, forward, NULL, NULL );
                if ( DotProduct( delta, forward ) < 0.4 ) {
                        continue;
                }

                // if not line of sight, no sound
                trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
                if ( tr.fraction != 1.0 ) {
                        continue;
                }

                // anti-reward
                client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
        }
        return RESPAWN_POWERUP;
}

//======================================================================

#ifdef MISSIONPACK
int Pickup_PersistantPowerup( gentity_t *ent, gentity_t *other ) {
  /*
        int             clientNum;
        char    userinfo[MAX_INFO_STRING];
        float   handicap;
        int             max;

//        other->client->ps.stats[STAT_PERSISTANT_POWERUP] = ent->item - bg_itemlist;
        other->client->persistantPowerup = ent;

        switch( ent->item->giTag ) {
        case PW_GUARD:
                clientNum = other->client->ps.clientNum;
                trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
                handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
                if( handicap<=0.0f || handicap>100.0f) {
                        handicap = 100.0f;
                }
                max = (int)(2 *  handicap);

                other->health = max;
                other->client->ps.stats[STAT_HEALTH] = max;
                //STAT_MAX_HEALTH = max;
               // other->client->ps.stats[STAT_ARMOR] = max;
                other->client->pers.maxHealth = max;
                other->client->ps.stats[STAT_STAMINA] = max; //Xamis probably don't need these, since not using PW_GUARD but just in case
                other->client->pers.maxStamina = max; //Xamis

                break;

        case PW_SCOUT:
                clientNum = other->client->ps.clientNum;
                trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
                handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
                if( handicap<=0.0f || handicap>100.0f) {
                        handicap = 100.0f;
                }
                other->client->pers.maxHealth = handicap;
//                other->client->ps.stats[STAT_ARMOR] = 0;
                break;

        case PW_DOUBLER:
                clientNum = other->client->ps.clientNum;
                trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
                handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
                if( handicap<=0.0f || handicap>100.0f) {
                        handicap = 100.0f;
                }
                other->client->pers.maxHealth = handicap;
                break;
        case PW_AMMOREGEN:
                clientNum = other->client->ps.clientNum;
                trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
                handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
                if( handicap<=0.0f || handicap>100.0f) {
                        handicap = 100.0f;
                }
                other->client->pers.maxHealth = handicap;
                memset(other->client->ammoTimes, 0, sizeof(other->client->ammoTimes));
                break;
        default:
                clientNum = other->client->ps.clientNum;
                trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
                handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
                if( handicap<=0.0f || handicap>100.0f) {
                        handicap = 100.0f;
                }
                other->client->pers.maxHealth = handicap;
                break;
        }
*/
        return -1;
}

//======================================================================
#endif

int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {

//        other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

 //       if( ent->item->giTag == HI_KAMIKAZE ) {
 //               other->client->ps.eFlags |= EF_KAMIKAZE;
 //       }

        return RESPAWN_HOLDABLE;
}


//======================================================================

void Add_Ammo (gentity_t *ent, int weapon, int count)
{
  int clips;
  int rounds;
  if ( BG_Grenade(weapon) ){
    bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] += count;
  }else if ( weapon == WP_KNIFE &&  bg_weaponlist[weapon].rounds[ent->client->ps.clientNum] <6){
    bg_weaponlist[weapon].rounds[ent->client->ps.clientNum] += count;
  }else{
    clips = count / RoundCount(weapon);
    rounds = count % RoundCount(weapon);
    if( rounds == 0 && clips > 0){
     clips--;
     rounds = RoundCount(weapon);
    }
//G_Printf("count = %i, clips = %i, rounds = %i\n", count, clips, rounds );

//G_Printf("clips before add = %i\n", bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] );
//G_Printf("rounds before add = %i\n", bg_weaponlist[weapon].rounds[ent->client->ps.clientNum] );
  bg_weaponlist[weapon].rounds[ent->client->ps.clientNum] += rounds;
  bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] += clips;
  }

  //set max number of clips --Xamis
    if ( bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] > 3 ) {
    bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] = 4;
  }

  //ent->client->ps.ammo[weapon] += count;
  if ( BG_Grenade(weapon) && bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] > 2 ) {
    bg_weaponlist[weapon].numClips[ent->client->ps.clientNum] = 2;

  }
  if ( bg_weaponlist[weapon].rounds[ent->client->ps.clientNum] > RoundCount(weapon) ) {
    bg_weaponlist[weapon].rounds[ent->client->ps.clientNum] = RoundCount(weapon);

        }
}

int Pickup_Ammo (gentity_t *ent, gentity_t *other)
{
        int             quantity;

        if ( ent->count ) {
                quantity = ent->count;
        } else {
                quantity = ent->item->quantity;
        }

        Add_Ammo (other, ent->item->giTag, quantity);

        return RESPAWN_AMMO;
}

//======================================================================


int Pickup_Weapon (gentity_t *ent, gentity_t *other) {
        int             quantity;

        if ( ent->count < 0 ) {
                quantity = 0; // None for you, sir!
        } else {
                if ( ent->count ) {
                        quantity = ent->count;
                } else {
                        quantity = ent->item->quantity;
                }
        }
        if (ent->item->giTag == WP_KNIFE)
            quantity =1;

        // add the weapon
        //other->client->ps.stats[STAT_WEAPONS] |= ( 1 << ent->item->giTag );
        
	
	if (BG_Sidearm(ent->item->giTag)){
		other->client->pers.inventory[SIDEARM]= ent->item->giTag;
                                   bg_inventory.sort[other->client->ps.clientNum][SIDEARM] = ent->item->giTag;
                                   BG_PackWeapon( ent->item->giTag, other->client->ps.stats );
        }
	if ( BG_Secondary(ent->item->giTag) && (!bg_inventory.sort[other->client->ps.clientNum][SECONDARY])){
		other->client->pers.inventory[SECONDARY]= ent->item->giTag;
                                    bg_inventory.sort[other->client->ps.clientNum][SECONDARY] = ent->item->giTag;
                                    BG_PackWeapon( ent->item->giTag, other->client->ps.stats );
                                   //G_Printf("Adding weapon as SECONDARY\n");
                                   
                                   
                }else if ( BG_Secondary(ent->item->giTag) && (!bg_inventory.sort[other->client->ps.clientNum][PRIMARY])){
		other->client->pers.inventory[PRIMARY]= ent->item->giTag;
                                    bg_inventory.sort[other->client->ps.clientNum][PRIMARY] = ent->item->giTag;
                                    BG_PackWeapon( ent->item->giTag, other->client->ps.stats );
                                   //G_Printf("Adding weapon as PRIMARY\n");
                                   
                                   
                }else  if ( BG_Primary(ent->item->giTag)){
                                  other->client->pers.inventory[PRIMARY]= ent->item->giTag;
                                  bg_inventory.sort[other->client->ps.clientNum][PRIMARY] = ent->item->giTag;
                                  BG_PackWeapon( ent->item->giTag, other->client->ps.stats );
                                  //G_Printf("Adding weapon as PRIMARY\n");
        }

          Add_Ammo( other, ent->item->giTag, ( quantity ));

//      if (ent->item->giTag == WP_GRAPPLING_HOOK)
//              other->client->ps.ammo[ent->item->giTag] = -1; // unlimited ammo

        // team deathmatch has slow weapon respawns
        if ( g_gametype.integer == GT_TEAM ) {
                return g_weaponTeamRespawn.integer;
        }

        return g_weaponRespawn.integer;
}


//======================================================================

int Pickup_Health (gentity_t *ent, gentity_t *other) {
        int                     max;
        int                     quantity;

        // small and mega healths will go over the max
#ifdef MISSIONPACK
//        if( other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
//                max = STAT_MAX_HEALTH;
//        }
//        else
#endif
        if ( ent->item->quantity != 5 && ent->item->quantity != 100 ) {
                max = STAT_MAX_HEALTH;
        } else {
                max = STAT_MAX_HEALTH * 2;
        }

        if ( ent->count ) {
                quantity = ent->count;
        } else {
                quantity = ent->item->quantity;
        }

        other->health += quantity;

        if (other->health > max ) {
                other->health = max;
        }
        other->client->ps.stats[STAT_HEALTH] = other->health;
        other->client->ps.stats[STAT_STAMINA] = other->health; //Xamis, not really needed but just in case


        if ( ent->item->quantity == 100 ) {             // mega health respawns slow
                return RESPAWN_MEGAHEALTH;
        }

        return RESPAWN_HEALTH;
}

//======================================================================

int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
#ifdef MISSIONPACK/*
        int             upperBound;

        other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;

        if( other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
                upperBound = STAT_MAX_HEALTH;
        }
        else {
                upperBound = STAT_MAX_HEALTH * 2;
        }

        if ( other->client->ps.stats[STAT_ARMOR] > upperBound ) {
                other->client->ps.stats[STAT_ARMOR] = upperBound;
        } */
#else
//        other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
 //       if ( other->client->ps.stats[STAT_ARMOR] > STAT_MAX_HEALTH * 2 ) {
 //               other->client->ps.stats[STAT_ARMOR] = STAT_MAX_HEALTH * 2;
 //       }
#endif

        return RESPAWN_ARMOR;
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
        // randomly select from teamed entities
        if (ent->team) {
                gentity_t       *master;
                int     count;
                int choice;

                if ( !ent->teammaster ) {
                        G_Error( "RespawnItem: bad teammaster");
                }
                master = ent->teammaster;

                for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
                        ;

                choice = rand() % count;

                for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++)
                        ;
        }

        ent->r.contents = CONTENTS_TRIGGER;
        ent->s.eFlags &= ~EF_NODRAW;
        ent->r.svFlags &= ~SVF_NOCLIENT;
        trap_LinkEntity (ent);

        if ( ent->item->giType == IT_POWERUP ) {
                // play powerup spawn sound to all clients
                gentity_t       *te;

                // if the powerup respawn sound should Not be global
                if (ent->speed) {
                        te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
                }
                else {
                        te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
                }
                te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
                te->r.svFlags |= SVF_BROADCAST;
        }


        // play the normal respawn sound only to nearby clients
        G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

        ent->nextthink = 0;
}


/*
===============
Touch_Item
===============
*/
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace) {
        int                     respawn;
        qboolean        predict;

        if (!other->client)
                return;
        if (other->health < 1)
                return;         // dead people can't pickup

        // the same pickup rules are used for client side and server side
        if ( !BG_CanItemBeGrabbed( g_gametype.integer, &ent->s, &other->client->ps ) ) {
                return;
        }

        G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

        predict = other->client->pers.predictItemPickup;

        // call the item-specific pickup function
        switch( ent->item->giType ) {
        case IT_WEAPON:
              respawn = Pickup_Weapon(ent, other);
              predict = qfalse;
                break;
        case IT_AMMO:
                respawn = Pickup_Ammo(ent, other);
              predict = qfalse;
                break;
        case IT_ARMOR:
                respawn = Pickup_Armor(ent, other);
                break;
        case IT_HEALTH:
                respawn = Pickup_Health(ent, other);
                break;
        case IT_POWERUP:
                respawn = Pickup_Powerup(ent, other);
                predict = qfalse;
                break;
#ifdef MISSIONPACK
        case IT_PERSISTANT_POWERUP:
                respawn = Pickup_PersistantPowerup(ent, other);
                break;
#endif
        case IT_TEAM:
                respawn = Pickup_Team(ent, other);
                break;
        case IT_HOLDABLE:
                respawn = Pickup_Holdable(ent, other);
                break;
        default:
                return;
        }

        if ( !respawn ) {
                return;
        }

        // play the normal pickup sound

        if ((other->client->respawnTime +1000 <= level.time) || (g_gametype.integer == GT_FFA)){
        if (predict) {
                G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
        } else {
                G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
        }
        }
/*
        // powerup pickups are global broadcasts
        if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM) {
                // if we want the global sound to play
                if (!ent->speed) {
                        gentity_t       *te;

                        te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
                        te->s.eventParm = ent->s.modelindex;
                        te->r.svFlags |= SVF_BROADCAST;
                } else {
                        gentity_t       *te;

                        te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
                        te->s.eventParm = ent->s.modelindex;
                        // only send this temp entity to a single client
                        te->r.svFlags |= SVF_SINGLECLIENT;
                        te->r.singleClient = other->s.number;
                }
        }*/

        // fire item targets
        G_UseTargets (ent, other);

        // wait of -1 will not respawn
        if ( ent->wait == -1 ) {
                ent->r.svFlags |= SVF_NOCLIENT;
                ent->s.eFlags |= EF_NODRAW;
                ent->r.contents = 0;
                ent->unlinkAfterEvent = qtrue;
                return;
        }

        // non zero wait overrides respawn time
        if ( ent->wait ) {
                respawn = ent->wait;
        }

        // random can be used to vary the respawn time
        if ( ent->random ) {
                respawn += crandom() * ent->random;
                if ( respawn < 1 ) {
                        respawn = 1;
                }
        }

        // dropped items will not respawn
        if ( ent->flags & FL_DROPPED_ITEM ) {
                ent->freeAfterEvent = qtrue;
        }

        // picked up items still stay around, they just don't
        // draw anything.  This allows respawnable items
        // to be placed on movers.
        ent->r.svFlags |= SVF_NOCLIENT;
        ent->s.eFlags |= EF_NODRAW;
        ent->r.contents = 0;

        // ZOID
        // A negative respawn times means to never respawn this item (but don't
        // delete it).  This is used by items that are respawned by third party
        // events such as ctf flags
        if ( respawn <= 0 ) {
                ent->nextthink = 0;
                ent->think = 0;
        } else {
                ent->nextthink = level.time + respawn * 1000;
                ent->think = RespawnItem;
        }
        trap_LinkEntity( ent );
}


/*
================
LaunchPowerup

spawns a weapon and tosses it forward . adding clips
================
*/
gentity_t *LaunchPowerup( gitem_t *item, vec3_t origin, vec3_t velocity, int x, int y ) {
  gentity_t   *dropped;



  dropped = G_Spawn();

  dropped->s.eType = ET_ITEM;
  dropped->s.modelindex = item - bg_itemlist; // store item number in modelindex
  dropped->s.modelindex2 = 1;                 // This is non-zero is it's a dropped item

  dropped->classname = item->classname;
  dropped->item = item;
  VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -3);
  VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 3);
  dropped->r.contents = CONTENTS_TRIGGER;


  origin[0] += 10;
  origin[1] += 10;
 // G_Printf("origin = 0:%f, 1:%f,2:%f\n", origin[0], origin[1],  origin[2] );
  G_SetOrigin( dropped, origin );

  dropped->s.groundEntityNum = -1;
  dropped->s.pos.trType = TR_GRAVITY;
  dropped->s.pos.trTime = level.time;
  VectorCopy( velocity, dropped->s.pos.trDelta );

  dropped->s.eFlags |= EF_BOUNCE_HALF;

  dropped->flags = FL_DROPPED_ITEM;

  dropped->touch = Touch_Item;


//  dropped->s.apos.trBase[YAW] = rand() % 360;
//  dropped->s.apos.trBase[ROLL] = 90;
    // clips
  dropped->count = 10000;
  dropped->think = G_FreeEntity;
  dropped->nextthink = level.time + 2000; // remove after 20 seconds
  trap_LinkEntity (dropped);
 // G_Printf("trBase[YAW] = %i, trBase[ROLL] = %i\n",(int)dropped->s.apos.trBase[YAW], (int)dropped->s.apos.trBase[ROLL] );
  return dropped;
}

/*
================
SpawnPowerup

================
*/
gentity_t *SpawnPowerup( gentity_t *ent, gitem_t *item, float angle, int clips, int x, int y ) {
  vec3_t      velocity,forward;
  vec3_t      angles;
  vec3_t      origin;

  VectorCopy( ent->s.apos.trBase, angles );
  angles[YAW] += angle;
  angles[PITCH] = 0;  // always forward

  AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
  VectorScale( forward, 250, velocity );
  velocity[2] += 200;

  VectorCopy(ent->client->ps.origin,origin );
  origin[2] += 10;


    // add this
  return LaunchPowerup( item, origin, velocity, x, y  );
}


/*
================
UT_SpawnPowerup

================
*/
void UT_SpawnPowerup ( gentity_t *ent, int i)
{
    

  gclient_t   *client;
  gitem_t             *item;

  client = ent->client;

  if ( ent->client->ps.pm_flags & PMF_FOLLOW )
    return;
    
  item = BG_FindItemForPowerup( i );

  SpawnPowerup( ent, item, 90, 1, 10, 10 );



}

/*
================
LaunchWeapon

spawns a weapon and tosses it forward . adding clips
================
*/
gentity_t *LaunchWeapon( gitem_t *item, vec3_t origin, vec3_t velocity, int clips, int x, int y ) {
  gentity_t   *dropped;



  dropped = G_Spawn();

  dropped->s.eType = ET_ITEM;
  dropped->s.modelindex = item - bg_itemlist; // store item number in modelindex
  dropped->s.modelindex2 = 1;                 // This is non-zero is it's a dropped item

  dropped->classname = item->classname;
  dropped->item = item;
  VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -3);
  VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 3);
  dropped->r.contents = CONTENTS_TRIGGER;


  origin[0] += x;
  origin[1] += y;
 // G_Printf("origin = 0:%f, 1:%f,2:%f\n", origin[0], origin[1],  origin[2] );
  G_SetOrigin( dropped, origin );

  dropped->s.groundEntityNum = -1;
  dropped->s.pos.trType = TR_GRAVITY;
  dropped->s.pos.trTime = level.time;
  VectorCopy( velocity, dropped->s.pos.trDelta );

  dropped->s.eFlags |= EF_BOUNCE_HALF;

  dropped->flags = FL_DROPPED_ITEM;

  dropped->touch = Touch_Item;


  dropped->s.apos.trBase[YAW] = rand() % 360;
  dropped->s.apos.trBase[ROLL] = 90;
    // clips
  dropped->count = clips;
  dropped->think = G_FreeEntity;
  dropped->nextthink = level.time + 20000; // remove after 20 seconds
  trap_LinkEntity (dropped);
  //G_Printf("trBase[YAW] = %i, trBase[ROLL] = %i\n",(int)dropped->s.apos.trBase[YAW], (int)dropped->s.apos.trBase[ROLL] );
  return dropped;
}


/*
================
LaunchItem

================
*/
gentity_t *LaunchItem2( gitem_t *item, vec3_t origin, vec3_t velocity  ) {
  gentity_t   *dropped;



  dropped = G_Spawn();

  dropped->s.eType = ET_ITEM;
  dropped->s.modelindex = item - bg_itemlist; // store item number in modelindex
  dropped->s.modelindex2 = 1;                 // This is non-zero is it's a dropped item

  dropped->classname = item->classname;
  dropped->item = item;
  VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -3);
  VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 3);
  dropped->r.contents = CONTENTS_TRIGGER;


 // G_Printf("origin = 0:%f, 1:%f,2:%f\n", origin[0], origin[1],  origin[2] );
  G_SetOrigin( dropped, origin );

  dropped->s.groundEntityNum = -1;
  dropped->s.pos.trType = TR_GRAVITY;
  dropped->s.pos.trTime = level.time;
  VectorCopy( velocity, dropped->s.pos.trDelta );

  dropped->s.eFlags |= EF_BOUNCE_HALF;

  dropped->flags = FL_DROPPED_ITEM;

  dropped->touch = Touch_Item;


  dropped->s.apos.trBase[YAW] = rand() % 360;

  dropped->count = 10000000;
  dropped->think = G_FreeEntity;
  dropped->nextthink = level.time + 20000; // remove after 20 seconds
  trap_LinkEntity (dropped);
  //G_Printf("trBase[YAW] = %i, trBase[ROLL] = %i\n",(int)dropped->s.apos.trBase[YAW], (int)dropped->s.apos.trBase[ROLL] );
  return dropped;
}

/*
================
Drop_Weapon

Drops weapons, and adds the current clips to the spawned entity...
================
*/
gentity_t *Drop_Weapon( gentity_t *ent, gitem_t *item, float angle, int clips, int x, int y ) {
  vec3_t      velocity,forward;
  vec3_t      angles;
  vec3_t      origin;

  
  
      if ( ent->client->ps.pm_flags & PMF_FOLLOW )
        return NULL;

    if ( ent->client->ps.pm_flags & EF_WEAPONS_LOCKED)
        return NULL;
  
  
  VectorCopy( ent->s.apos.trBase, angles );
  angles[YAW] += angle;
  angles[PITCH] = 0;  // always forward

  AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
  VectorScale( forward, 250, velocity );
  velocity[2] += 220;

  VectorCopy(ent->client->ps.origin,origin );
  origin[2] += 13;


    // add this
  return LaunchWeapon( item, origin, velocity, clips, x, y  );
}



gentity_t *TossItem( gentity_t *ent, gitem_t *item ) {
  vec3_t      velocity,forward;
  vec3_t      angles;
  vec3_t      origin;

  
  
      if ( ent->client->ps.pm_flags & PMF_FOLLOW )
        return NULL;

    if ( ent->client->ps.pm_flags & EF_WEAPONS_LOCKED)
        return NULL;
  
  
  VectorCopy( ent->s.apos.trBase, angles );
  angles[YAW] = 180;
  angles[PITCH] = 0;  // always forward

  AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
  VectorScale( forward, 320, velocity );
  velocity[2] += 80;

  VectorCopy(ent->client->ps.origin,origin );
  origin[2] +=23;
  

    // add this
  ent->client->ps.weaponTime = 250;
  return LaunchItem2( item, origin, velocity  );
}


void UT_DropItem ( gentity_t *ent)
{
   int         pw,i;
  char        msg[64];
  gclient_t   *client;
  gitem_t             *item;

  client = ent->client;
  
  if ( ent->client->ps.pm_flags & PMF_FOLLOW )
    return;

  if (client->ps.weaponstate != WEAPON_READY)
    return;
  
   trap_Argv( 1, msg, sizeof( msg ) );
   
   
   
  pw = atoi(msg);
  

          if(ent->client->ps.powerups[pw]){
                item = BG_FindItemForPowerup( pw );
                TossItem( ent, item );
                ent->client->ps.powerups[pw]=0;
          for(i=0; i < 3; i++ )
          if (bg_inventory.item[ent->client->ps.clientNum][i] == pw ){
          bg_inventory.item[ent->client->ps.clientNum][i] = 0;
          }
              
      }
  
}
/*
================
Drop_Weapon

Drops current Weapon
================
*/
void UT_DropWeapon ( gentity_t *ent)
{
  gclient_t   *client;
  gitem_t             *item;
  int ammo;
  int weapon;

  client = ent->client;
  weapon = client->ps.weapon;
  ammo = bg_weaponlist[weapon].rounds[client->ps.clientNum]
      + bg_weaponlist[weapon].numClips[client->ps.clientNum]
      * RoundCount(weapon);

  if ( ent->client->ps.pm_flags & PMF_FOLLOW )
    return;

  if (client->ps.weaponstate != WEAPON_READY)
    return;

  if (ent->s.weapon <= WP_KNIFE)
  {
    return;
  }
  item = BG_FindItemForWeapon( client->ps.weapon );
  
   BG_RemoveWeapon( weapon, ent->client->ps.stats );

  if ( BG_Grenade( weapon ) && bg_weaponlist[weapon].numClips[client->ps.clientNum] > 1 )
  {
    bg_weaponlist[weapon].numClips[client->ps.clientNum]--;
  }
  else
  {

    if ( BG_Grenade( weapon ) )
      bg_weaponlist[weapon].numClips[client->ps.clientNum] = 0;
  }
  //client->ps.weapon = WP_NONE;

  if ( weapon == bg_inventory.sort[client->ps.clientNum][PRIMARY])
    bg_inventory.sort[client->ps.clientNum][PRIMARY] = WP_NONE;
  if ( weapon == bg_inventory.sort[client->ps.clientNum][SECONDARY]){
	if( BG_Secondary( bg_inventory.sort[client->ps.clientNum][PRIMARY] )){
		bg_inventory.sort[client->ps.clientNum][SECONDARY] =  bg_inventory.sort[client->ps.clientNum][PRIMARY];
		bg_inventory.sort[client->ps.clientNum][PRIMARY] = WP_NONE;
	}else
    		bg_inventory.sort[client->ps.clientNum][SECONDARY] = WP_NONE;
	}
  if ( weapon == bg_inventory.sort[client->ps.clientNum][SIDEARM])
    bg_inventory.sort[client->ps.clientNum][SIDEARM] = WP_NONE;
  if ( weapon == bg_inventory.sort[client->ps.clientNum][NADE])
    bg_inventory.sort[client->ps.clientNum][NADE] = WP_NONE;

  if ( BG_Grenade( weapon ) )
  Drop_Weapon( ent, item, random()*360, bg_weaponlist[weapon].numClips[client->ps.clientNum], 0, 0 );
  else {
  Drop_Weapon( ent, item, random()*360, ammo, 0, 0 );
  }
  bg_weaponlist[weapon].rounds[client->ps.clientNum] = 0;
  bg_weaponlist[weapon].numClips[client->ps.clientNum] = 0;

  for ( ; ; ){
    if ( bg_inventory.sort[client->ps.clientNum][PRIMARY] ){
      client->ps.weapon = bg_inventory.sort[client->ps.clientNum][PRIMARY];
    break;
  }
  else if ( bg_inventory.sort[client->ps.clientNum][SECONDARY]){
    client->ps.weapon = bg_inventory.sort[client->ps.clientNum][SECONDARY];
    break;
  }
  else if ( bg_inventory.sort[client->ps.clientNum][SIDEARM]){
    client->ps.weapon = bg_inventory.sort[client->ps.clientNum][SIDEARM];
  break;
  }
  else if ( bg_inventory.sort[client->ps.clientNum][NADE]){
    client->ps.weapon = bg_inventory.sort[client->ps.clientNum][NADE];
    break;
  }
  else{
    client->ps.weapon = bg_inventory.sort[client->ps.clientNum][MELEE];
  break;
  }
  }
 
  
  //trap_SendConsoleCommand(client->ps.clientNum, va("weapon %i \n", client->ps.weapon ));
  ent->client->ps.weaponstate = WEAPON_READY;


  G_AddEvent(ent, EV_WEAPON_DROPPED, 0);
 // UT_SpawnPowerup ( ent, PW_SILENCER);
  //SpawnSilencer(ent );
}

//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
        gentity_t       *dropped;

        dropped = G_Spawn();

        dropped->s.eType = ET_ITEM;
        dropped->s.modelindex = item - bg_itemlist;     // store item number in modelindex
        dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

        dropped->classname = item->classname;
        dropped->item = item;
        VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
        VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
        dropped->r.contents = CONTENTS_TRIGGER;

        dropped->touch = Touch_Item;

        G_SetOrigin( dropped, origin );


        dropped->s.pos.trType = TR_GRAVITY;
        dropped->s.pos.trTime = level.time;
        VectorCopy( velocity, dropped->s.pos.trDelta );


      //  dropped->s.eFlags |= EF_BOUNCE_HALF;
#ifdef MISSIONPACK
        if ((g_gametype.integer == GT_CTF || g_gametype.integer == GT_BOMB)                     && item->giType == IT_TEAM) { // Special case for CTF flags
#else
        if (g_gametype.integer == GT_CTF && item->giType == IT_TEAM) { // Special case for CTF flags
#endif
                dropped->think = Team_DroppedFlagThink;
                dropped->nextthink = level.time + 30000;
                Team_CheckDroppedItem( dropped );
        } else { // auto-remove after 30 seconds
                dropped->think = G_FreeEntity;
                dropped->nextthink = level.time + 30000;
        }

        dropped->flags = FL_DROPPED_ITEM;
        
	if(  FL_THROWN_ITEM) {
		dropped->clipmask = MASK_SHOT;
		dropped->s.pos.trTime = level.time - 50;	// move a bit on the very first frame
		VectorScale( velocity, 60, dropped->s.pos.trDelta ); // 700
		SnapVector( dropped->s.pos.trDelta );		// save net bandwidth
		dropped->physicsBounce= 0.2f;
	}

        trap_LinkEntity (dropped);

        return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle ) {
        vec3_t  velocity;
        vec3_t  angles;
        //int i;

        VectorCopy( ent->s.apos.trBase, angles );
        angles[YAW] += angle;
        angles[PITCH] = 0;      // always forward

        AngleVectors( angles, velocity, NULL, NULL );
        VectorScale( velocity, 4, velocity );
        velocity[2] += 3 + crandom() * 2;

      /*  for(i=0; i < 3; i++ )
          if (bg_inventory.item[ent->client->ps.clientNum][i] = item->giTag && item->giType == IT_POWERUP){
          G_Printf(" bg_inventory.item[ent->client->ps.clientNum][%i] = 0\n", i);
          bg_inventory.item[ent->client->ps.clientNum][i] = 0;

          }
        */

        return LaunchItem( item, ent->s.pos.trBase, velocity );
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
        RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
        trace_t         tr;
        vec3_t          dest;


        VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
        VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

        ent->s.eType = ET_ITEM;
        ent->s.modelindex = ent->item - bg_itemlist;            // store item number in modelindex
        ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

        ent->r.contents = CONTENTS_TRIGGER;
        ent->touch = Touch_Item;
        // useing an item causes it to respawn
        ent->use = Use_Item;

        if ( 0/*ent->spawnflags & 1*/ ) {
                // suspended
                G_SetOrigin( ent, ent->s.origin );
        } else {
                // drop to floor
                VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
                trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
                if ( tr.startsolid ) {
                        G_Printf ("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
                        G_FreeEntity( ent );
                        return;
                }

                // allow to ride movers
                ent->s.groundEntityNum = tr.entityNum;

                G_SetOrigin( ent, tr.endpos );
        }
        G_Printf ("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
        // team slaves and targeted items aren't present at start
        if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
                ent->s.eFlags |= EF_NODRAW;
                ent->r.contents = 0;
                return;
        }

        // powerups don't spawn in for a while
        if ( ent->item->giType == IT_POWERUP ) {
                float   respawn;

                respawn = 10;
                ent->s.eFlags |= EF_NODRAW;
                ent->r.contents = 0;
                ent->nextthink = level.time + respawn;
                ent->think = RespawnItem;
                return;
        }


        trap_LinkEntity (ent);
}


qboolean        itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {

        // Set up team stuff
        Team_InitGame();

        if( g_gametype.integer == GT_CTF ) {
                gitem_t *item;

                // check for the two flags
                item = BG_FindItem( "Red Flag" );
                if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map" );
                }
                item = BG_FindItem( "Blue Flag" );
                if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map" );
                }
        }
#ifdef MISSIONPACK
        if( g_gametype.integer == GT_BOMB ) {
                gitem_t *item;

                // check for all three flags
                item = BG_FindItem( "Red Flag" );
                if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map" );
                }
                item = BG_FindItem( "Blue Flag" );
                if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map" );
                }
                item = BG_FindItem( "Neutral Flag" );
                if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_neutralflag in map" );
                }
        }

        if( g_gametype.integer == GT_BOMB ) {
                gentity_t       *ent;

                // check for the two obelisks
                ent = NULL;
                ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
                if( !ent ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map" );
                }

                ent = NULL;
                ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
                if( !ent ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map" );
                }
        }

        if( g_gametype.integer == GT_BOMB ) {
                gentity_t       *ent;

                // check for all three obelisks
                ent = NULL;
                ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
                if( !ent ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map" );
                }

                ent = NULL;
                ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
                if( !ent ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map" );
                }

                ent = NULL;
                ent = G_Find( ent, FOFS(classname), "team_neutralobelisk" );
                if( !ent ) {
                        G_Printf( S_COLOR_YELLOW "WARNING: No team_neutralobelisk in map" );
                }
        }
#endif
}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
    int i;
        memset( itemRegistered, 0, sizeof( itemRegistered ) );

        // players always start with the base weapon
        RegisterItem( BG_FindItemForWeapon( WP_KNIFE ) );
        
         for( i = PW_NEUTRALFLAG + 1; i < PW_NUM_POWERUPS -1; i++ ){
            RegisterItem(BG_FindItemForPowerup( i ));
        }
        
         for( i = WP_NONE + 1; i < WP_NUM_WEAPONS -1; i++ ){
            RegisterItem(BG_FindItemForWeapon( i ));
        }
        
        
#ifdef MISSIONPACK
        if( g_gametype.integer == GT_BOMB ) {
                RegisterItem( BG_FindItem( "Red Cube" ) );
                RegisterItem( BG_FindItem( "Blue Cube" ) );
        }
#endif
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
        if ( !item ) {
                G_Error( "RegisterItem: NULL" );
        }
        itemRegistered[ item - bg_itemlist ] = qtrue;
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
        char    string[MAX_ITEMS+1];
        int             i;
        int             count;

        count = 0;
        for ( i = 0 ; i < bg_numItems ; i++ ) {
                if ( itemRegistered[i] ) {
                        count++;
                        string[i] = '1';
                } else {
                        string[i] = '0';
                }
        }
        string[ bg_numItems ] = 0;

        G_Printf( "%i items registered\n", count );
        trap_SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled( gitem_t *item ) {

        char name[128];

        Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
        return trap_Cvar_VariableIntegerValue( name );
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem (gentity_t *ent, gitem_t *item) {
        G_SpawnFloat( "random", "0", &ent->random );
        G_SpawnFloat( "wait", "0", &ent->wait );

        RegisterItem( item );
        if ( G_ItemDisabled(item) )
                return;

        ent->item = item;
        // some movers spawn on the second frame, so delay item
        // spawns until the third frame so they can ride trains
        ent->nextthink = level.time + FRAMETIME * 2;
        ent->think = FinishSpawningItem;

        ent->physicsBounce = 0.0;              // items are bouncy

        if ( item->giType == IT_POWERUP ) {
                G_SoundIndex( "sound/items/poweruprespawn.wav" );
                G_SpawnFloat( "noglobalsound", "0", &ent->speed);
        }

#ifdef MISSIONPACK
        if ( item->giType == IT_PERSISTANT_POWERUP ) {
                ent->s.generic1 = ent->spawnflags;
        }
#endif
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
        vec3_t  velocity;
        float   dot;
        int             hitTime;

        // reflect the velocity on the trace plane
        hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
        BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
        dot = DotProduct( velocity, trace->plane.normal );
        VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

        // cut the velocity to keep from bouncing forever
        VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

        // check for stop
        if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
                trace->endpos[2] += 1.0;        // make sure it is off ground
                SnapVector( trace->endpos );
                G_SetOrigin( ent, trace->endpos );
                ent->s.groundEntityNum = trace->entityNum;
                return;
        }

        VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
        VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
        ent->s.pos.trTime = level.time;
}


/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
        vec3_t          origin;
        trace_t         tr;
        int                     contents;
        int                     mask;

        // if groundentity has been set to -1, it may have been pushed off an edge
        if ( ent->s.groundEntityNum == -1 ) {
                if ( ent->s.pos.trType != TR_GRAVITY ) {
                        ent->s.pos.trType = TR_GRAVITY;
                        ent->s.pos.trTime = level.time;
                }
        }

        if ( ent->s.pos.trType == TR_STATIONARY ) {
                // check think function
                G_RunThink( ent );
                return;
        }

        // get current position
        BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

        // trace a line from the previous position to the current position
        if ( ent->clipmask ) {
                mask = ent->clipmask;
        } else {
                mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;//MASK_SOLID;
        }
        trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
                ent->r.ownerNum, mask );

        VectorCopy( tr.endpos, ent->r.currentOrigin );

        if ( tr.startsolid ) {
                tr.fraction = 0;
        }

        trap_LinkEntity( ent ); // FIXME: avoid this for stationary?

        // check think function
        G_RunThink( ent );

        if ( tr.fraction == 1 ) {
                return;
        }

        // if it is in a nodrop volume, remove it
        contents = trap_PointContents( ent->r.currentOrigin, -1 );
        if ( contents & CONTENTS_NODROP ) {
                if (ent->item && ent->item->giType == IT_TEAM) {
                        Team_FreeEntity(ent);
                } else {
                        G_FreeEntity( ent );
                }
                return;
        }

        G_BounceItem( ent, &tr );
}


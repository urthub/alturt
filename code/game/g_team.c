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

int lastbombclient[TEAM_NUM_TEAMS];
int     lastvip[TEAM_NUM_TEAMS];

typedef struct teamgame_s {
	float			last_flag_capture;
	int				last_capture_team;
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int				redTakenTime;
	int				blueTakenTime;
	int				redObeliskAttackedTime;
	int				blueObeliskAttackedTime;
} teamgame_t;

teamgame_t teamgame;

gentity_t	*neutralObelisk;

void Team_SetFlagStatus( int team, flagStatus_t status );

void Team_InitGame( void ) {
	memset(&teamgame, 0, sizeof teamgame);

	switch( g_gametype.integer ) {
	case GT_CTF:
		teamgame.redStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_RED, FLAG_ATBASE );
		 teamgame.blueStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_BLUE, FLAG_ATBASE );
		break;
#ifdef MISSIONPACK
	case GT_BOMB:
		teamgame.flagStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_FREE, FLAG_ATBASE );
		break;
#endif
	default:
		break;
	}
}

int OtherTeam(int team) {
	if (team==TEAM_RED)
		return TEAM_BLUE;
	else if (team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *TeamName(int team)  {
	if (team==TEAM_RED_SPECTATOR)
		return "red";
	else if (team==TEAM_BLUE_SPECTATOR)
		return "blue";
	else if (team==TEAM_RED)
		return "red";
	else if (team==TEAM_BLUE)
		return "blue";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *OtherTeamName(int team) {
	if (team==TEAM_RED_SPECTATOR)
		return "red";
	else if (team==TEAM_BLUE_SPECTATOR)
		return "blue";
	else if (team==TEAM_RED)
		return "red";
	else if (team==TEAM_BLUE)
		return "blue";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *TeamColorString(int team) {
	if (team==TEAM_RED_SPECTATOR)
		return S_COLOR_RED;
	else if (team==TEAM_BLUE_SPECTATOR)
		return S_COLOR_BLUE;
	else if (team==TEAM_RED)
		return S_COLOR_RED;
	else if (team==TEAM_BLUE)
		return S_COLOR_BLUE;
	else if (team==TEAM_SPECTATOR)
		return S_COLOR_YELLOW;
	return S_COLOR_WHITE;
}

// NULL for everyone
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... ) {
	char		msg[1024];
	va_list		argptr;
	char		*p;

	va_start (argptr,fmt);
	if (Q_vsnprintf (msg, sizeof(msg), fmt, argptr) > sizeof(msg)) {
		G_Error ( "PrintMsg overrun" );
	}
	va_end (argptr);

	// double quotes are bad
	while ((p = strchr(msg, '"')) != NULL)
		*p = '\'';

	trap_SendServerCommand ( ( (ent == NULL) ? -1 : ent-g_entities ), va("print \"%s\"", msg ));
}

/*
==============
AddTeamScore

 used for gametype > GT_TEAM
 for gametype GT_TEAM the level.teamScores is updated in AddScore in g_combat.c
==============
*/
void AddTeamScore(vec3_t origin, int team, int score) {
	gentity_t	*te;

	te = G_TempEntity(origin, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;

	if ( team == TEAM_RED ) {
		if ( level.teamScores[ TEAM_RED ] + score == level.teamScores[ TEAM_BLUE ] ) {
			//teams are tied sound
			te->s.eventParm = GTS_TEAMS_ARE_TIED;
		}
		else if ( level.teamScores[ TEAM_RED ] <= level.teamScores[ TEAM_BLUE ] &&
					level.teamScores[ TEAM_RED ] + score > level.teamScores[ TEAM_BLUE ]) {
			// red took the lead sound
			te->s.eventParm = GTS_REDTEAM_TOOK_LEAD;
		}
		else {
			// red scored sound
			te->s.eventParm = GTS_REDTEAM_SCORED;
		}
	}
	else {
		if ( level.teamScores[ TEAM_BLUE ] + score == level.teamScores[ TEAM_RED ] ) {
			//teams are tied sound
			te->s.eventParm = GTS_TEAMS_ARE_TIED;
		}
		else if ( level.teamScores[ TEAM_BLUE ] <= level.teamScores[ TEAM_RED ] &&
					level.teamScores[ TEAM_BLUE ] + score > level.teamScores[ TEAM_RED ]) {
			// blue took the lead sound
			te->s.eventParm = GTS_BLUETEAM_TOOK_LEAD;
		}
		else {
			// blue scored sound
			te->s.eventParm = GTS_BLUETEAM_SCORED;
		}
	}
	level.teamScores[ team ] += score;
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 ) {
	if ( !ent1->client || !ent2->client ) {
		return qfalse;
	}

	if ( g_gametype.integer < GT_TEAM ) {
		return qfalse;
	}

	if ( ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam ) {
		return qtrue;
	}

	return qfalse;
}


static char ctfFlagStatusRemap[] = { '0', '1', '*', '*', '2' };
static char oneFlagStatusRemap[] = { '0', '1', '2', '3', '4' };

void Team_SetFlagStatus( int team, flagStatus_t status ) {
	qboolean modified = qfalse;

	switch( team ) {
	case TEAM_RED:	// CTF
		if( teamgame.redStatus != status ) {
			teamgame.redStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_BLUE:	// CTF
		if( teamgame.blueStatus != status ) {
			teamgame.blueStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_FREE:	// One Flag CTF
		if( teamgame.flagStatus != status ) {
			teamgame.flagStatus = status;
			modified = qtrue;
		}
		break;
	}

	if( modified ) {
		char st[4];

		if( g_gametype.integer == GT_CTF ) {
			st[0] = ctfFlagStatusRemap[teamgame.redStatus];
			st[1] = ctfFlagStatusRemap[teamgame.blueStatus];
			st[2] = 0;
		}
		else {		// GT_BOMB
			st[0] = oneFlagStatusRemap[teamgame.flagStatus];
			st[1] = 0;
		}

		trap_SetConfigstring( CS_FLAGSTATUS, st );
	}
}

void Team_CheckDroppedItem( gentity_t *dropped ) {
	if( dropped->item->giTag == PW_REDFLAG ) {
		Team_SetFlagStatus( TEAM_RED, FLAG_DROPPED );
	}
	else if( dropped->item->giTag == PW_BLUEFLAG ) {
		Team_SetFlagStatus( TEAM_BLUE, FLAG_DROPPED );
	}
	else if( dropped->item->giTag == PW_NEUTRALFLAG ) {
		Team_SetFlagStatus( TEAM_FREE, FLAG_DROPPED );
	}
}

/*
================
Team_ForceGesture
================
*/
void Team_ForceGesture(int team) {
	int i;
	gentity_t *ent;

	for (i = 0; i < MAX_CLIENTS; i++) {
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (ent->client->sess.sessionTeam != team)
			continue;
		//
		ent->flags |= FL_FORCE_GESTURE;
	}
}

/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker)
{
	int i;
	gentity_t *ent;
	int flag_pw, enemy_flag_pw;
	int otherteam;
	int tokens;
	gentity_t *flag, *carrier = NULL;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself or team mates
	if (!targ->client || !attacker->client || targ == attacker || OnSameTeam(targ, attacker))
		return;

	team = targ->client->sess.sessionTeam;
	otherteam = OtherTeam(targ->client->sess.sessionTeam);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM_RED) {
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

	//if (g_gametype.integer == GT_BOMB) {
	//	enemy_flag_pw = PW_NEUTRALFLAG;
//	}

	// did the attacker frag the flag carrier?
	tokens = 0;
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_BOMB ) {
		tokens = targ->client->ps.generic1;
	}
#endif
	if (targ->client->ps.powerups[enemy_flag_pw]) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
			attacker->client->pers.netname, TeamName(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	// did the attacker frag a head carrier? other->client->ps.generic1
	if (tokens) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS * tokens * tokens);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's skull carrier!\n",
			attacker->client->pers.netname, TeamName(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->ps.powerups[flag_pw]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		team = attacker->client->sess.sessionTeam;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~( EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT) {
		// attacker is on the same team as the skull carrier and
		AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		team = attacker->client->sess.sessionTeam;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_BOMB ) {
		// find the team obelisk
		switch (attacker->client->sess.sessionTeam) {
		case TEAM_RED:
			c = "team_redobelisk";
			break;
		case TEAM_BLUE:
			c = "team_blueobelisk";
			break;
		default:
			return;
		}

	} else if (g_gametype.integer == GT_BOMB ) {
		// find the center obelisk
		c = "team_neutralobelisk";
	} else {
#endif
	// find the flag
	switch (attacker->client->sess.sessionTeam) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return;
	}
	// find attacker's team's flag carrier
	for (i = 0; i < g_maxclients.integer; i++) {
		carrier = g_entities + i;
		if (carrier->inuse && carrier->client->ps.powerups[flag_pw])
			break;
		carrier = NULL;
	}
#ifdef MISSIONPACK
	}
#endif
	flag = NULL;
	while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
		if (!(flag->flags & FL_DROPPED_ITEM))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract(targ->r.currentOrigin, flag->r.currentOrigin, v1);
	VectorSubtract(attacker->r.currentOrigin, flag->r.currentOrigin, v2);

	if ( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, targ->r.currentOrigin ) ) ||
		( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
		attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {

		// we defended the base flag
		AddScore(attacker, targ->r.currentOrigin, CTF_FLAG_DEFENSE_BONUS);
		attacker->client->pers.teamState.basedefense++;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if (carrier && carrier != attacker) {
		VectorSubtract(targ->r.currentOrigin, carrier->r.currentOrigin, v1);
		VectorSubtract(attacker->r.currentOrigin, carrier->r.currentOrigin, v1);

		if ( ( ( VectorLength(v1) < CTF_ATTACKER_PROTECT_RADIUS &&
			trap_InPVS(carrier->r.currentOrigin, targ->r.currentOrigin ) ) ||
			( VectorLength(v2) < CTF_ATTACKER_PROTECT_RADIUS &&
				trap_InPVS(carrier->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
			attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
			AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_PROTECT_BONUS);
			attacker->client->pers.teamState.carrierdefense++;

			attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
			// add the sprite over the player's head
			attacker->client->ps.eFlags &= ~( EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
			attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
			attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

			return;
		}
	}
}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier(gentity_t *targ, gentity_t *attacker)
{
	int flag_pw;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->sess.sessionTeam == TEAM_RED)
		flag_pw = PW_BLUEFLAG;
	else
		flag_pw = PW_REDFLAG;

	// flags
	if (targ->client->ps.powerups[flag_pw] &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;

	// skulls
	if (targ->client->ps.generic1 &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;
}


gentity_t *Team_ResetFlag( int team ) {
	char *c;
	gentity_t *ent, *rent = NULL;

	switch (team) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	case TEAM_FREE:
		c = "team_CTF_neutralflag";
		break;
	default:
		return NULL;
	}

	ent = NULL;
	while ((ent = G_Find (ent, FOFS(classname), c)) != NULL) {
		if (ent->flags & FL_DROPPED_ITEM)
			G_FreeEntity(ent);
		else {
			rent = ent;
			RespawnItem(ent);
		}
	}

	Team_SetFlagStatus( team, FLAG_ATBASE );

	return rent;
}

void Team_ResetFlags( void ) {
	if( g_gametype.integer == GT_CTF ) {
		Team_ResetFlag( TEAM_RED );
		Team_ResetFlag( TEAM_BLUE );
	}
#ifdef MISSIONPACK
	else if( g_gametype.integer == GT_BOMB ) {
		Team_ResetFlag( TEAM_FREE );
	}
#endif
}

void Team_ReturnFlagSound( gentity_t *ent, int team ) {
	gentity_t	*te;

	if (ent == NULL) {
		G_Printf ("Warning:  NULL passed to Team_ReturnFlagSound\n");
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_RED_RETURN;
	}
	else {
		te->s.eventParm = GTS_BLUE_RETURN;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_TakeFlagSound( gentity_t *ent, int team ) {
	gentity_t	*te;

	if (ent == NULL) {
		G_Printf ("Warning:  NULL passed to Team_TakeFlagSound\n");
		return;
	}

	// only play sound when the flag was at the base
	// or not picked up the last 10 seconds
	switch(team) {
		case TEAM_RED:
			if( teamgame.blueStatus != FLAG_ATBASE ) {
				if (teamgame.blueTakenTime > level.time - 10000)
					return;
			}
			teamgame.blueTakenTime = level.time;
			break;

		case TEAM_BLUE:	// CTF
			if( teamgame.redStatus != FLAG_ATBASE ) {
				if (teamgame.redTakenTime > level.time - 10000)
					return;
			}
			teamgame.redTakenTime = level.time;
			break;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_RED_TAKEN;
	}
	else {
		te->s.eventParm = GTS_BLUE_TAKEN;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_CaptureFlagSound( gentity_t *ent, int team ) {
	gentity_t	*te;

	if (ent == NULL) {
		G_Printf ("Warning:  NULL passed to Team_CaptureFlagSound\n");
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_BLUE_CAPTURE;
	}
	else {
		te->s.eventParm = GTS_RED_CAPTURE;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_ReturnFlag( int team ) {
	Team_ReturnFlagSound(Team_ResetFlag(team), team);
	if( team == TEAM_FREE ) {
		PrintMsg(NULL, "The flag has returned!\n" );
	}
	else {
		PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team));
	}
}

void Team_FreeEntity( gentity_t *ent ) {
	if( ent->item->giTag == PW_REDFLAG ) {
		Team_ReturnFlag( TEAM_RED );
	}
	else if( ent->item->giTag == PW_BLUEFLAG ) {
		Team_ReturnFlag( TEAM_BLUE );
	}
	else if( ent->item->giTag == PW_NEUTRALFLAG ) {
		Team_ReturnFlag( TEAM_FREE );
	}
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink(gentity_t *ent) {
	int		team = TEAM_FREE;

	if( ent->item->giTag == PW_REDFLAG ) {
		team = TEAM_RED;
	}
	else if( ent->item->giTag == PW_BLUEFLAG ) {
		team = TEAM_BLUE;
	}
	else if( ent->item->giTag == PW_NEUTRALFLAG ) {
		team = TEAM_FREE;
	}

	Team_ReturnFlagSound( Team_ResetFlag( team ), team );
	// Reset Flag will delete this entity
}


/*
==============
Team_DroppedFlagThink
==============
*/
int Team_TouchOurFlag( gentity_t *ent, gentity_t *other, int team ) {
	int			i;
	gentity_t	*player;
	gclient_t	*cl = other->client;
	int			enemy_flag;

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_BOMB ) {
		enemy_flag = PW_NEUTRALFLAG;
	}
	else {
#endif
	if (cl->sess.sessionTeam == TEAM_RED) {
		enemy_flag = PW_BLUEFLAG;
	} else {
		enemy_flag = PW_REDFLAG;
	}

	if ( ent->flags & FL_DROPPED_ITEM ) {
		// hey, its not home.  return it by teleporting it back
		PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n",
			cl->pers.netname, TeamName(team));
		AddScore(other, ent->r.currentOrigin, CTF_RECOVERY_BONUS);
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound(Team_ResetFlag(team), team);
		return 0;
	}
#ifdef MISSIONPACK
	}
#endif

	// the flag is at home base.  if the player has the enemy
	// flag, he's just won!
	if (!cl->ps.powerups[enemy_flag])
		return 0; // We don't have the flag
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_BOMB ) {
		PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the flag!\n", cl->pers.netname );
	}
	else {
#endif
	PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n", cl->pers.netname, TeamName(OtherTeam(team)));
#ifdef MISSIONPACK
	}
#endif

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	// Increase the team's score
	AddTeamScore(ent->s.pos.trBase, other->client->sess.sessionTeam, 1);
	//blud disabling the auto-taunt on capture again (hope it doesn't get removed by svn this time.. cough!
	//Team_ForceGesture(other->client->sess.sessionTeam);

	other->client->pers.teamState.captures++;
	// add the sprite over the player's head
	other->client->ps.eFlags &= ~( EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	other->client->ps.eFlags |= EF_AWARD_CAP;
	other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	other->client->ps.persistant[PERS_CAPTURES]++;

	// other gets another 10 frag bonus
	AddScore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);

	Team_CaptureFlagSound( ent, team );

	// Ok, let's do the player loop, hand out the bonuses
	for (i = 0; i < g_maxclients.integer; i++) {
		player = &g_entities[i];

		// also make sure we don't award assist bonuses to the flag carrier himself.
		if (!player->inuse || player == other)
			continue;

		if (player->client->sess.sessionTeam !=
			cl->sess.sessionTeam) {
			player->client->pers.teamState.lasthurtcarrier = -5;
		} else if (player->client->sess.sessionTeam ==
			cl->sess.sessionTeam) {
			if (player != other)
				AddScore(player, ent->r.currentOrigin, CTF_TEAM_BONUS);
			// award extra points for capture assists
			if (player->client->pers.teamState.lastreturnedflag +
				CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
				AddScore (player, ent->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS);
				other->client->pers.teamState.assists++;

				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				player->client->ps.eFlags &= ~(EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardTime = level.time + REWARD_SPRITE_TIME;

			} else if (player->client->pers.teamState.lastfraggedcarrier +
				CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
				AddScore(player, ent->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS);
				other->client->pers.teamState.assists++;
				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				player->client->ps.eFlags &= ~( EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
		}
	}
	Team_ResetFlags();

	CalculateRanks();

	return 0; // Do not respawn this automatically
}

int Team_TouchEnemyFlag( gentity_t *ent, gentity_t *other, int team ) {
	gclient_t *cl = other->client;

		PrintMsg (NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
			other->client->pers.netname, TeamName(team));

		if (team == TEAM_RED)
			cl->ps.powerups[PW_REDFLAG] = INT_MAX; // flags never expire
		else
			cl->ps.powerups[PW_BLUEFLAG] = INT_MAX; // flags never expire

		Team_SetFlagStatus( team, FLAG_TAKEN );

	AddScore(other, ent->r.currentOrigin, CTF_FLAG_BONUS);
	cl->pers.teamState.flagsince = level.time;
	Team_TakeFlagSound( ent, team );

	return -1; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int Pickup_Team( gentity_t *ent, gentity_t *other ) {
	int team;
	gclient_t *cl = other->client;

	// figure out what team this flag is
	if( strcmp(ent->classname, "team_CTF_redflag") == 0 ) {
		team = TEAM_RED;
	}
	else if( strcmp(ent->classname, "team_CTF_blueflag") == 0 ) {
		team = TEAM_BLUE;
	}
#ifdef MISSIONPACK
	else if( strcmp(ent->classname, "team_CTF_neutralflag") == 0  ) {
		team = TEAM_FREE;
	}
#endif
	else {
		PrintMsg ( other, "Don't know what team the flag is on.\n");
		return 0;
	}
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_BOMB ) {
		if( team == TEAM_FREE ) {
			return Team_TouchEnemyFlag( ent, other, cl->sess.sessionTeam );
		}
		if( team != cl->sess.sessionTeam) {
			return Team_TouchOurFlag( ent, other, cl->sess.sessionTeam );
		}
		return 0;
	}
#endif
	// GT_CTF
	if( team == cl->sess.sessionTeam) {
		return Team_TouchOurFlag( ent, other, team );
	}
	return Team_TouchEnemyFlag( ent, other, team );
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation(gentity_t *ent)
{
	gentity_t		*eloc, *best;
	float			bestlen, len;
	vec3_t			origin;

	best = NULL;
	bestlen = 3*8192.0*8192.0;

	VectorCopy( ent->r.currentOrigin, origin );

	for (eloc = level.locationHead; eloc; eloc = eloc->nextTrain) {
		len = ( origin[0] - eloc->r.currentOrigin[0] ) * ( origin[0] - eloc->r.currentOrigin[0] )
			+ ( origin[1] - eloc->r.currentOrigin[1] ) * ( origin[1] - eloc->r.currentOrigin[1] )
			+ ( origin[2] - eloc->r.currentOrigin[2] ) * ( origin[2] - eloc->r.currentOrigin[2] );

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}


/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg(gentity_t *ent, char *loc, int loclen)
{
	gentity_t *best;

	best = Team_GetLocation( ent );

	if (!best)
		return qfalse;

	if (best->count) {
		if (best->count < 0)
			best->count = 0;
		if (best->count > 7)
			best->count = 7;
		Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else
		Com_sprintf(loc, loclen, "%s", best->message);

	return qtrue;
}


/*---------------------------------------------------------------------------*/

/*
================
SelectRandomTeamSpawnPoint   --Xamis-- Modified for info_ut_spawn

go to a random point that doesn't telefrag
================
*/
#define	MAX_TEAM_SPAWN_POINTS	128
gentity_t *SelectRandomTeamSpawnPoint( int teamstate, team_t utteam ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	//int			i;
	gentity_t	*spots[MAX_TEAM_SPAWN_POINTS];

	//char		*classname;
	char		*teamname;
	char		*class = "info_ut_spawn";

		if (utteam == TEAM_BLUE || utteam == TEAM_BLUE_SPECTATOR ){
			teamname = "blue";
		}
		else if (utteam == TEAM_RED || utteam == TEAM_RED_SPECTATOR){
			teamname = "red";
		}else
			return NULL;

	count = 0;

	spot = NULL;

	while (qtrue) {
		(spot = G_Find (spot, FOFS(team), teamname));// != NULL

                                       if(  strcmp(spot->classname, class))
                                           continue;
                
		if ( SpotWouldTelefrag( spot ) ) {
			//G_Printf( S_COLOR_BLUE "WARNING: spots telefrag\n" );
			continue;
		}

		spots[ count ] = spot;
		//G_Printf( S_COLOR_GREEN "SPOT LOCATED\n" );

		if (++count == MAX_TEAM_SPAWN_POINTS){
			//G_Printf( S_COLOR_BLUE "WARNING: MAX_SPAWN_POINTS\n" );
			break;
		}



	}

	if ( !count ) {	// no spots that won't telefrag
		//G_Printf( S_COLOR_BLUE "WARNING: no spots that won't telefrag\n" );
		//return  G_Find ((spot = G_Find (spot,FOFS(classname), class)), FOFS(team), teamname);
                                        SelectRandomTeamSpawnPoint( teamstate,  utteam );
			return NULL;
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectCTFSpawnPoint --Xamis-- Modified for info_ut_spawn

============
*/
gentity_t *SelectCTFSpawnPoint ( team_t team, int teamstate, vec3_t origin, vec3_t angles, qboolean isbot ) {
	gentity_t	*spot;
	int		tries = 0;

	spot = SelectRandomTeamSpawnPoint ( teamstate, team );

	if (!spot) {
		if (tries < 8 ){
		//G_Printf( S_COLOR_BLUE "WARNING: no spots SelectCTFSpawnPoint, Trying again\n"); //using SelectSpawnPoint\n" );
		spot = SelectRandomTeamSpawnPoint( teamstate,team );// try again
		tries++;
		}else{
		return SelectSpawnPoint( vec3_origin, origin, angles, isbot ); //This should never happen --Xamis
		}
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 12;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL SortClients( const void *a, const void *b ) {
	return *(int *)a - *(int *)b;
}


/*
==================
TeamplayLocationsMessage

Format:
	clientNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[8192];
	int			stringlength;
	int			i, j;
	gentity_t	*player;
	int			cnt;
	int			h, a;
	int			clients[TEAM_MAXOVERLAY];

	if ( ! ent->client->pers.teamInfo )
		return;

	// figure out what client should be on the display
	// we are limited to 8, but we want to use the top eight players
	// but in client order (so they don't keep changing position on the overlay)
	for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + level.sortedClients[i];
		if (player->inuse && player->client->sess.sessionTeam ==
			ent->client->sess.sessionTeam ) {
			clients[cnt++] = level.sortedClients[i];
		}
	}

	// We have the top eight players, sort them by clientNum
	qsort( clients, cnt, sizeof( clients[0] ), SortClients );

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + i;
		if (player->inuse && player->client->sess.sessionTeam ==
			ent->client->sess.sessionTeam ) {

			h = player->client->ps.stats[STAT_HEALTH];
			a = 0;
			if (h < 0) h = 0;
			if (a < 0) a = 0;

			Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i",
//				level.sortedClients[i], player->client->pers.teamState.location, h, a,
				i, player->client->pers.teamState.location, h, a,
				player->client->ps.weapon, player->s.powerups);
			j = strlen(entry);
			if (stringlength + j > sizeof(string))
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand( ent-g_entities, va("tinfo %i %s", cnt, string) );
}

void CheckTeamStatus(void) {
	int i;
	gentity_t *loc, *ent;

	if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME) {

		level.lastTeamLocationTime = level.time;

		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_RED ||	ent->client->sess.sessionTeam == TEAM_BLUE)) {
				loc = Team_GetLocation( ent );
				if (loc)
					ent->client->pers.teamState.location = loc->health;
				else
					ent->client->pers.teamState.location = 0;
			}
		}

		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if (ent->inuse && ( ent->client->sess.sessionTeam == TEAM_RED_SPECTATOR ||
						ent->client->sess.sessionTeam == TEAM_BLUE_SPECTATOR ||
						ent->client->sess.sessionTeam == TEAM_RED ||
						ent->client->sess.sessionTeam == TEAM_BLUE)) {
				TeamplayInfoMessage( ent );
			}
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void SP_team_CTF_redplayer( gentity_t *ent ) {
}


/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer( gentity_t *ent ) {
}


/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
potential spawning position for red team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_redspawn(gentity_t *ent) {
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for blue team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_bluespawn(gentity_t *ent) {
}


#ifdef MISSIONPACK
/*
================
Obelisks
================
*/

static void ObeliskRegen( gentity_t *self ) {
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	if( self->health >= g_obeliskHealth.integer ) {
		return;
	}

	G_AddEvent( self, EV_POWERUP_REGEN, 0 );
	self->health += g_obeliskRegenAmount.integer;
	if ( self->health > g_obeliskHealth.integer ) {
		self->health = g_obeliskHealth.integer;
	}

	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	self->activator->s.frame = 0;
}


static void ObeliskRespawn( gentity_t *self ) {
	self->takedamage = qtrue;
	self->health = g_obeliskHealth.integer;

	self->think = ObeliskRegen;
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;

	self->activator->s.frame = 0;
}


static void ObeliskDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	int			otherTeam;

	otherTeam = OtherTeam( self->spawnflags );
	AddTeamScore(self->s.pos.trBase, otherTeam, 1);
	Team_ForceGesture(otherTeam);

	CalculateRanks();

	self->takedamage = qfalse;
	self->think = ObeliskRespawn;
	self->nextthink = level.time + g_obeliskRespawnDelay.integer * 1000;

	self->activator->s.modelindex2 = 0xff;
	self->activator->s.frame = 2;

	G_AddEvent( self->activator, EV_OBELISKEXPLODE, 0 );

	AddScore(attacker, self->r.currentOrigin, CTF_CAPTURE_BONUS);

	// add the sprite over the player's head
	attacker->client->ps.eFlags &= ~( EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	attacker->client->ps.eFlags |= EF_AWARD_CAP;
	attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	attacker->client->ps.persistant[PERS_CAPTURES]++;

	teamgame.redObeliskAttackedTime = 0;
	teamgame.blueObeliskAttackedTime = 0;
}


static void ObeliskTouch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	int			tokens;

	if ( !other->client ) {
		return;
	}

	if ( OtherTeam(other->client->sess.sessionTeam) != self->spawnflags ) {
		return;
	}

	tokens = other->client->ps.generic1;
	if( tokens <= 0 ) {
		return;
	}

	PrintMsg(NULL, "%s" S_COLOR_WHITE " brought in %i skull%s.\n",
					other->client->pers.netname, tokens, tokens ? "s" : "" );

	AddTeamScore(self->s.pos.trBase, other->client->sess.sessionTeam, tokens);
	Team_ForceGesture(other->client->sess.sessionTeam);

	AddScore(other, self->r.currentOrigin, CTF_CAPTURE_BONUS*tokens);

	// add the sprite over the player's head
	other->client->ps.eFlags &= ~( EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	other->client->ps.eFlags |= EF_AWARD_CAP;
	other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	other->client->ps.persistant[PERS_CAPTURES] += tokens;

	other->client->ps.generic1 = 0;
	CalculateRanks();

	Team_CaptureFlagSound( self, self->spawnflags );
}

static void ObeliskPain( gentity_t *self, gentity_t *attacker, int damage ) {
	int actualDamage = damage / 10;
	if (actualDamage <= 0) {
		actualDamage = 1;
	}
	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	if (!self->activator->s.frame) {
		G_AddEvent(self, EV_OBELISKPAIN, 0);
	}
	self->activator->s.frame = 1;
	AddScore(attacker, self->r.currentOrigin, actualDamage);
}

gentity_t *SpawnObelisk( vec3_t origin, int team, int spawnflags) {
	trace_t		tr;
	vec3_t		dest;
	gentity_t	*ent;

	ent = G_Spawn();

	VectorCopy( origin, ent->s.origin );
	VectorCopy( origin, ent->s.pos.trBase );
	VectorCopy( origin, ent->r.currentOrigin );

	VectorSet( ent->r.mins, -15, -15, 0 );
	VectorSet( ent->r.maxs, 15, 15, 87 );

	ent->s.eType = ET_GENERAL;
	ent->flags = FL_NO_KNOCKBACK;

	if( g_gametype.integer == GT_BOMB ) {
		ent->r.contents = CONTENTS_SOLID;
		ent->takedamage = qtrue;
		ent->health = g_obeliskHealth.integer;
		ent->die = ObeliskDie;
		ent->pain = ObeliskPain;
		ent->think = ObeliskRegen;
		ent->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	}
	if( g_gametype.integer == GT_BOMB ) {
		ent->r.contents = CONTENTS_TRIGGER;
		ent->touch = ObeliskTouch;
	}

	if ( spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// mappers like to put them exactly on the floor, but being coplanar
		// will sometimes show up as starting in solid, so lif it up one pixel
		ent->s.origin[2] += 1;

		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			ent->s.origin[2] -= 1;
			G_Printf( "SpawnObelisk: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin) );

			ent->s.groundEntityNum = ENTITYNUM_NONE;
			G_SetOrigin( ent, ent->s.origin );
		}
		else {
			// allow to ride movers
			ent->s.groundEntityNum = tr.entityNum;
			G_SetOrigin( ent, tr.endpos );
		}
	}

	ent->spawnflags = team;

	trap_LinkEntity( ent );

	return ent;
}

/*QUAKED team_redobelisk (1 0 0) (-16 -16 0) (16 16 8)
*/
void SP_team_redobelisk( gentity_t *ent ) {
	gentity_t *obelisk;

	if ( g_gametype.integer <= GT_TEAM ) {
		G_FreeEntity(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if ( g_gametype.integer == GT_BOMB ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_RED, ent->spawnflags );
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	}
	if ( g_gametype.integer == GT_BOMB ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_RED, ent->spawnflags );
		obelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_RED;
	trap_LinkEntity(ent);
}

/*QUAKED team_blueobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_blueobelisk( gentity_t *ent ) {
	gentity_t *obelisk;

	if ( g_gametype.integer <= GT_TEAM ) {
		G_FreeEntity(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if ( g_gametype.integer == GT_BOMB ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_BLUE, ent->spawnflags );
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	}
	if ( g_gametype.integer == GT_BOMB ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_BLUE, ent->spawnflags );
		obelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_BLUE;
	trap_LinkEntity(ent);
}

/*QUAKED team_neutralobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_neutralobelisk( gentity_t *ent ) {
	if ( g_gametype.integer != GT_BOMB && g_gametype.integer != GT_BOMB ) {
		G_FreeEntity(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if ( g_gametype.integer == GT_BOMB) {
		neutralObelisk = SpawnObelisk( ent->s.origin, TEAM_FREE, ent->spawnflags);
		neutralObelisk->spawnflags = TEAM_FREE;
	}
	ent->s.modelindex = TEAM_FREE;
	trap_LinkEntity(ent);
}


/*
================
CheckObeliskAttack
================
*/
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker ) {
	gentity_t	*te;

	// if this really is an obelisk
	if( obelisk->die != ObeliskDie ) {
		return qfalse;
	}

	// if the attacker is a client
	if( !attacker->client ) {
		return qfalse;
	}

	// if the obelisk is on the same team as the attacker then don't hurt it
	if( obelisk->spawnflags == attacker->client->sess.sessionTeam ) {
		return qtrue;
	}

	// obelisk may be hurt

	// if not played any sounds recently
	if ((obelisk->spawnflags == TEAM_RED &&
		teamgame.redObeliskAttackedTime < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME) ||
		(obelisk->spawnflags == TEAM_BLUE &&
		teamgame.blueObeliskAttackedTime < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME) ) {

		// tell which obelisk is under attack
		te = G_TempEntity( obelisk->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
		if( obelisk->spawnflags == TEAM_RED ) {
			te->s.eventParm = GTS_REDOBELISK_ATTACKED;
			teamgame.redObeliskAttackedTime = level.time;
		}
		else {
			te->s.eventParm = GTS_BLUEOBELISK_ATTACKED;
			teamgame.blueObeliskAttackedTime = level.time;
		}
		te->r.svFlags |= SVF_BROADCAST;
	}

	return qfalse;
}
#endif


void G_EndTimer ( void )
{
    // send the config string to the clients
    trap_SetConfigstring( CS_ROUND_START_TIME, "0" );
    trap_SetConfigstring( CS_VIP_START_TIME, "0" );
    trap_SetConfigstring( CS_BOMB_START_TIME, "0" );

}




int AliveTeamCount( int ignoreClientNum, team_t team ) {
    int         i;
    int         count = 0;

    for ( i = 0 ; i < level.maxclients ; i++ ) {
        if ( i == ignoreClientNum ) {
            continue;
        }
        if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
            continue;
        }
        if ( g_gametype.integer == GT_BOMB && level.clients[i].pers.connected != CON_CONNECTED )
            continue;


	if( level.clients[i].ps.stats[STAT_HEALTH] <= 0) //How can we call the function AliveTeamCount if we don't check this? --Xamis
	    continue;

        if ( level.clients[i].sess.sessionTeam == team && level.clients[i].sess.waiting == qfalse ) {
            count++;
        }

    }

    return count;
}


void G_EndRound ( void )
{
    if (level.warmupTime != -1)
        return;

    // the round has just been won
    if ( AliveTeamCount( -1, TEAM_RED ) > 0 && AliveTeamCount( -1, TEAM_BLUE ) > 0)
        G_WonRound(TEAM_FREE);
    else if (AliveTeamCount( -1, TEAM_RED ) > 0)
        G_WonRound(TEAM_RED);
    else if (AliveTeamCount( -1, TEAM_BLUE ) > 0)
        G_WonRound(TEAM_BLUE);
    else
        G_WonRound(TEAM_FREE); // draw again

    level.done_objectives[TEAM_RED] = level.done_objectives[TEAM_BLUE] = 0;

    // buffer time until the next round starts
    level.warmupTime = -3;
    level.winTime = level.time +  ONE_SECOND; // 1 second till next match
    level.roundstartTime = 0;


   // assault_field_stopall( );


}

void G_WonRound ( team_t team )
{
    LTS_Rounds++;

    G_SetGameState( STATE_OVER );

    if (team != TEAM_RED && team != TEAM_BLUE)
    {
        gentity_t       *te;

        te = G_TempEntity(vec3_origin, EV_GLOBAL_TEAM_SOUND );
        te->r.svFlags |= SVF_BROADCAST;
        te->s.eventParm = GTS_DRAW_ROUND;

        trap_SendServerCommand( -1, "cp \"DRAW!\n\"" );

        G_LogPrintf( "DRAW.\n" );
        return;
    }
    AddTeamScore( vec3_origin, team, 1 );

    trap_SendServerCommand( -1, va("cp \"The %s team won the round.\n\"", TeamName( team ) ) );
   // G_LogPrintf( "ROUND: %s won.\n", TeamName( team ) );

    // otherteam is still the looser
    if ( level.lastLoser == OtherTeam( team ) )
        level.loseCount++;
    else if ( level.lastLoser != OtherTeam( team ) )
    {
        level.loseCount = 1;
        level.lastLoser = OtherTeam( team );
    }

    //CalculateRanks();
}


void G_EndRoundForTeam ( int team )
{
    if (level.warmupTime != -1)
        return;

    level.done_objectives[TEAM_RED] = level.done_objectives[TEAM_BLUE] = 0;


    // buffer time until the next round starts
    level.warmupTime = -3;
    level.winTime = level.time + ( 10 * ONE_SECOND ); // 10 seconds till next match
    level.roundstartTime = 0;

    //assault_field_stopall( );

    // the round has just been won by a team
    G_WonRound( team );  // give points & stuff

    // send the config string to the clients
    trap_SetConfigstring( CS_ROUND_START_TIME, "0" );
    trap_SetConfigstring( CS_VIP_START_TIME, "0" );
    trap_SetConfigstring( CS_BOMB_START_TIME, "0" );

    G_EndTimer();
}




void G_SetGameState(int state) {
    int i;

    if (state != STATE_OPEN &&
            state != STATE_LOCKED &&
            state != STATE_OVER) return;

    if (state == GameState) return;

    GameState = state;

    for ( i = 0 ; i < level.maxclients ; i++ ) {
        if ( level.clients[i].pers.connected == CON_DISCONNECTED ) continue;
        if ( g_gametype.integer == GT_BOMB && level.clients[i].pers.connected != CON_CONNECTED ) continue;
        G_AddEvent(&g_entities[level.clients[i].ps.clientNum], EV_GAMESTATE, state);
    }

}

qboolean G_BombPlanted ( team_t team )
{
    int                 i;
    gentity_t   *ent;

    ent = &g_entities[0];

    for (i=0 ; i<level.num_entities ; i++, ent++)
    {
        if ( !ent->inuse ) {
            continue;
        }
        if (!ent || !ent->classname)
            continue;

        // found roundholder? for whatever it called.
    //    if (!Q_stricmp("endround_holder", ent->classname ) )
     //       return qtrue;
        // find c4
        if (!Q_stricmp("bomb_placed", ent->classname))
            // if entity still has got time left
           // if ( ent->team == team )
                return qtrue; // don't end round
    }
    // return
    return qfalse;

}

#define MAX_VIP_SPAWN_POINTS    4
gentity_t *SelectRandomVipSpawnPoint( void ) {
    gentity_t   *spot;
    int                 count;
    int                 selection;
    gentity_t   *spots[MAX_VIP_SPAWN_POINTS];
    char            *teamname;
    char            *class = "info_ut_spawn";

    count = 0;
    spot = NULL;
    teamname = "red";

    while ((spot = G_Find (spot, FOFS(team), teamname)) != NULL) {

        if(  strcmp(spot->classname, class ))
            continue;

        if ( SpotWouldTelefrag( spot ) ) {
            continue;
        }
        spots[ count ] = spot;
        count++;
    }

    if ( !count ) {     // no spots that won't telefrag
        return G_Find( NULL, FOFS(classname), "ut_info_spawn");
    }

    selection = rand() % count;
    return spots[ selection ];
}



void G_DoVipStuff(int team, gentity_t *player)
{
    gentity_t   *vip;
   // gitem_t             *sec;
    gentity_t   *spot;
    //int                 _wp = WP_DEAGLE;
   // int                 _close = WP_KNIFE;

    if ( player == NULL ) {
        // we need to do this for both seals and tanogs.
        if ( TeamCount( -1, team ) <= 1 )
            vip = G_RandomPlayer( -1, team );
        else
            vip = G_RandomPlayer( lastvip[team], team );
    }
    else
        vip = player;


    if ( !vip )
    {

        if (team == TEAM_RED) G_EndRoundForTeam(TEAM_BLUE);
        else if (team == TEAM_BLUE) G_EndRoundForTeam(TEAM_RED);
        else G_EndRound();

        return;
    }

    vip->client->ut.is_vip = qtrue;
    lastvip[team] = vip->s.clientNum;

    // remove ammo & powerups
    //memset( vip->client->ps.powerups, 0, sizeof(vip->client->ps.powerups) );
    //memset( vip->client->ps.ammo, 0, sizeof(vip->client->ps.ammo) );

    //vip->client->ps.stats[STAT_WEAPONS] = 0;
    //vip->client->ps.stats[STAT_WEAPONS_EXT] = 0;

    vip->health = 100;


    // set new weapons
    //if ( vip->client->sess.sessionTeam == TEAM_BLUE ) {
    //    _wp = WP_DEAGLE;
    //    _close = WP_KNIFE;
    //}

    //BG_PackWeapon( _wp, vip->client->ps.stats );

    //sec = BG_FindItemForWeapon( _wp );

    //vip->client->ps.ammo[sec->giAmmoTag] = 6;
    //vip->client->ps.weapon = _wp;
    //vip->client->ps.eFlags |= EF_VIP;

    //vip->s.weapon = _wp;
   // spot = NULL;//SelectRandomSpawnPoint();
    //spot = SelectRandomVipSpawnPoint();
	spot = SelectRandomTeamSpawnPoint( 0, TEAM_RED );
    if (!spot)
    {
        G_Error("Trying to spawn V.I.P.[%s] - without spawnpoint!", TeamName(team) );
        return;
    }

    // give the briefcase to the vip
 //   if ( spot->ns_flags )
//    {
//        Pickup_Briefcase( spot, vip );
//        vip->client->ns.is_vipWithBriefcase = qtrue;
//    }

    //vip->client->ns.rewards |= REWARD_VIP_ALIVE;

    G_SetOrigin( vip, spot->s.origin );
    VectorCopy (spot->s.origin, vip->client->ps.origin);

    SetClientViewAngle( vip, spot->s.angles );

    trap_SendServerCommand( vip->client->ps.clientNum, va("cp \""S_COLOR_GREEN"You are the V.I.P.\n\""));
    G_UseTargets( spot, vip );

    ClientUserinfoChanged( vip->s.clientNum );
    G_LogPrintf( "OBJECTIVE: [%i] \"%s\" spawned as VIP\n", vip->client->ps.clientNum, vip->client->pers.netname );
}




#define ROUND_WARMUP_TIME       0;
void CheckTeamplay( void ) {
    int i;
    int minplayers = 1;//g_minPlayers.integer;
    gentity_t *bmbplayer;


    if ( minplayers < 1 )
        minplayers = 0;

    if (g_gametype.integer != GT_BOMB && g_gametype.integer != GT_TEAMSV)
        return; 

    if (g_doWarmup.integer != 0){
        trap_SendConsoleCommand( EXEC_INSERT, "g_doWarmup 0" );

}
    if ( level.intermissiontime ) {
        return;
    }
    if ( (TeamCount( -1, TEAM_RED ) +TeamCount( -1, TEAM_RED_SPECTATOR )  < minplayers) || ( TeamCount( -1, TEAM_BLUE )+TeamCount( -1, TEAM_BLUE_SPECTATOR )  < minplayers)  )
    {
       G_SetGameState( STATE_OPEN );

        if (level.time > i_sNextWaitPrint )
        {       // not enough players
            G_Printf("Waiting for players to join.\n");
            i_sNextWaitPrint = level.time + 5000;
        }
        for ( i = 0; i < level.maxclients; i++ )
        {
            gclient_t *client = &level.clients[i];
            if (client->pers.connected != CON_CONNECTED)
                continue;
            if ( client->ps.pm_flags & PMF_FOLLOW )
                continue;
            if ( client->sess.waiting == qtrue )
            {
                if ( ( TeamCount( -1, TEAM_RED )+TeamCount( -1, TEAM_RED_SPECTATOR ) > 0 ) || ( TeamCount( -1, TEAM_BLUE ) +TeamCount( -1, TEAM_BLUE_SPECTATOR )> 0 ) )
                {
                    client->sess.waiting = qfalse;
                    ClientSpawn( &g_entities[ client - level.clients ] );
                   client->ps.eFlags &= ~EF_WEAPONS_LOCKED; //removed this, don't fire during warmup! --xamis
                  //  client->ps.eFlags |= EF_WEAPONS_LOCKED;
                }
            }
        }
        b_sWaitingForPlayers = qtrue;

        level.loseCount = 0;
        level.lastLoser = TEAM_FREE;   
        level.warmupTime = -2;
        level.roundstartTime = 0;

        trap_SetConfigstring( CS_ROUND_START_TIME, "0" );
        trap_SetConfigstring( CS_VIP_START_TIME, "0" );
        trap_SetConfigstring( CS_BOMB_START_TIME, "0" );
	return;
    }
    else
    {
        // time to start a new warmup, initiate warmup - reset some variables
        if (level.winTime == level.time ) {
            level.warmupTime = -2;
            level.done_objectives[TEAM_RED] = level.done_objectives[TEAM_BLUE] = 0;
            G_SetGameState( STATE_OPEN );
        }
        // start the warmup
        if (level.warmupTime == -2)
        {
            G_SetGameState( STATE_OPEN );
            if ( LTS_Rounds == 1 ){
                level.warmupTime = level.time + 10 * ONE_SECOND; //g_firstCountdown.integer * ONE_SECOND;
            }else{
                level.warmupTime = level.time + ROUND_WARMUP_TIME;
		}
            trap_SendServerCommand( -1, va( "cp \"Round %i\"", LTS_Rounds ) );
            trap_SendServerCommand( -1, "roundst" );

            G_EndTimer();
           // G_SendResetAllStatusToAllPlayers();
            for ( i = 0; i < level.maxclients; i++ )
            {
                gclient_t *client = &level.clients[i];

                if (client->pers.connected != CON_CONNECTED)
                    continue;

                if ( client->sess.sessionTeam == TEAM_SPECTATOR )
                    continue;
                
          //      if ( client->sess.sessionTeam == TEAM_RED_SPECTATOR )
//			client->sess.sessionTeam = TEAM_RED;
               //     SetTeam(  &g_entities[ client->ps.clientNum ], "red");
             //   if ( client->sess.sessionTeam == TEAM_BLUE_SPECTATOR )
//			client->sess.sessionTeam = TEAM_BLUE;
               //     SetTeam(  &g_entities[ client->ps.clientNum ], "blue");

                client->ps.eFlags &= ~EF_VIP;
                client->ut.is_vip = client->ut.is_vipWithBriefcase = qfalse;
                client->sess.waiting = qfalse;

                if ( client->ps.pm_flags & PMF_FOLLOW )
                    StopFollowing( &g_entities[ client - level.clients ] );

                ClientSpawn( &g_entities[ client - level.clients ] );
                // for all clients in game... lock weapons
                client->ps.eFlags |= EF_WEAPONS_LOCKED;
                ClientUserinfoChanged( client->ps.clientNum );
            }
        }
        // our round begins
        if (level.warmupTime == level.time)
        {
            G_ResetEntities();
           // Time to start the next round
            level.warmupTime = -1;
            G_SetGameState( STATE_LOCKED );
		if ( 1/*g_roundTime.integer */)
                level.roundstartTime = level.time + ( 2/*g_roundTime.integer*/ * 60 * ONE_SECOND ); // round now started
            else
                level.roundstartTime = -1;
            trap_SetConfigstring( CS_ROUND_START_TIME, va("%i",level.roundstartTime) );
            // start timelimit
            if ( LTS_Rounds == 1 )
            {
                level.startTime = level.time;
                trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

                // spawn all clients that are still waiting to get spawned
                for ( i = 0; i < level.maxclients; i++ )
                {
                    gclient_t *client = &level.clients[i];

                    if (client->pers.connected != CON_CONNECTED)
                        continue;
                    if ( client->sess.sessionTeam == TEAM_SPECTATOR )
                        continue;
                    if ( !client->sess.waiting )
                        continue;

                    if ( client->ps.pm_flags & PMF_FOLLOW )
                        StopFollowing( &g_entities[ client - level.clients ] );

                    // spawn
                    ClientSpawn( &g_entities[ client - level.clients ] );
                }
            }

            for ( i = 0; i < level.maxclients; i++ )
            {
                gclient_t *client = &level.clients[i];

                if (client->pers.connected != CON_CONNECTED)
                    continue;
                // if we're a waiting player don't ready our weapons
                if (client->sess.waiting != qfalse)
                    continue;
                if ( client->sess.sessionTeam == TEAM_SPECTATOR )
                    continue;
                // unlock weapons
                client->ps.eFlags &= ~EF_WEAPONS_LOCKED;
            }

              if (g_gametype.integer == GT_BOMB ){
            // give bombs to players that require it.
            if ( ! G_GiveBombToTeam( TEAM_RED ) ) {
                G_EndRoundForTeam(TEAM_BLUE);
                return;
            }
            // radiobroadcast the start radiomsg.
            bmbplayer =G_RandomPlayer(-1, TEAM_RED);
            
          }
            // log print a start
            G_LogPrintf( "ROUND: Start.\n" );
        }

        if ( level.warmupTime != -3 )
        {
                        G_SetGameState( STATE_LOCKED );
            {
                if ( ( level.done_objectives[TEAM_RED] >= level.num_objectives[TEAM_RED] ) && level.num_objectives[TEAM_RED] > 0 )
                {
                    G_EndRoundForTeam( TEAM_RED );
                    return;
                }
                else if ( ( level.done_objectives[TEAM_BLUE] >= level.num_objectives[TEAM_BLUE] ) && level.num_objectives[TEAM_BLUE] > 0 )
                { 
                    G_EndRoundForTeam( TEAM_BLUE );
                    return;
                }
            }

            if ( AliveTeamCount( -1, TEAM_RED ) + AliveTeamCount( -1, TEAM_BLUE ) <= 0 )
            {

                if ( G_BombPlanted( TEAM_RED ) )
                    G_EndRoundForTeam( TEAM_RED );
                else 
		    G_EndRoundForTeam( TEAM_BLUE );

                return;

           }
            if (AliveTeamCount( -1, TEAM_RED ) < 1  ){
                G_EndRoundForTeam( TEAM_BLUE );
                return;
            }
            else if ( AliveTeamCount( -1, TEAM_BLUE ) < 1 ){
                 G_EndRoundForTeam( TEAM_RED ); 
                return;
            }
            if (level.roundstartTime == level.time)
                G_EndRound();
        }
    }

    {
        char buffer[512];
        int oldWarmup;

        trap_GetConfigstring( CS_WARMUP, buffer, sizeof(buffer) );
        oldWarmup = atoi(buffer);

        if (level.warmupTime > 0 )
            trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
        else if ( level.warmupTime == -2 )
            trap_SetConfigstring( CS_WARMUP, "-1" );
        else if ( oldWarmup  != 0 )
            trap_SetConfigstring( CS_WARMUP, "0" );
    }
}


/*
=================
G_ResetEntities
=================
*/
void G_ResetEntities( void ) {

    // launch entities
    int			i;
    gentity_t	*ent;


    ent = &g_entities[0];
    for (i=0 ; i<level.num_entities ; i++, ent++) {

         if (!Q_stricmp("func_wall", ent->classname ) )
        {
            ent->r.contents = CONTENTS_SOLID;
            ent->r.svFlags &= ~SVF_NOCLIENT;

            if (ent->spawnflags & 4)
            {
                ent->r.contents = CONTENTS_SOLID;
            }
            else
            {
                ent->r.contents = 0;
                ent->r.svFlags |= SVF_NOCLIENT;
            }
        } else  if ( (!Q_stricmp("func_door", ent->classname )) ||
                    !Q_stricmp("func_rotating_door", ent->classname ) )
        {
            Door_ResetState( ent );
        } else if (!Q_stricmp("target_delay", ent->classname) )
        {
            ent->nextthink = 0;
            ent->think = 0;
            ent->activator = 0;
        } 
    }

}


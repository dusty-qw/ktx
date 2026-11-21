#include "g_local.h"

#define WOX_FLAG_CLASSNAME "item_wipeoutx_flag"
#define WOX_FLAG_PICKUP_DELAY 0.75

typedef struct wox_ammo_s
{
	float shells, nails, rockets, cells;
} wox_ammo_t;

static char *wox_flag_model = "progs/w_g_key.mdl";
static int wox_round_winner;

// Sequence numbers preserve kill order even when several players die in one frame.
static unsigned int wox_death_sequence;
static unsigned int wox_death_order[MAX_CLIENTS + 1];

// Latch the extra respawn step at death, independently of later flag changes.
static qbool wox_respawn_penalty[MAX_CLIENTS + 1];

// Remember possession across drops so passes and same-team recoveries stay silent.
static int wox_flag_team;

// There is only one flag, so one snapshot follows its current carrier.
static wox_ammo_t wox_carrier_ammo;

// The upright CTF model's cloth extends along +Y, above the pole's origin.
static const vec3_t wox_flag_pickup_mins = { -16, -16, -2 };
static const vec3_t wox_flag_pickup_maxs = { 16, 56, 80 };

void WOX_Precache(void)
{
	// Use the CTF flag model when installed, with CTF's stock Quake fallback.
	wox_flag_model = can_exec("progs/flag.mdl") ? "progs/flag.mdl" : "progs/w_g_key.mdl";
	trap_precache_model(wox_flag_model);
	trap_precache_sound("misc/flagtk.wav");
}

static qbool WOX_FlagActive(void)
{
	// Keep the flag through the round pause; the next round resets it.
	return (isWipeoutX() && match_in_progress == 2 && !match_over);
}

static qbool WOX_CanCarryFlag(gedict_t *p)
{
	// Only living round participants can carry or pick up the flag.
	return (p->ct == ctPlayer && p->ca_ready && p->in_play && ISLIVE(p));
}

void WOX_RefreshCarrierAmmo(gedict_t *p)
{
	// Spectators copy inventory items, so use actual possession to grant the bonus.
	if (!WOX_FlagActive() || !(p->ctf_flag & CTF_FLAG) || !WOX_CanCarryFlag(p))
	{
		return;
	}

	// Picking up the flag during the pause must keep unlimited GL ammo too.
	if (ca_round_pause)
	{
		W_RefreshUnlimitedAmmo(p, true);
		return;
	}

	// GL uses ca_ammo_grenades and must continue spending its own ammunition.
	W_RefreshUnlimitedAmmo(p, false);
}

static void WOX_ClearCarrier(gedict_t *p)
{
	// A new round may already have spawned this player and cleared their flag bit.
	if (p->ctf_flag & CTF_FLAG)
	{
		// Restore only the boosted pools; grenades fired while carrying stay spent.
		p->s.v.ammo_shells = wox_carrier_ammo.shells;
		p->s.v.ammo_nails = wox_carrier_ammo.nails;
		p->s.v.ammo_rockets = wox_carrier_ammo.rockets;
		p->s.v.ammo_cells = wox_carrier_ammo.cells;
		W_UpdateCurrentAmmo(p);
	}

	// Remove possession, the back-mounted model, and the HUD flag indicator.
	p->ctf_flag &= ~CTF_FLAG;
	p->s.v.effects = (int)p->s.v.effects & ~EF_FLAG1;
	p->s.v.items = (int)p->s.v.items & ~IT_KEY2;

	// A pause-time toss must not briefly replace unlimited ammo with the saved counts.
	if (WOX_FlagActive() && ca_round_pause)
	{
		W_RefreshUnlimitedAmmo(p, true);
	}
}

void WOX_ResetFlag(void)
{
	gedict_t *flag = world;

	// Discard the previous round's result, possession history, and death order.
	wox_round_winner = 0;
	wox_flag_team = 0;
	wox_death_sequence = 0;
	memset(wox_death_order, 0, sizeof(wox_death_order));
	memset(wox_respawn_penalty, 0, sizeof(wox_respawn_penalty));

	// Remove both carried and dropped flag entities, clearing any carrier first.
	while ((flag = find(flag, FOFCLSN, WOX_FLAG_CLASSNAME)))
	{
		if (flag->cnt == FLAG_CARRIED)
		{
			WOX_ClearCarrier(PROG_TO_EDICT(flag->s.v.owner));
		}

		ent_remove(flag);
	}
}

static int WOX_PlayerTeam(gedict_t *p)
{
	// Match CA's team numbers; zero means the player belongs to neither team.
	if (streq(getteam(p), cvar_string("_k_team1")))
	{
		return 1;
	}

	return streq(getteam(p), cvar_string("_k_team2")) ? 2 : 0;
}

static void WOX_RespawnPlayer(gedict_t *p)
{
	// Ghosts already counted this death; count fresh corpses before bypassing limbo.
	if (p->in_play)
	{
		p->round_deaths++;
	}

	// Freshly killed players may not have entered limbo yet.
	// If we don't set this, they may become a ghost instead of respawning.
	p->in_limbo = true;

	// Respawn without copying the corpse, as it would freeze its unfinished death animation.
	k_respawn(p, false);

	// Discard the pending respawn timer and preview the next death's delay.
	p->spawn_delay = 0;
	p->seconds_to_respawn = calc_respawn_time(p, 1);
	p->time_of_respawn = g_globalvars.time;
	snprintf(p->cptext, sizeof(p->cptext), "FIGHT!\n");
	G_centerprint(p, "%s", p->cptext);
}

qbool WOX_RespawnPenalty(gedict_t *p)
{
	return wox_respawn_penalty[NUM_FOR_EDICT(p)];
}

int WOX_PlayerDied(gedict_t *p, gedict_t *attacker)
{
	gedict_t *teammate, *flag, *carrier;
	int team = WOX_PlayerTeam(p);
	qbool enemy_has_flag;

	// Only deaths during an undecided, active round can determine a winner.
	if (!WOX_FlagActive() || ra_match_fight != 2 || ca_round_pause
		|| wox_round_winner || !p->ca_ready || !p->in_play || !team)
	{
		return 0;
	}

	// Record every participating death before checking whether it completes a wipe.
	wox_death_order[NUM_FOR_EDICT(p)] = ++wox_death_sequence;

	// Only active possession counts, not the last team to hold a dropped flag.
	flag = find(world, FOFCLSN, WOX_FLAG_CLASSNAME);
	carrier = flag && flag->cnt == FLAG_CARRIED ? PROG_TO_EDICT(flag->s.v.owner) : NULL;
	enemy_has_flag = carrier && WOX_CanCarryFlag(carrier) && WOX_PlayerTeam(carrier) == 3 - team;

	// Enemy kills gain one delay step while their team holds the flag.
	wox_respawn_penalty[NUM_FOR_EDICT(p)] = enemy_has_flag && attacker->ct == ctPlayer
		&& WOX_PlayerTeam(attacker) == 3 - team;

	// Publish the actual delay immediately, before the scoreboard can show the old preview.
	// The ghost transition has not counted this death yet, so include it with the penalty.
	if (p->can_respawn)
	{
		p->seconds_to_respawn = calc_respawn_time(p, 1 + WOX_RespawnPenalty(p));
	}

	// This is a wipe only if none of the dying player's teammates remain alive.
	for (teammate = world; (teammate = find_plr(teammate));)
	{
		if (teammate != p && WOX_CanCarryFlag(teammate) && WOX_PlayerTeam(teammate) == team)
		{
			return 0;
		}
	}

	// The opposing team must have a living carrier at the moment of the wipe.
	if (enemy_has_flag)
	{
		// Latch possession at the final death, before any later toss or pickup.
		wox_round_winner = 3 - team;
	}

	return wox_round_winner;
}

int WOX_CheckRound(void)
{
	gedict_t *p;
	gedict_t *last_killed[3] = { NULL, NULL, NULL };
	int team, participating = 0, alive = 0, eliminated, announced = 0;

	// Leave other modes, warmup, and completed matches untouched.
	if (!WOX_FlagActive())
	{
		return 0;
	}

	// Keep the recorded result through the pause without reviving either team.
	if (wox_round_winner || ca_round_pause || ra_match_fight != 2)
	{
		return wox_round_winner;
	}

	// Use each team number as a bit position to track participation and survivors.
	for (p = world; (p = find_plr(p));)
	{
		int team = WOX_PlayerTeam(p);

		if (!p->ca_ready || !team)
		{
			continue;
		}

		participating |= 1 << team;
		if (WOX_CanCarryFlag(p))
		{
			alive |= 1 << team;
		}
		// Select the latest eligible death on each team, excluding suicides and late joins.
		else if (p->can_respawn && wox_death_order[NUM_FOR_EDICT(p)]
			&& (!last_killed[team] || wox_death_order[NUM_FOR_EDICT(p)]
				> wox_death_order[NUM_FOR_EDICT(last_killed[team])]))
		{
			last_killed[team] = p;
		}
	}

	// Snapshot both wiped teams before reviving anyone, including mutual wipes.
	eliminated = participating & ~alive;
	for (team = 1; team <= 2; team++)
	{
		// Only the most recently killed eligible player on a wiped team returns.
		p = last_killed[team];
		if (!p || !(eliminated & (1 << team)))
		{
			continue;
		}

		// Name the returning player and the opponent that lacked the flag.
		G_bprint(2, "team [%s] wasn't holding the flag! %s respawns!\n",
			cvar_string(team == 1 ? "_k_team2" : "_k_team1"), p->netname);
		announced |= 1 << team;

		WOX_RespawnPlayer(p);
	}

	// Send team-specific sounds once after the selected players have respawned.
	if (announced)
	{
		for (p = world; (p = find_plr(p));)
		{
			int team = WOX_PlayerTeam(p);

			// When both teams revive, both hear the revival sound.
			if (announced & (1 << team))
			{
				stuffcmd(p, "play ca/hero.wav\n");
			}
			else if (team && (announced & (1 << (3 - team))))
			{
				stuffcmd(p, "play boss2/idle.wav\n");
			}
		}
	}

	return 0;
}

static void WOX_GiveFlag(gedict_t *flag, gedict_t *p)
{
	int team = WOX_PlayerTeam(p);
	qbool team_changed = wox_flag_team && team && team != wox_flag_team;
	gedict_t *teammate;

	// Save this carrier's ammo before granting the temporary unlimited supply.
	wox_carrier_ammo.shells = p->s.v.ammo_shells;
	wox_carrier_ammo.nails = p->s.v.ammo_nails;
	wox_carrier_ammo.rockets = p->s.v.ammo_rockets;
	wox_carrier_ammo.cells = p->s.v.ammo_cells;

	// Hide the world flag while the client displays it on the carrier's back.
	flag->cnt = FLAG_CARRIED;
	flag->s.v.solid = SOLID_NOT;
	flag->s.v.movetype = MOVETYPE_NONE;
	flag->s.v.owner = EDICT_TO_PROG(p);
	VectorClear(flag->s.v.velocity);
	VectorClear(flag->s.v.avelocity);
	setmodel(flag, "");

	// CTF's possession bits supply the carried model and teaminfo HUD icon.
	p->ctf_flag |= CTF_FLAG;
	p->s.v.effects = (int)p->s.v.effects | EF_FLAG1;
	p->s.v.items = (int)p->s.v.items | IT_KEY2;
	WOX_RefreshCarrierAmmo(p);

	// Commit possession before respawns can trigger any further game callbacks.
	wox_flag_team = team;

	// Sound only switches between teams; the initial assignment stays silent.
	if (team_changed)
	{
		sound(p, CHAN_FLAG, "misc/flagtk.wav", 1, ATTN_NONE);
	}
	G_bprint(2, "%s has the flag!\n", p->netname);

	// Taking possession from the enemy immediately returns eligible dead teammates.
	if (team_changed)
	{
		for (teammate = world; (teammate = find_plr(teammate));)
		{
			if (teammate->ca_ready && teammate->can_respawn
				&& WOX_PlayerTeam(teammate) == team && !WOX_CanCarryFlag(teammate))
			{
				WOX_RespawnPlayer(teammate);
			}
		}
	}
}

static qbool WOX_TryPickupFlag(gedict_t *flag, gedict_t *p)
{
	// Allow pickups during the countdown, combat, and the round pause.
	if (!WOX_FlagActive() || (!ca_round_pause && ra_match_fight != 1 && ra_match_fight != 2)
		|| flag->cnt != FLAG_DROPPED
		|| !WOX_CanCarryFlag(p))
	{
		return false;
	}

	// Prevent the thrower from immediately catching their own toss.
	if (flag->s.v.owner == EDICT_TO_PROG(p) && g_globalvars.time < flag->attack_finished)
	{
		return false;
	}

	WOX_GiveFlag(flag, p);
	return true;
}

static void WOX_FlagTouch(void)
{
	// Physics touches attempt pickup without adding bounce sounds or rotation.
	if (!WOX_FlagActive() || (!ca_round_pause && ra_match_fight != 1 && ra_match_fight != 2)
		|| self->cnt != FLAG_DROPPED)
	{
		return;
	}

	WOX_TryPickupFlag(self, other);
}

static void WOX_CheckFlagPickup(gedict_t *flag)
{
	gedict_t *p;
	int axis;

	// Keep pickup bounds independent of the point hull used for bouncing.
	for (p = world; (p = find_plr(p));)
	{
		// Reject players whose bounds miss the flag's pickup bounds on any axis.
		for (axis = 0; axis < 3; axis++)
		{
			if (p->s.v.absmax[axis] < flag->s.v.origin[axis] + wox_flag_pickup_mins[axis]
				|| p->s.v.absmin[axis] > flag->s.v.origin[axis] + wox_flag_pickup_maxs[axis])
			{
				break;
			}
		}

		// Overlap on all three axes lets any part of the flag model be touched.
		if (axis == 3 && WOX_TryPickupFlag(flag, p))
		{
			return;
		}
	}
}

static void WOX_FlagThink(void)
{
	// Remove the flag and carrier markers when the match or mode ends.
	if (!WOX_FlagActive())
	{
		WOX_ResetFlag();
		return;
	}

	if (self->cnt == FLAG_DROPPED && g_globalvars.time >= self->attack_finished)
	{
		// Let the thrower pick it up again after the brief pickup delay.
		self->s.v.owner = EDICT_TO_PROG(self);
	}

	// Check the full pickup bounds because the bouncing hull is only a point.
	if (self->cnt == FLAG_DROPPED)
	{
		WOX_CheckFlagPickup(self);
	}

	// Check moving flags frequently so brief overlaps still register as pickups.
	self->s.v.nextthink = g_globalvars.time + (self->cnt == FLAG_DROPPED ? 0.01 : 0.1);
}

void WOX_StartRound(void)
{
	gedict_t *p, *carrier = NULL, *flag;
	int count = 0;

	// Each new round replaces the previous flag and clears its recorded winner.
	WOX_ResetFlag();
	if (!WOX_FlagActive())
	{
		return;
	}

	// Reservoir sampling gives every participating player the same chance.
	for (p = world; (p = find_plr(p));)
	{
		if (WOX_CanCarryFlag(p) && i_rnd(1, ++count) == 1)
		{
			carrier = p;
		}
	}

	// Do not create an unowned flag if no eligible round participants exist.
	if (!carrier)
	{
		return;
	}

	// Create the shared flag entity; giving it to the carrier initially hides it.
	flag = spawn();
	flag->classname = WOX_FLAG_CLASSNAME;
	flag->s.v.skin = 0;
	flag->s.v.effects = EF_DIMLIGHT;
	flag->touch = (func_t) WOX_FlagTouch;
	flag->think = (func_t) WOX_FlagThink;
	flag->s.v.nextthink = g_globalvars.time + 0.1;
	WOX_GiveFlag(flag, carrier);
}

void WOX_PlayerDropFlag(gedict_t *p, qbool tossed)
{
	gedict_t *flag = find(world, FOFCLSN, WOX_FLAG_CLASSNAME);

	// Only the current carrier can drop or toss this flag.
	if (!flag || flag->cnt != FLAG_CARRIED || flag->s.v.owner != EDICT_TO_PROG(p))
	{
		return;
	}

	// Ending the mode removes the flag rather than leaving a dropped entity.
	if (!WOX_FlagActive())
	{
		WOX_ResetFlag();
		return;
	}

	// Living carriers can toss during the countdown, combat, and the round pause.
	if (tossed && ((!ca_round_pause && ra_match_fight != 1 && ra_match_fight != 2)
		|| !WOX_CanCarryFlag(p)))
	{
		return;
	}

	// Replace the carried model with a bouncing world entity.
	WOX_ClearCarrier(p);
	flag->cnt = FLAG_DROPPED;
	flag->s.v.movetype = MOVETYPE_BOUNCE;
	flag->s.v.solid = SOLID_BBOX;
	flag->s.v.flags = 0;
	flag->s.v.groundentity = EDICT_TO_PROG(world);
	// Reserve the brief pickup lock for deliberate tosses, not death drops.
	flag->s.v.owner = tossed ? EDICT_TO_PROG(p) : EDICT_TO_PROG(flag);
	flag->attack_finished = g_globalvars.time + (tossed ? WOX_FLAG_PICKUP_DELAY : 0);
	flag->s.v.nextthink = g_globalvars.time + 0.01;
	setmodel(flag, wox_flag_model);
	// A grenade's point hull keeps its trajectory and bounces identical.
	setsize(flag, 0, 0, 0, 0, 0, 0);
	setorigin(flag, PASSVEC3(p->s.v.origin));

	// Deliberate tosses use grenade launch velocity; death drops pop upward.
	if (tossed)
	{
		float r1 = crandom(), r2 = crandom();

		W_SetGrenadeVelocity(flag, p, r1, r2);
	}
	else
	{
		SetVector(flag->s.v.velocity, 0, 0, 300);
	}

	// Keep grenade movement while holding the flag upright, without spin.
	VectorClear(flag->s.v.angles);
	VectorClear(flag->s.v.avelocity);

	G_bprint(2, "%s %s the flag!\n", p->netname, tossed ? "tossed" : "dropped");
}

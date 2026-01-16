#include "g_local.h"
#include "stats.h"

// Snapshot format version and size limits for laststats.json.
#define LASTSTATS_JSON_VERSION 2
#define LS_INVALID_INT (-1)
#define LS_INVALID_FLOAT (-1.0f)
#define LS_JSON_MAX (256 * 1024)
#define LS_NAME_LEN 64

// Flags derived from match state; drive JSON content and print format.
typedef struct
{
	qbool midair;
	qbool instagib;
	qbool lgc;
	qbool ctf;
	qbool team;
	qbool duel;
	qbool ctf_runes;
	qbool ra;
	qbool ca;
	qbool race;
	qbool socd;
	qbool time_valid;
	qbool overtime;
	int deathmatch;
} laststats_flags_t;

// Flattened per-player snapshot used for JSON storage and /laststats output.
typedef struct
{
	char name[LS_NAME_LEN];
	char team[MAX_TEAM_NAME];
	qbool ghost;
	qbool bot;
	int ct;

	int frags;
	int frags_raw;
	int frag_diff;
	int deaths;
	int friendly;
	float efficiency;
	int spawn_frags;
	int spree_max;
	int spree_max_q;

	int health;
	int armor;
	int armor_items;
	int ot_h;
	int ot_a;
	int ot_items;

	float speed_max;
	float speed_avg;

	int control_time;
	int control_pct;
	float quad_time;

	int socd_perfect;
	int socd_changes;
	int socd_detection;
	int socd_validation;
	qbool socd_assisted;

	int ga;
	int ya;
	int ra;
	int mh;
	int quad;
	int pent;
	int ring;

	int quad_ekills;
	int quad_tkills;
	int ring_ekills;
	int ring_tkills;

	int dmg_t;
	int dmg_g;
	int dmg_team;
	int dmg_self;
	int dmg_eweapon;
	int dmg_todie;

	float rl_ad;
	int rl_hits;

	float lg_pct;
	int lg_hits;
	int lg_attacks;
	int lg_tooks;
	int lg_inv_ekills;
	int lg_drops;
	int lg_xfer;

	float rl_pct;
	int rl_vhits;
	int rl_attacks;
	int rl_tooks;
	int rl_inv_ekills;
	int rl_drops;
	int rl_xfer;

	float gl_pct;
	int gl_hits;

	float sg_pct;
	int sg_hits;
	int sg_attacks;
	int sg_damage;

	float ssg_pct;
	int ssg_hits;
	int ssg_attacks;
	int ssg_damage;

	float axe_pct;
	int axe_hits;
	int axe_attacks;

	int pickups;
	int caps;
	int returns;
	int f_defends;
	int c_defends;
	int res;
	int str;
	int hst;
	int rgn;

	int axe_ekills;
	int axe_tkills;
	int sg_ekills;
	int sg_tkills;
	int ssg_ekills;
	int ssg_tkills;
	int ng_ekills;
	int ng_tkills;
	int sng_ekills;
	int sng_tkills;
	int gl_ekills;
	int gl_tkills;
	int rl_ekills;
	int rl_tkills;
	int lg_ekills;
	int lg_tkills;
	int discharge_ekills;
	int discharge_tkills;

	int lgc_over;
	int lgc_under;

	int mid_total;
	int mid_bronze;
	int mid_silver;
	int mid_gold;
	int mid_platinum;
	int mid_stomps;
	int mid_bonus;
	float mid_maxheight;
	float mid_avgheight;

	int i_cggibs;
	int i_axegibs;
	int i_stompgibs;
	int i_multigibs;
	int i_maxmultigibs;
	int i_airgibs;
	int i_height;
	int i_maxheight;
	int i_rings;
} laststats_player_t;

// Aggregated team totals for version 2 summary output.
typedef struct
{
	int frags;
	int kills;
	int deaths;
	int suicides;
	int friendly;
	int spawn_frags;
	float efficiency;
} ls_kill_row_t;

typedef struct
{
	int ga;
	int ya;
	int ra;
	int mh;
	int quad;
	int pent;
	int ring;
} ls_item_row_t;

typedef struct
{
	float lg;
	float rl;
	float gl;
	float sng;
	float ng;
	float ssg;
	float sg;
} ls_wep_eff_row_t;

typedef struct
{
	int lg;
	int rl;
	int gl;
	int sng;
	int ng;
	int ssg;
	int sg;
} ls_wep_dmg_row_t;

typedef struct
{
	int lg;
	int rl;
	int gl;
	int sng;
	int ng;
	int ssg;
} ls_wep_simple_row_t;

typedef struct
{
	int taken;
	int given;
	int eweapon;
	int team;
	int self;
	int per_death;
} ls_damage_row_t;

typedef struct
{
	int ra;
	int ya;
	int ga;
	int quad;
	int pent;
	int ring;
} ls_item_time_row_t;

typedef struct
{
	int lg;
	int rl;
	int gl;
	int sng;
	int ng;
	int ssg;
} ls_wep_time_row_t;

typedef struct
{
	int pickups;
	int caps;
	int returns;
	int f_defends;
	int c_defends;
	int res;
	int str;
	int hst;
	int rgn;
} ls_ctf_row_t;

typedef struct
{
	int total;
	int bronze;
	int silver;
	int gold;
	int platinum;
	int stomps;
	int bonus;
	float max_height;
	float avg_height;
} ls_midair_row_t;

typedef struct
{
	int frags;
	int spawn_frags;
	int spree;
	float rl_eff;
} ls_midair_kill_row_t;

typedef struct
{
	int cggibs;
	int axegibs;
	int stompgibs;
	int multigibs;
	int maxmultigibs;
	int airgibs;
	int total_height;
	int max_height;
	int rings;
} ls_insta_row_t;

typedef struct
{
	int frags;
	int kills;
	int deaths;
	int spawn_frags;
	int spree;
	float sg_eff;
} ls_insta_kill_row_t;

typedef struct
{
	int score;
	float over_pct;
	float under_pct;
	float eff_pct;
} ls_lgc_row_t;

typedef struct
{
	char name[MAX_TEAM_NAME];
	int ra;
	int ya;
	int ga;
	int mh;
	int quad;
	int pent;
	int ring;
} ls_team_items_row_t;

typedef struct
{
	char name[MAX_TEAM_NAME];
	int lgt;
	int lgk;
	int lgd;
	int lgx;
	int rlt;
	int rlk;
	int rld;
	int rlx;
} ls_team_weapons_row_t;

typedef struct
{
	char name[MAX_TEAM_NAME];
	int frags;
	int given;
	int taken;
	int eweapon;
	int team;
} ls_team_damage_row_t;

typedef struct
{
	char name[MAX_TEAM_NAME];
	int pickups;
	int caps;
	int returns;
	int f_defends;
	int c_defends;
	int res;
	int str;
	int hst;
	int rgn;
} ls_team_ctf_row_t;

typedef struct
{
	qbool has_value;
	int value;
	int players[MAX_CLIENTS];
	int player_count;
} ls_top_int_entry_t;

typedef struct
{
	qbool has_value;
	float value;
	int players[MAX_CLIENTS];
	int player_count;
} ls_top_float_entry_t;

typedef struct
{
	ls_top_int_entry_t frags;
	ls_top_int_entry_t deaths;
	ls_top_int_entry_t friendkills;
	ls_top_int_entry_t efficiency;
	ls_top_int_entry_t fragstreak;
	ls_top_int_entry_t quadrun;
	ls_top_int_entry_t annihilator;
	ls_top_int_entry_t survivor;
	ls_top_int_entry_t spawnfrags;
	ls_top_int_entry_t captures;
	ls_top_int_entry_t flagdefends;
} ls_top_stats_t;

typedef struct
{
	ls_top_int_entry_t score;
	ls_top_int_entry_t kills;
	ls_top_int_entry_t midairs;
	ls_top_int_entry_t headstomps;
	ls_top_int_entry_t streak;
	ls_top_int_entry_t spawnfrags;
	ls_top_int_entry_t bonus;
	ls_top_float_entry_t highest;
	ls_top_float_entry_t avgheight;
	ls_top_float_entry_t rleff;
} ls_top_midair_t;

typedef struct
{
	char name[MAX_TEAM_NAME];
	int frags;
	int gfrags;
	int deaths;
	int tkills;
	int dmg_t;
	int dmg_g;
	int dmg_team;
	int dmg_eweapon;
	int ga;
	int ya;
	int ra;
	int mh;
	int quad;
	int pent;
	int ring;
	float quad_time;
	int rl_took;
	int rl_ekill;
	int rl_drop;
	int rl_xfer;
	int lg_took;
	int lg_ekill;
	int lg_drop;
	int lg_xfer;
	int lg_hits;
	int lg_attacks;
	int rl_hits;
	int gl_hits;
	int sg_hits;
	int sg_attacks;
	int ssg_hits;
	int ssg_attacks;
	int pickups;
	int caps;
	int returns;
	int f_defends;
	int c_defends;
	int res;
	int str;
	int hst;
	int rgn;
} ls_team_stats_v2_t;

typedef struct
{
	int version;
	laststats_flags_t flags;
	int ca_team1_score;
	int ca_team2_score;
	char ca_team1_name[MAX_TEAM_NAME];
	char ca_team2_name[MAX_TEAM_NAME];
	int player_count;
	laststats_player_t players[MAX_CLIENTS];

	int kill_count;
	ls_kill_row_t kill[MAX_CLIENTS];
	int item_count;
	ls_item_row_t items[MAX_CLIENTS];
	int wep_eff_count;
	ls_wep_eff_row_t wep_eff[MAX_CLIENTS];
	int wep_dmg_count;
	ls_wep_dmg_row_t wep_dmg[MAX_CLIENTS];
	int wep_taken_count;
	ls_wep_simple_row_t wep_taken[MAX_CLIENTS];
	int wep_drop_count;
	ls_wep_simple_row_t wep_drop[MAX_CLIENTS];
	int wep_kill_count;
	ls_wep_simple_row_t wep_kill[MAX_CLIENTS];
	int wep_ekill_count;
	ls_wep_simple_row_t wep_ekill[MAX_CLIENTS];
	int damage_count;
	ls_damage_row_t damage[MAX_CLIENTS];
	int item_time_count;
	ls_item_time_row_t item_time[MAX_CLIENTS];
	int wep_time_count;
	ls_wep_time_row_t wep_time[MAX_CLIENTS];
	int ctf_count;
	ls_ctf_row_t ctf[MAX_CLIENTS];

	int midair_count;
	ls_midair_row_t midair[MAX_CLIENTS];
	int midair_kill_count;
	ls_midair_kill_row_t midair_kill[MAX_CLIENTS];

	int insta_count;
	ls_insta_row_t insta[MAX_CLIENTS];
	int insta_kill_count;
	ls_insta_kill_row_t insta_kill[MAX_CLIENTS];

	int lgc_count;
	ls_lgc_row_t lgc[MAX_CLIENTS];

	int team_count;
	ls_team_items_row_t team_items[MAX_TM_STATS];
	ls_team_weapons_row_t team_weapons[MAX_TM_STATS];
	ls_team_damage_row_t team_damage[MAX_TM_STATS];
	ls_team_ctf_row_t team_ctf[MAX_TM_STATS];

	ls_top_stats_t top;
	ls_top_midair_t top_midair;
} laststats_snapshot_t;

static float laststats_efficiency(const gedict_t *player)
{
	if (isCTF())
	{
		float frags = player->s.v.frags - player->ps.ctf_points;
		if (frags < 1)
		{
			return 0;
		}
		return frags / (frags + player->deaths) * 100.0f;
	}

	if (isRA())
	{
		if ((player->ps.loses + player->ps.wins) == 0)
		{
			return 0;
		}
		return (player->ps.wins * 100.0f) / (player->ps.loses + player->ps.wins);
	}

	if (player->s.v.frags < 1)
	{
		return 0;
	}
	return player->s.v.frags / (player->s.v.frags + player->deaths) * 100.0f;
}

static char* json_string(const char *input)
{
	static char string[MAX_STRINGS][1024];
	static int index = 0;
	char *ch, *start;

	index %= MAX_STRINGS;
	start = ch = string[index++];
	while (*input)
	{
		unsigned char current = *input;

		if ((ch - start) >= 1000)
		{
			break;
		}

		if ((current == '\\') || (current == '"'))
		{
			*ch++ = '\\';
			*ch++ = current;
		}
		else if (current == '\n')
		{
			*ch++ = '\\';
			*ch++ = 'n';
		}
		else if (current == '\r')
		{
			*ch++ = '\\';
			*ch++ = 'r';
		}
		else if (current == '\b')
		{
			*ch++ = '\\';
			*ch++ = 'b';
		}
		else if (current == '\t')
		{
			*ch++ = '\\';
			*ch++ = 't';
		}
		else if (current == '\f')
		{
			*ch++ = '\\';
			*ch++ = 'f';
		}
		else if ((current < ' ') || (current >= 128))
		{
			*ch++ = '\\';
			*ch++ = 'u';
			*ch++ = '0';
			*ch++ = '0';
			if (current < 16)
			{
				*ch++ = '0';
				*ch++ = "0123456789ABCDEF"[(int)current];
			}
			else
			{
				*ch++ = "0123456789ABCDEF"[((int)(current)) >> 4];
				*ch++ = "0123456789ABCDEF"[((int)(current)) & 15];
			}
		}
		else
		{
			*ch++ = current;
		}

		++input;
	}

	*ch = '\0';

	return start;
}

static void json_write_int_or_null(fileHandle_t handle, int value)
{
	if (value == LS_INVALID_INT)
	{
		S2di(handle, "null");
		return;
	}

	S2di(handle, "%d", value);
}

static void json_write_float_or_null(fileHandle_t handle, float value)
{
	if (value == LS_INVALID_FLOAT)
	{
		S2di(handle, "null");
		return;
	}

	S2di(handle, "%.6f", value);
}

static void json_write_top_int_entry(fileHandle_t handle, const char *name, const ls_top_int_entry_t *entry,
		qbool *any)
{
	int i;

	if (!entry->has_value)
	{
		return;
	}

	if (*any)
	{
		S2di(handle, ",");
	}
	*any = true;

	S2di(handle, "\"%s\":[%d,[", name, entry->value);
	for (i = 0; i < entry->player_count; i++)
	{
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "%d", entry->players[i]);
	}
	S2di(handle, "]]");
}

static void json_write_top_float_entry(fileHandle_t handle, const char *name, const ls_top_float_entry_t *entry,
		qbool *any)
{
	int i;

	if (!entry->has_value)
	{
		return;
	}

	if (*any)
	{
		S2di(handle, ",");
	}
	*any = true;

	S2di(handle, "\"%s\":[%.6f,[", name, entry->value);
	for (i = 0; i < entry->player_count; i++)
	{
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "%d", entry->players[i]);
	}
	S2di(handle, "]]");
}

// JSON serialization for the versioned snapshot.
static void laststats_write_players(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"players\":[");
	for (i = 0; i < snap->player_count; i++)
	{
		const laststats_player_t *pl = &snap->players[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "{");
		S2di(handle, "\"name\":\"%s\",\"team\":\"%s\",\"ghost\":%d,\"bot\":%d,\"ct\":%d",
				json_string(pl->name),
				json_string(pl->team),
				pl->ghost ? 1 : 0,
				pl->bot ? 1 : 0,
				pl->ct);
		S2di(handle, ",\"frags\":%d,\"frags_raw\":%d,\"frag_diff\":%d,\"deaths\":%d",
				pl->frags, pl->frags_raw, pl->frag_diff, pl->deaths);
		S2di(handle, ",\"friendly\":%d,\"eff\":%.6f,\"spawn_frags\":%d,\"spree\":%d,\"spree_q\":%d",
				pl->friendly, pl->efficiency, pl->spawn_frags, pl->spree_max, pl->spree_max_q);
		S2di(handle, ",\"health\":%d,\"armor\":%d,\"armor_items\":%d,\"ot_h\":%d,\"ot_a\":%d,\"ot_items\":%d",
				pl->health, pl->armor, pl->armor_items, pl->ot_h, pl->ot_a, pl->ot_items);
		S2di(handle, ",\"speed_max\":%.6f,\"speed_avg\":%.6f", pl->speed_max, pl->speed_avg);
		S2di(handle, ",\"control_time\":%d,\"control_pct\":%d,\"quad_time\":%.6f",
				pl->control_time, pl->control_pct, pl->quad_time);
		S2di(handle, ",\"socd_perfect\":%d,\"socd_changes\":%d,\"socd_detection\":%d,\"socd_validation\":%d,"
				"\"socd_assist\":%d",
				pl->socd_perfect, pl->socd_changes, pl->socd_detection, pl->socd_validation,
				pl->socd_assisted ? 1 : 0);
		S2di(handle, ",\"ga\":%d,\"ya\":%d,\"ra\":%d,\"mh\":%d,\"quad\":%d,\"pent\":%d,\"ring\":%d",
				pl->ga, pl->ya, pl->ra, pl->mh, pl->quad, pl->pent, pl->ring);
		S2di(handle, ",\"quad_ekills\":%d,\"quad_tkills\":%d,\"ring_ekills\":%d,\"ring_tkills\":%d",
				pl->quad_ekills, pl->quad_tkills, pl->ring_ekills, pl->ring_tkills);
		S2di(handle, ",\"dmg_t\":%d,\"dmg_g\":%d,\"dmg_team\":%d,\"dmg_self\":%d,\"dmg_eweapon\":%d,"
				"\"dmg_todie\":%d",
				pl->dmg_t, pl->dmg_g, pl->dmg_team, pl->dmg_self, pl->dmg_eweapon, pl->dmg_todie);
		S2di(handle, ",\"rl_ad\":%.6f,\"rl_hits\":%d", pl->rl_ad, pl->rl_hits);
		S2di(handle, ",\"lg_pct\":%.6f,\"lg_hits\":%d,\"lg_att\":%d,\"lg_took\":%d,\"lg_ekill\":%d,"
				"\"lg_drop\":%d,\"lg_xfer\":%d",
				pl->lg_pct, pl->lg_hits, pl->lg_attacks, pl->lg_tooks, pl->lg_inv_ekills,
				pl->lg_drops, pl->lg_xfer);
		S2di(handle, ",\"rl_pct\":%.6f,\"rl_vhits\":%d,\"rl_att\":%d,\"rl_took\":%d,\"rl_ekill\":%d,"
				"\"rl_drop\":%d,\"rl_xfer\":%d",
				pl->rl_pct, pl->rl_vhits, pl->rl_attacks, pl->rl_tooks, pl->rl_inv_ekills,
				pl->rl_drops, pl->rl_xfer);
		S2di(handle, ",\"gl_pct\":%.6f,\"gl_hits\":%d", pl->gl_pct, pl->gl_hits);
		S2di(handle, ",\"sg_pct\":%.6f,\"sg_hits\":%d,\"sg_att\":%d,\"sg_dmg\":%d",
				pl->sg_pct, pl->sg_hits, pl->sg_attacks, pl->sg_damage);
		S2di(handle, ",\"ssg_pct\":%.6f,\"ssg_hits\":%d,\"ssg_att\":%d,\"ssg_dmg\":%d",
				pl->ssg_pct, pl->ssg_hits, pl->ssg_attacks, pl->ssg_damage);
		S2di(handle, ",\"axe_pct\":%.6f,\"axe_hits\":%d,\"axe_att\":%d",
				pl->axe_pct, pl->axe_hits, pl->axe_attacks);
		S2di(handle, ",\"ctf_pick\":%d,\"ctf_caps\":%d,\"ctf_ret\":%d,\"ctf_fdef\":%d,\"ctf_cdef\":%d,"
				"\"ctf_res\":%d,\"ctf_str\":%d,\"ctf_hst\":%d,\"ctf_rgn\":%d",
				pl->pickups, pl->caps, pl->returns, pl->f_defends, pl->c_defends,
				pl->res, pl->str, pl->hst, pl->rgn);
		S2di(handle, ",\"axe_ek\":%d,\"axe_tk\":%d,\"sg_ek\":%d,\"sg_tk\":%d,\"ssg_ek\":%d,\"ssg_tk\":%d,"
				"\"ng_ek\":%d,\"ng_tk\":%d,\"sng_ek\":%d,\"sng_tk\":%d,\"gl_ek\":%d,\"gl_tk\":%d,"
				"\"rl_ek\":%d,\"rl_tk\":%d,\"lg_ek\":%d,\"lg_tk\":%d,\"dc_ek\":%d,\"dc_tk\":%d",
				pl->axe_ekills, pl->axe_tkills, pl->sg_ekills, pl->sg_tkills, pl->ssg_ekills,
				pl->ssg_tkills, pl->ng_ekills, pl->ng_tkills, pl->sng_ekills, pl->sng_tkills,
				pl->gl_ekills, pl->gl_tkills, pl->rl_ekills, pl->rl_tkills, pl->lg_ekills,
				pl->lg_tkills, pl->discharge_ekills, pl->discharge_tkills);
		S2di(handle, ",\"lgc_over\":%d,\"lgc_under\":%d", pl->lgc_over, pl->lgc_under);
		S2di(handle, ",\"mid_total\":%d,\"mid_bronze\":%d,\"mid_silver\":%d,\"mid_gold\":%d,"
				"\"mid_plat\":%d,\"mid_stomps\":%d,\"mid_bonus\":%d,\"mid_maxh\":%.6f,\"mid_avgh\":%.6f",
				pl->mid_total, pl->mid_bronze, pl->mid_silver, pl->mid_gold, pl->mid_platinum,
				pl->mid_stomps, pl->mid_bonus, pl->mid_maxheight, pl->mid_avgheight);
		S2di(handle, ",\"i_cg\":%d,\"i_axe\":%d,\"i_stomp\":%d,\"i_multi\":%d,\"i_maxmulti\":%d,"
				"\"i_air\":%d,\"i_height\":%d,\"i_maxheight\":%d,\"i_rings\":%d",
				pl->i_cggibs, pl->i_axegibs, pl->i_stompgibs, pl->i_multigibs, pl->i_maxmultigibs,
				pl->i_airgibs, pl->i_height, pl->i_maxheight, pl->i_rings);
		S2di(handle, "}");
	}
	S2di(handle, "]");
}

static void laststats_write_kill(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"kill\":[");
	for (i = 0; i < snap->kill_count; i++)
	{
		const ls_kill_row_t *row = &snap->kill[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,", row->frags, row->kills, row->deaths, row->suicides);
		json_write_int_or_null(handle, row->friendly);
		S2di(handle, ",%d,%.6f]", row->spawn_frags, row->efficiency);
	}
	S2di(handle, "]");
}

static void laststats_write_items(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"items\":[");
	for (i = 0; i < snap->item_count; i++)
	{
		const ls_item_row_t *row = &snap->items[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,", row->ga, row->ya, row->ra, row->mh);
		json_write_int_or_null(handle, row->quad);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->pent);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->ring);
		S2di(handle, "]");
	}
	S2di(handle, "]");
}

static void laststats_write_wep_eff(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"wep_eff\":[");
	for (i = 0; i < snap->wep_eff_count; i++)
	{
		const ls_wep_eff_row_t *row = &snap->wep_eff[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[");
		json_write_float_or_null(handle, row->lg);
		S2di(handle, ",");
		json_write_float_or_null(handle, row->rl);
		S2di(handle, ",");
		json_write_float_or_null(handle, row->gl);
		S2di(handle, ",");
		json_write_float_or_null(handle, row->sng);
		S2di(handle, ",");
		json_write_float_or_null(handle, row->ng);
		S2di(handle, ",");
		json_write_float_or_null(handle, row->ssg);
		S2di(handle, ",");
		json_write_float_or_null(handle, row->sg);
		S2di(handle, "]");
	}
	S2di(handle, "]");
}

static void laststats_write_wep_dmg(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"wep_dmg\":[");
	for (i = 0; i < snap->wep_dmg_count; i++)
	{
		const ls_wep_dmg_row_t *row = &snap->wep_dmg[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,%d,%d]", row->lg, row->rl, row->gl, row->sng, row->ng,
				row->ssg, row->sg);
	}
	S2di(handle, "]");
}

static void laststats_write_wep_simple(fileHandle_t handle, const char *name, const ls_wep_simple_row_t *rows,
		int count)
{
	int i;

	S2di(handle, "\"%s\":[", name);
	for (i = 0; i < count; i++)
	{
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,%d]", rows[i].lg, rows[i].rl, rows[i].gl, rows[i].sng,
				rows[i].ng, rows[i].ssg);
	}
	S2di(handle, "]");
}

static void laststats_write_damage(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"damage\":[");
	for (i = 0; i < snap->damage_count; i++)
	{
		const ls_damage_row_t *row = &snap->damage[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,%d]", row->taken, row->given, row->eweapon, row->team, row->self,
				row->per_death);
	}
	S2di(handle, "]");
}

static void laststats_write_item_time(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"item_time\":[");
	for (i = 0; i < snap->item_time_count; i++)
	{
		const ls_item_time_row_t *row = &snap->item_time[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,", row->ra, row->ya, row->ga);
		json_write_int_or_null(handle, row->quad);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->pent);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->ring);
		S2di(handle, "]");
	}
	S2di(handle, "]");
}

static void laststats_write_wep_time(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"wep_time\":[");
	for (i = 0; i < snap->wep_time_count; i++)
	{
		const ls_wep_time_row_t *row = &snap->wep_time[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,%d]", row->lg, row->rl, row->gl, row->sng, row->ng, row->ssg);
	}
	S2di(handle, "]");
}

static void laststats_write_ctf(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"ctf\":[");
	for (i = 0; i < snap->ctf_count; i++)
	{
		const ls_ctf_row_t *row = &snap->ctf[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,", row->pickups, row->caps, row->returns, row->f_defends,
				row->c_defends);
		json_write_int_or_null(handle, row->res);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->str);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->hst);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->rgn);
		S2di(handle, "]");
	}
	S2di(handle, "]");
}

static void laststats_write_midair(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"midair\":[");
	for (i = 0; i < snap->midair_count; i++)
	{
		const ls_midair_row_t *row = &snap->midair[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,%d,%d,%.6f,%.6f]", row->total, row->bronze, row->silver, row->gold,
				row->platinum, row->stomps, row->bonus, row->max_height, row->avg_height);
	}
	S2di(handle, "]");
}

static void laststats_write_midair_kill(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"midair_kill\":[");
	for (i = 0; i < snap->midair_kill_count; i++)
	{
		const ls_midair_kill_row_t *row = &snap->midair_kill[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%.6f]", row->frags, row->spawn_frags, row->spree, row->rl_eff);
	}
	S2di(handle, "]");
}

static void laststats_write_insta(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"insta\":[");
	for (i = 0; i < snap->insta_count; i++)
	{
		const ls_insta_row_t *row = &snap->insta[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,%d,%d,%d,%d]", row->cggibs, row->axegibs, row->stompgibs,
				row->multigibs, row->maxmultigibs, row->airgibs, row->total_height, row->max_height,
				row->rings);
	}
	S2di(handle, "]");
}

static void laststats_write_insta_kill(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"insta_kill\":[");
	for (i = 0; i < snap->insta_kill_count; i++)
	{
		const ls_insta_kill_row_t *row = &snap->insta_kill[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%d,%d,%d,%d,", row->frags, row->kills, row->deaths, row->spawn_frags, row->spree);
		json_write_float_or_null(handle, row->sg_eff);
		S2di(handle, "]");
	}
	S2di(handle, "]");
}

static void laststats_write_lgc(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"lgc\":[");
	for (i = 0; i < snap->lgc_count; i++)
	{
		const ls_lgc_row_t *row = &snap->lgc[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[%d,%.6f,%.6f,%.6f]", row->score, row->over_pct, row->under_pct, row->eff_pct);
	}
	S2di(handle, "]");
}

static void laststats_write_team_items(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"team_items\":[");
	for (i = 0; i < snap->team_count; i++)
	{
		const ls_team_items_row_t *row = &snap->team_items[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[\"%s\",%d,%d,%d,%d,%d,%d,%d]", json_string(row->name), row->ra, row->ya, row->ga,
				row->mh, row->quad, row->pent, row->ring);
	}
	S2di(handle, "]");
}

static void laststats_write_team_weapons(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"team_weapons\":[");
	for (i = 0; i < snap->team_count; i++)
	{
		const ls_team_weapons_row_t *row = &snap->team_weapons[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[\"%s\",%d,%d,%d,%d,%d,%d,%d,%d]", json_string(row->name), row->lgt, row->lgk,
				row->lgd, row->lgx, row->rlt, row->rlk, row->rld, row->rlx);
	}
	S2di(handle, "]");
}

static void laststats_write_team_damage(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"team_damage\":[");
	for (i = 0; i < snap->team_count; i++)
	{
		const ls_team_damage_row_t *row = &snap->team_damage[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[\"%s\",%d,%d,%d,%d,%d]", json_string(row->name), row->frags, row->given, row->taken,
				row->eweapon, row->team);
	}
	S2di(handle, "]");
}

static void laststats_write_team_ctf(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	int i;

	S2di(handle, "\"team_ctf\":[");
	for (i = 0; i < snap->team_count; i++)
	{
		const ls_team_ctf_row_t *row = &snap->team_ctf[i];
		if (i)
		{
			S2di(handle, ",");
		}
		S2di(handle, "[\"%s\",%d,%d,%d,%d,%d,", json_string(row->name), row->pickups, row->caps, row->returns,
				row->f_defends, row->c_defends);
		json_write_int_or_null(handle, row->res);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->str);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->hst);
		S2di(handle, ",");
		json_write_int_or_null(handle, row->rgn);
		S2di(handle, "]");
	}
	S2di(handle, "]");
}

static void laststats_write_top(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	qbool any = false;

	S2di(handle, "\"top\":{");
	json_write_top_int_entry(handle, "frags", &snap->top.frags, &any);
	json_write_top_int_entry(handle, "deaths", &snap->top.deaths, &any);
	json_write_top_int_entry(handle, "friendkills", &snap->top.friendkills, &any);
	json_write_top_int_entry(handle, "efficiency", &snap->top.efficiency, &any);
	json_write_top_int_entry(handle, "fragstreak", &snap->top.fragstreak, &any);
	json_write_top_int_entry(handle, "quadrun", &snap->top.quadrun, &any);
	json_write_top_int_entry(handle, "annihilator", &snap->top.annihilator, &any);
	json_write_top_int_entry(handle, "survivor", &snap->top.survivor, &any);
	json_write_top_int_entry(handle, "spawnfrags", &snap->top.spawnfrags, &any);
	json_write_top_int_entry(handle, "captures", &snap->top.captures, &any);
	json_write_top_int_entry(handle, "flagdefends", &snap->top.flagdefends, &any);
	S2di(handle, "}");
}

static void laststats_write_top_midair(fileHandle_t handle, const laststats_snapshot_t *snap)
{
	qbool any = false;

	S2di(handle, "\"top_midair\":{");
	json_write_top_int_entry(handle, "score", &snap->top_midair.score, &any);
	json_write_top_int_entry(handle, "kills", &snap->top_midair.kills, &any);
	json_write_top_int_entry(handle, "midairs", &snap->top_midair.midairs, &any);
	json_write_top_int_entry(handle, "headstomps", &snap->top_midair.headstomps, &any);
	json_write_top_int_entry(handle, "streak", &snap->top_midair.streak, &any);
	json_write_top_int_entry(handle, "spawnfrags", &snap->top_midair.spawnfrags, &any);
	json_write_top_int_entry(handle, "bonus", &snap->top_midair.bonus, &any);
	json_write_top_float_entry(handle, "highest", &snap->top_midair.highest, &any);
	json_write_top_float_entry(handle, "avgheight", &snap->top_midair.avgheight, &any);
	json_write_top_float_entry(handle, "rleff", &snap->top_midair.rleff, &any);
	S2di(handle, "}");
}

// Minimal JSON reader for the laststats snapshot (no allocations).
typedef struct
{
	const char *cur;
	const char *end;
} json_reader_t;

static void json_skip_ws(json_reader_t *reader)
{
	while (reader->cur < reader->end)
	{
		char c = *reader->cur;
		if ((c != ' ') && (c != '\n') && (c != '\r') && (c != '\t'))
		{
			break;
		}
		reader->cur++;
	}
}

static qbool json_consume(json_reader_t *reader, char expected)
{
	json_skip_ws(reader);
	if ((reader->cur >= reader->end) || (*reader->cur != expected))
	{
		return false;
	}
	reader->cur++;
	return true;
}

static qbool json_match(json_reader_t *reader, const char *word)
{
	const char *start = reader->cur;

	while (*word)
	{
		if ((reader->cur >= reader->end) || (*reader->cur != *word))
		{
			reader->cur = start;
			return false;
		}
		reader->cur++;
		word++;
	}

	return true;
}

static qbool json_parse_string(json_reader_t *reader, char *out, int out_len)
{
	int len = 0;

	json_skip_ws(reader);
	if ((reader->cur >= reader->end) || (*reader->cur != '"'))
	{
		return false;
	}
	reader->cur++;

	while (reader->cur < reader->end)
	{
		char c = *reader->cur++;
		if (c == '"')
		{
			if (out_len > 0)
			{
				out[len < out_len ? len : out_len - 1] = '\0';
			}
			return true;
		}

		if (c == '\\')
		{
			if (reader->cur >= reader->end)
			{
				return false;
			}
			c = *reader->cur++;
			switch (c)
			{
				case '"':
				case '\\':
				case '/':
					break;
				case 'b':
					c = '\b';
					break;
				case 'f':
					c = '\f';
					break;
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				case 'u':
				{
					int i;
					int value = 0;
					for (i = 0; i < 4; i++)
					{
						if (reader->cur >= reader->end)
						{
							return false;
						}
						c = *reader->cur++;
						value <<= 4;
						if ((c >= '0') && (c <= '9'))
						{
							value += c - '0';
						}
						else if ((c >= 'a') && (c <= 'f'))
						{
							value += 10 + (c - 'a');
						}
						else if ((c >= 'A') && (c <= 'F'))
						{
							value += 10 + (c - 'A');
						}
						else
						{
							return false;
						}
					}
					c = (char)(value & 0xff);
					break;
				}
				default:
					return false;
			}
		}

		if (len + 1 < out_len)
		{
			out[len] = c;
		}
		len++;
	}

	return false;
}

static qbool json_parse_number(json_reader_t *reader, double *out)
{
	char buf[64];
	int len = 0;

	json_skip_ws(reader);
	if (reader->cur >= reader->end)
	{
		return false;
	}

	if ((*reader->cur == '-') || (*reader->cur == '+'))
	{
		if (len + 1 < (int)sizeof(buf))
		{
			buf[len++] = *reader->cur;
		}
		reader->cur++;
	}

	while (reader->cur < reader->end)
	{
		char c = *reader->cur;
		if ((c >= '0') && (c <= '9'))
		{
			if (len + 1 < (int)sizeof(buf))
			{
				buf[len++] = c;
			}
			reader->cur++;
			continue;
		}
		break;
	}

	if ((reader->cur < reader->end) && (*reader->cur == '.'))
	{
		if (len + 1 < (int)sizeof(buf))
		{
			buf[len++] = *reader->cur;
		}
		reader->cur++;
		while (reader->cur < reader->end)
		{
			char c = *reader->cur;
			if ((c >= '0') && (c <= '9'))
			{
				if (len + 1 < (int)sizeof(buf))
				{
					buf[len++] = c;
				}
				reader->cur++;
				continue;
			}
			break;
		}
	}

	if ((reader->cur < reader->end) && ((*reader->cur == 'e') || (*reader->cur == 'E')))
	{
		if (len + 1 < (int)sizeof(buf))
		{
			buf[len++] = *reader->cur;
		}
		reader->cur++;
		if ((reader->cur < reader->end) && ((*reader->cur == '-') || (*reader->cur == '+')))
		{
			if (len + 1 < (int)sizeof(buf))
			{
				buf[len++] = *reader->cur;
			}
			reader->cur++;
		}
		while (reader->cur < reader->end)
		{
			char c = *reader->cur;
			if ((c >= '0') && (c <= '9'))
			{
				if (len + 1 < (int)sizeof(buf))
				{
					buf[len++] = c;
				}
				reader->cur++;
				continue;
			}
			break;
		}
	}

	if (len == 0)
	{
		return false;
	}

	buf[len] = '\0';
	*out = atof(buf);
	return true;
}

static qbool json_parse_int(json_reader_t *reader, int *out)
{
	double value;
	if (!json_parse_number(reader, &value))
	{
		return false;
	}
	*out = (int)value;
	return true;
}

static qbool json_parse_float(json_reader_t *reader, float *out)
{
	double value;
	if (!json_parse_number(reader, &value))
	{
		return false;
	}
	*out = (float)value;
	return true;
}

static qbool json_parse_int_or_null(json_reader_t *reader, int *out, int null_value)
{
	json_skip_ws(reader);
	if (json_match(reader, "null"))
	{
		*out = null_value;
		return true;
	}
	return json_parse_int(reader, out);
}

static qbool json_parse_float_or_null(json_reader_t *reader, float *out, float null_value)
{
	json_skip_ws(reader);
	if (json_match(reader, "null"))
	{
		*out = null_value;
		return true;
	}
	return json_parse_float(reader, out);
}

static qbool json_skip_value(json_reader_t *reader)
{
	json_skip_ws(reader);
	if (reader->cur >= reader->end)
	{
		return false;
	}

	if (*reader->cur == '"')
	{
		char dummy[4];
		return json_parse_string(reader, dummy, sizeof(dummy));
	}

	if (*reader->cur == '{')
	{
		reader->cur++;
		if (json_consume(reader, '}'))
		{
			return true;
		}
		for (;;)
		{
			char key[64];
			if (!json_parse_string(reader, key, sizeof(key)))
			{
				return false;
			}
			if (!json_consume(reader, ':'))
			{
				return false;
			}
			if (!json_skip_value(reader))
			{
				return false;
			}
			if (json_consume(reader, ','))
			{
				continue;
			}
			return json_consume(reader, '}');
		}
	}

	if (*reader->cur == '[')
	{
		reader->cur++;
		if (json_consume(reader, ']'))
		{
			return true;
		}
		for (;;)
		{
			if (!json_skip_value(reader))
			{
				return false;
			}
			if (json_consume(reader, ','))
			{
				continue;
			}
			return json_consume(reader, ']');
		}
	}

	if (json_match(reader, "null") || json_match(reader, "true") || json_match(reader, "false"))
	{
		return true;
	}

	{
		double value;
		return json_parse_number(reader, &value);
	}
}

// Parse versioned player entries (v1 arrays or v2 objects).
static qbool json_parse_player_list(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		laststats_player_t *player;
		int ghost;

		if (snap->player_count >= MAX_CLIENTS)
		{
			return false;
		}

		player = &snap->players[snap->player_count];
		memset(player, 0, sizeof(*player));
		player->friendly = LS_INVALID_INT;

		if (json_consume(reader, '['))
		{
			if (!json_parse_string(reader, player->name, sizeof(player->name)))
			{
				return false;
			}
			if (!json_consume(reader, ','))
			{
				return false;
			}
			if (!json_parse_string(reader, player->team, sizeof(player->team)))
			{
				return false;
			}
			if (!json_consume(reader, ','))
			{
				return false;
			}
			if (!json_parse_int(reader, &ghost))
			{
				return false;
			}
			player->ghost = ghost ? true : false;
			if (!json_consume(reader, ']'))
			{
				return false;
			}
		}
		else if (json_consume(reader, '{'))
		{
			if (json_consume(reader, '}'))
			{
				return false;
			}
			for (;;)
			{
				char key[64];
				double number = 0.0;
				int value = 0;

				if (!json_parse_string(reader, key, sizeof(key)))
				{
					return false;
				}
				if (!json_consume(reader, ':'))
				{
					return false;
				}

				if (streq(key, "name"))
				{
					if (!json_parse_string(reader, player->name, sizeof(player->name)))
					{
						return false;
					}
				}
				else if (streq(key, "team"))
				{
					if (!json_parse_string(reader, player->team, sizeof(player->team)))
					{
						return false;
					}
				}
				else if (streq(key, "ghost"))
				{
					if (!json_parse_int(reader, &value))
					{
						return false;
					}
					player->ghost = value ? true : false;
				}
				else if (streq(key, "bot"))
				{
					if (!json_parse_int(reader, &value))
					{
						return false;
					}
					player->bot = value ? true : false;
				}
				else if (streq(key, "ct"))
				{
					if (!json_parse_int(reader, &player->ct))
					{
						return false;
					}
				}
				else if (streq(key, "frags"))
				{
					if (!json_parse_int(reader, &player->frags))
					{
						return false;
					}
				}
				else if (streq(key, "frags_raw"))
				{
					if (!json_parse_int(reader, &player->frags_raw))
					{
						return false;
					}
				}
				else if (streq(key, "frag_diff"))
				{
					if (!json_parse_int(reader, &player->frag_diff))
					{
						return false;
					}
				}
				else if (streq(key, "deaths"))
				{
					if (!json_parse_int(reader, &player->deaths))
					{
						return false;
					}
				}
				else if (streq(key, "friendly"))
				{
					if (!json_parse_int(reader, &player->friendly))
					{
						return false;
					}
				}
				else if (streq(key, "eff"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->efficiency = (float)number;
				}
				else if (streq(key, "spawn_frags"))
				{
					if (!json_parse_int(reader, &player->spawn_frags))
					{
						return false;
					}
				}
				else if (streq(key, "spree"))
				{
					if (!json_parse_int(reader, &player->spree_max))
					{
						return false;
					}
				}
				else if (streq(key, "spree_q"))
				{
					if (!json_parse_int(reader, &player->spree_max_q))
					{
						return false;
					}
				}
				else if (streq(key, "health"))
				{
					if (!json_parse_int(reader, &player->health))
					{
						return false;
					}
				}
				else if (streq(key, "armor"))
				{
					if (!json_parse_int(reader, &player->armor))
					{
						return false;
					}
				}
				else if (streq(key, "armor_items"))
				{
					if (!json_parse_int(reader, &player->armor_items))
					{
						return false;
					}
				}
				else if (streq(key, "ot_h"))
				{
					if (!json_parse_int(reader, &player->ot_h))
					{
						return false;
					}
				}
				else if (streq(key, "ot_a"))
				{
					if (!json_parse_int(reader, &player->ot_a))
					{
						return false;
					}
				}
				else if (streq(key, "ot_items"))
				{
					if (!json_parse_int(reader, &player->ot_items))
					{
						return false;
					}
				}
				else if (streq(key, "speed_max"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->speed_max = (float)number;
				}
				else if (streq(key, "speed_avg"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->speed_avg = (float)number;
				}
				else if (streq(key, "control_time"))
				{
					if (!json_parse_int(reader, &player->control_time))
					{
						return false;
					}
				}
				else if (streq(key, "control_pct"))
				{
					if (!json_parse_int(reader, &player->control_pct))
					{
						return false;
					}
				}
				else if (streq(key, "quad_time"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->quad_time = (float)number;
				}
				else if (streq(key, "socd_perfect"))
				{
					if (!json_parse_int(reader, &player->socd_perfect))
					{
						return false;
					}
				}
				else if (streq(key, "socd_changes"))
				{
					if (!json_parse_int(reader, &player->socd_changes))
					{
						return false;
					}
				}
				else if (streq(key, "socd_detection"))
				{
					if (!json_parse_int(reader, &player->socd_detection))
					{
						return false;
					}
				}
				else if (streq(key, "socd_validation"))
				{
					if (!json_parse_int(reader, &player->socd_validation))
					{
						return false;
					}
				}
				else if (streq(key, "socd_assist"))
				{
					if (!json_parse_int(reader, &value))
					{
						return false;
					}
					player->socd_assisted = value ? true : false;
				}
				else if (streq(key, "ga"))
				{
					if (!json_parse_int(reader, &player->ga))
					{
						return false;
					}
				}
				else if (streq(key, "ya"))
				{
					if (!json_parse_int(reader, &player->ya))
					{
						return false;
					}
				}
				else if (streq(key, "ra"))
				{
					if (!json_parse_int(reader, &player->ra))
					{
						return false;
					}
				}
				else if (streq(key, "mh"))
				{
					if (!json_parse_int(reader, &player->mh))
					{
						return false;
					}
				}
				else if (streq(key, "quad"))
				{
					if (!json_parse_int(reader, &player->quad))
					{
						return false;
					}
				}
				else if (streq(key, "pent"))
				{
					if (!json_parse_int(reader, &player->pent))
					{
						return false;
					}
				}
				else if (streq(key, "ring"))
				{
					if (!json_parse_int(reader, &player->ring))
					{
						return false;
					}
				}
				else if (streq(key, "quad_ekills"))
				{
					if (!json_parse_int(reader, &player->quad_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "quad_tkills"))
				{
					if (!json_parse_int(reader, &player->quad_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "ring_ekills"))
				{
					if (!json_parse_int(reader, &player->ring_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "ring_tkills"))
				{
					if (!json_parse_int(reader, &player->ring_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "dmg_t"))
				{
					if (!json_parse_int(reader, &player->dmg_t))
					{
						return false;
					}
				}
				else if (streq(key, "dmg_g"))
				{
					if (!json_parse_int(reader, &player->dmg_g))
					{
						return false;
					}
				}
				else if (streq(key, "dmg_team"))
				{
					if (!json_parse_int(reader, &player->dmg_team))
					{
						return false;
					}
				}
				else if (streq(key, "dmg_self"))
				{
					if (!json_parse_int(reader, &player->dmg_self))
					{
						return false;
					}
				}
				else if (streq(key, "dmg_eweapon"))
				{
					if (!json_parse_int(reader, &player->dmg_eweapon))
					{
						return false;
					}
				}
				else if (streq(key, "dmg_todie"))
				{
					if (!json_parse_int(reader, &player->dmg_todie))
					{
						return false;
					}
				}
				else if (streq(key, "rl_ad"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->rl_ad = (float)number;
				}
				else if (streq(key, "rl_hits"))
				{
					if (!json_parse_int(reader, &player->rl_hits))
					{
						return false;
					}
				}
				else if (streq(key, "lg_pct"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->lg_pct = (float)number;
				}
				else if (streq(key, "lg_hits"))
				{
					if (!json_parse_int(reader, &player->lg_hits))
					{
						return false;
					}
				}
				else if (streq(key, "lg_att"))
				{
					if (!json_parse_int(reader, &player->lg_attacks))
					{
						return false;
					}
				}
				else if (streq(key, "lg_took"))
				{
					if (!json_parse_int(reader, &player->lg_tooks))
					{
						return false;
					}
				}
				else if (streq(key, "lg_ekill"))
				{
					if (!json_parse_int(reader, &player->lg_inv_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "lg_drop"))
				{
					if (!json_parse_int(reader, &player->lg_drops))
					{
						return false;
					}
				}
				else if (streq(key, "lg_xfer"))
				{
					if (!json_parse_int(reader, &player->lg_xfer))
					{
						return false;
					}
				}
				else if (streq(key, "rl_pct"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->rl_pct = (float)number;
				}
				else if (streq(key, "rl_vhits"))
				{
					if (!json_parse_int(reader, &player->rl_vhits))
					{
						return false;
					}
				}
				else if (streq(key, "rl_att"))
				{
					if (!json_parse_int(reader, &player->rl_attacks))
					{
						return false;
					}
				}
				else if (streq(key, "rl_took"))
				{
					if (!json_parse_int(reader, &player->rl_tooks))
					{
						return false;
					}
				}
				else if (streq(key, "rl_ekill"))
				{
					if (!json_parse_int(reader, &player->rl_inv_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "rl_drop"))
				{
					if (!json_parse_int(reader, &player->rl_drops))
					{
						return false;
					}
				}
				else if (streq(key, "rl_xfer"))
				{
					if (!json_parse_int(reader, &player->rl_xfer))
					{
						return false;
					}
				}
				else if (streq(key, "gl_pct"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->gl_pct = (float)number;
				}
				else if (streq(key, "gl_hits"))
				{
					if (!json_parse_int(reader, &player->gl_hits))
					{
						return false;
					}
				}
				else if (streq(key, "sg_pct"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->sg_pct = (float)number;
				}
				else if (streq(key, "sg_hits"))
				{
					if (!json_parse_int(reader, &player->sg_hits))
					{
						return false;
					}
				}
				else if (streq(key, "sg_att"))
				{
					if (!json_parse_int(reader, &player->sg_attacks))
					{
						return false;
					}
				}
				else if (streq(key, "sg_dmg"))
				{
					if (!json_parse_int(reader, &player->sg_damage))
					{
						return false;
					}
				}
				else if (streq(key, "ssg_pct"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->ssg_pct = (float)number;
				}
				else if (streq(key, "ssg_hits"))
				{
					if (!json_parse_int(reader, &player->ssg_hits))
					{
						return false;
					}
				}
				else if (streq(key, "ssg_att"))
				{
					if (!json_parse_int(reader, &player->ssg_attacks))
					{
						return false;
					}
				}
				else if (streq(key, "ssg_dmg"))
				{
					if (!json_parse_int(reader, &player->ssg_damage))
					{
						return false;
					}
				}
				else if (streq(key, "axe_pct"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->axe_pct = (float)number;
				}
				else if (streq(key, "axe_hits"))
				{
					if (!json_parse_int(reader, &player->axe_hits))
					{
						return false;
					}
				}
				else if (streq(key, "axe_att"))
				{
					if (!json_parse_int(reader, &player->axe_attacks))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_pick"))
				{
					if (!json_parse_int(reader, &player->pickups))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_caps"))
				{
					if (!json_parse_int(reader, &player->caps))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_ret"))
				{
					if (!json_parse_int(reader, &player->returns))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_fdef"))
				{
					if (!json_parse_int(reader, &player->f_defends))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_cdef"))
				{
					if (!json_parse_int(reader, &player->c_defends))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_res"))
				{
					if (!json_parse_int(reader, &player->res))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_str"))
				{
					if (!json_parse_int(reader, &player->str))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_hst"))
				{
					if (!json_parse_int(reader, &player->hst))
					{
						return false;
					}
				}
				else if (streq(key, "ctf_rgn"))
				{
					if (!json_parse_int(reader, &player->rgn))
					{
						return false;
					}
				}
				else if (streq(key, "axe_ek"))
				{
					if (!json_parse_int(reader, &player->axe_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "axe_tk"))
				{
					if (!json_parse_int(reader, &player->axe_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "sg_ek"))
				{
					if (!json_parse_int(reader, &player->sg_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "sg_tk"))
				{
					if (!json_parse_int(reader, &player->sg_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "ssg_ek"))
				{
					if (!json_parse_int(reader, &player->ssg_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "ssg_tk"))
				{
					if (!json_parse_int(reader, &player->ssg_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "ng_ek"))
				{
					if (!json_parse_int(reader, &player->ng_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "ng_tk"))
				{
					if (!json_parse_int(reader, &player->ng_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "sng_ek"))
				{
					if (!json_parse_int(reader, &player->sng_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "sng_tk"))
				{
					if (!json_parse_int(reader, &player->sng_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "gl_ek"))
				{
					if (!json_parse_int(reader, &player->gl_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "gl_tk"))
				{
					if (!json_parse_int(reader, &player->gl_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "rl_ek"))
				{
					if (!json_parse_int(reader, &player->rl_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "rl_tk"))
				{
					if (!json_parse_int(reader, &player->rl_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "lg_ek"))
				{
					if (!json_parse_int(reader, &player->lg_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "lg_tk"))
				{
					if (!json_parse_int(reader, &player->lg_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "dc_ek"))
				{
					if (!json_parse_int(reader, &player->discharge_ekills))
					{
						return false;
					}
				}
				else if (streq(key, "dc_tk"))
				{
					if (!json_parse_int(reader, &player->discharge_tkills))
					{
						return false;
					}
				}
				else if (streq(key, "lgc_over"))
				{
					if (!json_parse_int(reader, &player->lgc_over))
					{
						return false;
					}
				}
				else if (streq(key, "lgc_under"))
				{
					if (!json_parse_int(reader, &player->lgc_under))
					{
						return false;
					}
				}
				else if (streq(key, "mid_total"))
				{
					if (!json_parse_int(reader, &player->mid_total))
					{
						return false;
					}
				}
				else if (streq(key, "mid_bronze"))
				{
					if (!json_parse_int(reader, &player->mid_bronze))
					{
						return false;
					}
				}
				else if (streq(key, "mid_silver"))
				{
					if (!json_parse_int(reader, &player->mid_silver))
					{
						return false;
					}
				}
				else if (streq(key, "mid_gold"))
				{
					if (!json_parse_int(reader, &player->mid_gold))
					{
						return false;
					}
				}
				else if (streq(key, "mid_plat"))
				{
					if (!json_parse_int(reader, &player->mid_platinum))
					{
						return false;
					}
				}
				else if (streq(key, "mid_stomps"))
				{
					if (!json_parse_int(reader, &player->mid_stomps))
					{
						return false;
					}
				}
				else if (streq(key, "mid_bonus"))
				{
					if (!json_parse_int(reader, &player->mid_bonus))
					{
						return false;
					}
				}
				else if (streq(key, "mid_maxh"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->mid_maxheight = (float)number;
				}
				else if (streq(key, "mid_avgh"))
				{
					if (!json_parse_number(reader, &number))
					{
						return false;
					}
					player->mid_avgheight = (float)number;
				}
				else if (streq(key, "i_cg"))
				{
					if (!json_parse_int(reader, &player->i_cggibs))
					{
						return false;
					}
				}
				else if (streq(key, "i_axe"))
				{
					if (!json_parse_int(reader, &player->i_axegibs))
					{
						return false;
					}
				}
				else if (streq(key, "i_stomp"))
				{
					if (!json_parse_int(reader, &player->i_stompgibs))
					{
						return false;
					}
				}
				else if (streq(key, "i_multi"))
				{
					if (!json_parse_int(reader, &player->i_multigibs))
					{
						return false;
					}
				}
				else if (streq(key, "i_maxmulti"))
				{
					if (!json_parse_int(reader, &player->i_maxmultigibs))
					{
						return false;
					}
				}
				else if (streq(key, "i_air"))
				{
					if (!json_parse_int(reader, &player->i_airgibs))
					{
						return false;
					}
				}
				else if (streq(key, "i_height"))
				{
					if (!json_parse_int(reader, &player->i_height))
					{
						return false;
					}
				}
				else if (streq(key, "i_maxheight"))
				{
					if (!json_parse_int(reader, &player->i_maxheight))
					{
						return false;
					}
				}
				else if (streq(key, "i_rings"))
				{
					if (!json_parse_int(reader, &player->i_rings))
					{
						return false;
					}
				}
				else
				{
					if (!json_skip_value(reader))
					{
						return false;
					}
				}

				if (json_consume(reader, ','))
				{
					continue;
				}
				if (!json_consume(reader, '}'))
				{
					return false;
				}
				break;
			}
		}
		else
		{
			return false;
		}

		snap->player_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_kill_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_kill_row_t *row;

		if (snap->kill_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->kill[snap->kill_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->frags)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->kills)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->deaths)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->suicides)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->friendly, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->spawn_frags)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->efficiency))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->kill_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_items_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_item_row_t *row;

		if (snap->item_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->items[snap->item_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->ga)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ya)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ra)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->mh)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->quad, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->pent, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->ring, LS_INVALID_INT))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->item_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_wep_eff_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_wep_eff_row_t *row;

		if (snap->wep_eff_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->wep_eff[snap->wep_eff_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_float_or_null(reader, &row->lg, LS_INVALID_FLOAT)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->rl, LS_INVALID_FLOAT)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->gl, LS_INVALID_FLOAT)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->sng, LS_INVALID_FLOAT)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->ng, LS_INVALID_FLOAT)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->ssg, LS_INVALID_FLOAT)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->sg, LS_INVALID_FLOAT))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->wep_eff_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_wep_dmg_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_wep_dmg_row_t *row;

		if (snap->wep_dmg_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->wep_dmg[snap->wep_dmg_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->lg)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rl)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->gl)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->sng)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ng)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ssg)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->sg))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->wep_dmg_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_wep_simple_table(json_reader_t *reader, ls_wep_simple_row_t *rows, int *count)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_wep_simple_row_t *row;

		if (*count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &rows[*count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->lg)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rl)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->gl)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->sng)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ng)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ssg))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		(*count)++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_damage_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_damage_row_t *row;

		if (snap->damage_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->damage[snap->damage_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->taken)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->given)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->eweapon)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->team)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->self)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->per_death))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->damage_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_item_time_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_item_time_row_t *row;

		if (snap->item_time_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->item_time[snap->item_time_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->ra)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ya)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ga)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->quad, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->pent, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->ring, LS_INVALID_INT))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->item_time_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_wep_time_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_wep_time_row_t *row;

		if (snap->wep_time_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->wep_time[snap->wep_time_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->lg)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rl)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->gl)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->sng)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ng)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ssg))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->wep_time_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_ctf_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_ctf_row_t *row;

		if (snap->ctf_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->ctf[snap->ctf_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->pickups)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->caps)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->returns)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->f_defends)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->c_defends)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->res, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->str, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->hst, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->rgn, LS_INVALID_INT))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->ctf_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_midair_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_midair_row_t *row;

		if (snap->midair_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->midair[snap->midair_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->total)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->bronze)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->silver)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->gold)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->platinum)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->stomps)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->bonus)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->max_height)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->avg_height))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->midair_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_midair_kill_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_midair_kill_row_t *row;

		if (snap->midair_kill_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->midair_kill[snap->midair_kill_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->frags)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->spawn_frags)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->spree)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->rl_eff))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->midair_kill_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_insta_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_insta_row_t *row;

		if (snap->insta_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->insta[snap->insta_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->cggibs)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->axegibs)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->stompgibs)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->multigibs)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->maxmultigibs)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->airgibs)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->total_height)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->max_height)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rings))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->insta_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_insta_kill_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_insta_kill_row_t *row;

		if (snap->insta_kill_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->insta_kill[snap->insta_kill_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->frags)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->kills)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->deaths)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->spawn_frags)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->spree)
				|| !json_consume(reader, ',')
				|| !json_parse_float_or_null(reader, &row->sg_eff, LS_INVALID_FLOAT))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->insta_kill_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_lgc_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_lgc_row_t *row;

		if (snap->lgc_count >= MAX_CLIENTS)
		{
			return false;
		}

		row = &snap->lgc[snap->lgc_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_int(reader, &row->score)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->over_pct)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->under_pct)
				|| !json_consume(reader, ',')
				|| !json_parse_float(reader, &row->eff_pct))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->lgc_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_team_items_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_team_items_row_t *row;

		if (snap->team_count >= MAX_TM_STATS)
		{
			return false;
		}

		row = &snap->team_items[snap->team_count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_string(reader, row->name, sizeof(row->name))
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ra)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ya)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ga)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->mh)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->quad)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->pent)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->ring))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		snap->team_count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, ']');
	}
}

static qbool json_parse_team_weapons_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	int count = 0;

	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_team_weapons_row_t *row;

		if (count >= MAX_TM_STATS)
		{
			return false;
		}

		row = &snap->team_weapons[count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_string(reader, row->name, sizeof(row->name))
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->lgt)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->lgk)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->lgd)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->lgx)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rlt)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rlk)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rld)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->rlx))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		break;
	}

	if (snap->team_count == 0)
	{
		snap->team_count = count;
	}
	return json_consume(reader, ']');
}

static qbool json_parse_team_damage_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	int count = 0;

	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_team_damage_row_t *row;

		if (count >= MAX_TM_STATS)
		{
			return false;
		}

		row = &snap->team_damage[count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_string(reader, row->name, sizeof(row->name))
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->frags)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->given)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->taken)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->eweapon)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->team))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		break;
	}

	if (snap->team_count == 0)
	{
		snap->team_count = count;
	}
	return json_consume(reader, ']');
}

static qbool json_parse_team_ctf_table(json_reader_t *reader, laststats_snapshot_t *snap)
{
	int count = 0;

	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (json_consume(reader, ']'))
	{
		return true;
	}

	for (;;)
	{
		ls_team_ctf_row_t *row;

		if (count >= MAX_TM_STATS)
		{
			return false;
		}

		row = &snap->team_ctf[count];
		if (!json_consume(reader, '['))
		{
			return false;
		}
		if (!json_parse_string(reader, row->name, sizeof(row->name))
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->pickups)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->caps)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->returns)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->f_defends)
				|| !json_consume(reader, ',')
				|| !json_parse_int(reader, &row->c_defends)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->res, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->str, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->hst, LS_INVALID_INT)
				|| !json_consume(reader, ',')
				|| !json_parse_int_or_null(reader, &row->rgn, LS_INVALID_INT))
		{
			return false;
		}
		if (!json_consume(reader, ']'))
		{
			return false;
		}

		count++;
		if (json_consume(reader, ','))
		{
			continue;
		}
		break;
	}

	if (snap->team_count == 0)
	{
		snap->team_count = count;
	}
	return json_consume(reader, ']');
}

static qbool json_parse_top_int_entry(json_reader_t *reader, ls_top_int_entry_t *entry)
{
	int count = 0;

	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (!json_parse_int(reader, &entry->value))
	{
		return false;
	}
	if (!json_consume(reader, ','))
	{
		return false;
	}
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (!json_consume(reader, ']'))
	{
		for (;;)
		{
			if (count >= MAX_CLIENTS)
			{
				return false;
			}
			if (!json_parse_int(reader, &entry->players[count]))
			{
				return false;
			}
			count++;
			if (json_consume(reader, ','))
			{
				continue;
			}
			if (!json_consume(reader, ']'))
			{
				return false;
			}
			break;
		}
	}
	if (!json_consume(reader, ']'))
	{
		return false;
	}

	entry->player_count = count;
	entry->has_value = true;
	return true;
}

static qbool json_parse_top_float_entry(json_reader_t *reader, ls_top_float_entry_t *entry)
{
	int count = 0;

	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (!json_parse_float(reader, &entry->value))
	{
		return false;
	}
	if (!json_consume(reader, ','))
	{
		return false;
	}
	if (!json_consume(reader, '['))
	{
		return false;
	}
	if (!json_consume(reader, ']'))
	{
		for (;;)
		{
			if (count >= MAX_CLIENTS)
			{
				return false;
			}
			if (!json_parse_int(reader, &entry->players[count]))
			{
				return false;
			}
			count++;
			if (json_consume(reader, ','))
			{
				continue;
			}
			if (!json_consume(reader, ']'))
			{
				return false;
			}
			break;
		}
	}
	if (!json_consume(reader, ']'))
	{
		return false;
	}

	entry->player_count = count;
	entry->has_value = true;
	return true;
}

static qbool json_parse_top_object(json_reader_t *reader, ls_top_stats_t *top)
{
	if (!json_consume(reader, '{'))
	{
		return false;
	}
	if (json_consume(reader, '}'))
	{
		return true;
	}

	for (;;)
	{
		char key[64];
		if (!json_parse_string(reader, key, sizeof(key)))
		{
			return false;
		}
		if (!json_consume(reader, ':'))
		{
			return false;
		}

		if (streq(key, "frags"))
		{
			if (!json_parse_top_int_entry(reader, &top->frags))
			{
				return false;
			}
		}
		else if (streq(key, "deaths"))
		{
			if (!json_parse_top_int_entry(reader, &top->deaths))
			{
				return false;
			}
		}
		else if (streq(key, "friendkills"))
		{
			if (!json_parse_top_int_entry(reader, &top->friendkills))
			{
				return false;
			}
		}
		else if (streq(key, "efficiency"))
		{
			if (!json_parse_top_int_entry(reader, &top->efficiency))
			{
				return false;
			}
		}
		else if (streq(key, "fragstreak"))
		{
			if (!json_parse_top_int_entry(reader, &top->fragstreak))
			{
				return false;
			}
		}
		else if (streq(key, "quadrun"))
		{
			if (!json_parse_top_int_entry(reader, &top->quadrun))
			{
				return false;
			}
		}
		else if (streq(key, "annihilator"))
		{
			if (!json_parse_top_int_entry(reader, &top->annihilator))
			{
				return false;
			}
		}
		else if (streq(key, "survivor"))
		{
			if (!json_parse_top_int_entry(reader, &top->survivor))
			{
				return false;
			}
		}
		else if (streq(key, "spawnfrags"))
		{
			if (!json_parse_top_int_entry(reader, &top->spawnfrags))
			{
				return false;
			}
		}
		else if (streq(key, "captures"))
		{
			if (!json_parse_top_int_entry(reader, &top->captures))
			{
				return false;
			}
		}
		else if (streq(key, "flagdefends"))
		{
			if (!json_parse_top_int_entry(reader, &top->flagdefends))
			{
				return false;
			}
		}
		else
		{
			if (!json_skip_value(reader))
			{
				return false;
			}
		}

		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, '}');
	}
}

static qbool json_parse_top_midair_object(json_reader_t *reader, ls_top_midair_t *top)
{
	if (!json_consume(reader, '{'))
	{
		return false;
	}
	if (json_consume(reader, '}'))
	{
		return true;
	}

	for (;;)
	{
		char key[64];
		if (!json_parse_string(reader, key, sizeof(key)))
		{
			return false;
		}
		if (!json_consume(reader, ':'))
		{
			return false;
		}

		if (streq(key, "score"))
		{
			if (!json_parse_top_int_entry(reader, &top->score))
			{
				return false;
			}
		}
		else if (streq(key, "kills"))
		{
			if (!json_parse_top_int_entry(reader, &top->kills))
			{
				return false;
			}
		}
		else if (streq(key, "midairs"))
		{
			if (!json_parse_top_int_entry(reader, &top->midairs))
			{
				return false;
			}
		}
		else if (streq(key, "headstomps"))
		{
			if (!json_parse_top_int_entry(reader, &top->headstomps))
			{
				return false;
			}
		}
		else if (streq(key, "streak"))
		{
			if (!json_parse_top_int_entry(reader, &top->streak))
			{
				return false;
			}
		}
		else if (streq(key, "spawnfrags"))
		{
			if (!json_parse_top_int_entry(reader, &top->spawnfrags))
			{
				return false;
			}
		}
		else if (streq(key, "bonus"))
		{
			if (!json_parse_top_int_entry(reader, &top->bonus))
			{
				return false;
			}
		}
		else if (streq(key, "highest"))
		{
			if (!json_parse_top_float_entry(reader, &top->highest))
			{
				return false;
			}
		}
		else if (streq(key, "avgheight"))
		{
			if (!json_parse_top_float_entry(reader, &top->avgheight))
			{
				return false;
			}
		}
		else if (streq(key, "rleff"))
		{
			if (!json_parse_top_float_entry(reader, &top->rleff))
			{
				return false;
			}
		}
		else
		{
			if (!json_skip_value(reader))
			{
				return false;
			}
		}

		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, '}');
	}
}

static qbool json_parse_flags(json_reader_t *reader, laststats_flags_t *flags)
{
	if (!json_consume(reader, '{'))
	{
		return false;
	}
	if (json_consume(reader, '}'))
	{
		return true;
	}

	for (;;)
	{
		char key[64];
		int value = 0;

		if (!json_parse_string(reader, key, sizeof(key)))
		{
			return false;
		}
		if (!json_consume(reader, ':'))
		{
			return false;
		}
		if (!json_parse_int(reader, &value))
		{
			return false;
		}

		if (streq(key, "midair"))
		{
			flags->midair = value ? true : false;
		}
		else if (streq(key, "instagib"))
		{
			flags->instagib = value ? true : false;
		}
		else if (streq(key, "lgc"))
		{
			flags->lgc = value ? true : false;
		}
		else if (streq(key, "ctf"))
		{
			flags->ctf = value ? true : false;
		}
		else if (streq(key, "team"))
		{
			flags->team = value ? true : false;
		}
		else if (streq(key, "duel"))
		{
			flags->duel = value ? true : false;
		}
		else if (streq(key, "ctf_runes"))
		{
			flags->ctf_runes = value ? true : false;
		}
		else if (streq(key, "ra"))
		{
			flags->ra = value ? true : false;
		}
		else if (streq(key, "ca"))
		{
			flags->ca = value ? true : false;
		}
		else if (streq(key, "race"))
		{
			flags->race = value ? true : false;
		}
		else if (streq(key, "socd"))
		{
			flags->socd = value ? true : false;
		}
		else if (streq(key, "time_valid"))
		{
			flags->time_valid = value ? true : false;
		}
		else if (streq(key, "overtime"))
		{
			flags->overtime = value ? true : false;
		}
		else if (streq(key, "deathmatch"))
		{
			flags->deathmatch = value;
		}

		if (json_consume(reader, ','))
		{
			continue;
		}
		return json_consume(reader, '}');
	}
}

static qbool laststats_json_parse(const char *text, laststats_snapshot_t *snap)
{
	json_reader_t reader;

	reader.cur = text;
	reader.end = text + strlen(text);

	if (!json_consume(&reader, '{'))
	{
		return false;
	}
	if (json_consume(&reader, '}'))
	{
		return true;
	}

	for (;;)
	{
		char key[64];
		if (!json_parse_string(&reader, key, sizeof(key)))
		{
			return false;
		}
		if (!json_consume(&reader, ':'))
		{
			return false;
		}

		if (streq(key, "version"))
		{
			if (!json_parse_int(&reader, &snap->version))
			{
				return false;
			}
		}
		else if (streq(key, "flags"))
		{
			if (!json_parse_flags(&reader, &snap->flags))
			{
				return false;
			}
		}
		else if (streq(key, "players"))
		{
			if (!json_parse_player_list(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "ca_team1_name"))
		{
			if (!json_parse_string(&reader, snap->ca_team1_name, sizeof(snap->ca_team1_name)))
			{
				return false;
			}
		}
		else if (streq(key, "ca_team1_score"))
		{
			if (!json_parse_int(&reader, &snap->ca_team1_score))
			{
				return false;
			}
		}
		else if (streq(key, "ca_team2_name"))
		{
			if (!json_parse_string(&reader, snap->ca_team2_name, sizeof(snap->ca_team2_name)))
			{
				return false;
			}
		}
		else if (streq(key, "ca_team2_score"))
		{
			if (!json_parse_int(&reader, &snap->ca_team2_score))
			{
				return false;
			}
		}
		else if (streq(key, "kill"))
		{
			if (!json_parse_kill_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "items"))
		{
			if (!json_parse_items_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "wep_eff"))
		{
			if (!json_parse_wep_eff_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "wep_dmg"))
		{
			if (!json_parse_wep_dmg_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "wep_taken"))
		{
			if (!json_parse_wep_simple_table(&reader, snap->wep_taken, &snap->wep_taken_count))
			{
				return false;
			}
		}
		else if (streq(key, "wep_drop"))
		{
			if (!json_parse_wep_simple_table(&reader, snap->wep_drop, &snap->wep_drop_count))
			{
				return false;
			}
		}
		else if (streq(key, "wep_kill"))
		{
			if (!json_parse_wep_simple_table(&reader, snap->wep_kill, &snap->wep_kill_count))
			{
				return false;
			}
		}
		else if (streq(key, "wep_ekill"))
		{
			if (!json_parse_wep_simple_table(&reader, snap->wep_ekill, &snap->wep_ekill_count))
			{
				return false;
			}
		}
		else if (streq(key, "damage"))
		{
			if (!json_parse_damage_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "item_time"))
		{
			if (!json_parse_item_time_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "wep_time"))
		{
			if (!json_parse_wep_time_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "ctf"))
		{
			if (!json_parse_ctf_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "midair"))
		{
			if (!json_parse_midair_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "midair_kill"))
		{
			if (!json_parse_midair_kill_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "insta"))
		{
			if (!json_parse_insta_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "insta_kill"))
		{
			if (!json_parse_insta_kill_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "lgc"))
		{
			if (!json_parse_lgc_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "team_items"))
		{
			if (!json_parse_team_items_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "team_weapons"))
		{
			if (!json_parse_team_weapons_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "team_damage"))
		{
			if (!json_parse_team_damage_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "team_ctf"))
		{
			if (!json_parse_team_ctf_table(&reader, snap))
			{
				return false;
			}
		}
		else if (streq(key, "top"))
		{
			if (!json_parse_top_object(&reader, &snap->top))
			{
				return false;
			}
		}
		else if (streq(key, "top_midair"))
		{
			if (!json_parse_top_midair_object(&reader, &snap->top_midair))
			{
				return false;
			}
		}
		else
		{
			if (!json_skip_value(&reader))
			{
				return false;
			}
		}

		if (json_consume(&reader, ','))
		{
			continue;
		}
		return json_consume(&reader, '}');
	}
}

// Load laststats.json into a snapshot struct.
static qbool laststats_json_load(laststats_snapshot_t *snap)
{
	fileHandle_t handle;
	static char buffer[LS_JSON_MAX];
	int len = 0;
	int c;

	handle = std_fropen(LASTSTATS_FILENAME);
	if (handle < 0)
	{
		return false;
	}

	while ((c = std_fgetc(handle)) != -1)
	{
		if (len + 1 >= (int)sizeof(buffer))
		{
			std_fclose(handle);
			return false;
		}
		buffer[len++] = (char)c;
	}
	buffer[len] = '\0';
	std_fclose(handle);

	if (len == 0)
	{
		return false;
	}

	memset(snap, 0, sizeof(*snap));
	return laststats_json_parse(buffer, snap);
}

static qbool laststats_snapshot_valid(const laststats_snapshot_t *snap)
{
	if ((snap->version != 1) && (snap->version != LASTSTATS_JSON_VERSION))
	{
		return false;
	}

	if (snap->player_count <= 0)
	{
		return false;
	}

	if (snap->version == LASTSTATS_JSON_VERSION)
	{
		return true;
	}

	if (snap->flags.midair)
	{
		return (snap->midair_count == snap->player_count)
				&& (snap->midair_kill_count == snap->player_count);
	}
	if (snap->flags.instagib)
	{
		return (snap->insta_count == snap->player_count)
				&& (snap->insta_kill_count == snap->player_count);
	}
	if (snap->flags.lgc)
	{
		return snap->lgc_count == snap->player_count;
	}

	if ((snap->kill_count != snap->player_count)
			|| (snap->item_count != snap->player_count)
			|| (snap->wep_eff_count != snap->player_count)
			|| (snap->wep_dmg_count != snap->player_count)
			|| (snap->wep_taken_count != snap->player_count)
			|| (snap->wep_drop_count != snap->player_count)
			|| (snap->wep_kill_count != snap->player_count)
			|| (snap->wep_ekill_count != snap->player_count)
			|| (snap->damage_count != snap->player_count)
			|| (snap->item_time_count != snap->player_count)
			|| (snap->wep_time_count != snap->player_count))
	{
		return false;
	}

	if (snap->flags.ctf && (snap->ctf_count != snap->player_count))
	{
		return false;
	}

	return true;
}

// Formatting helpers for the /laststats output.
static const char* laststats_player_name(const laststats_snapshot_t *snap, int index)
{
	return va("%s%s", snap->players[index].ghost ? "\203" : "", snap->players[index].name);
}

static const char* laststats_format_efficiency(float efficiency)
{
	if (efficiency >= 100.0f)
	{
		return va("%.0f%%", efficiency);
	}
	if (efficiency == 0.0f)
	{
		return va(" %.1f%%", efficiency);
	}
	return va("%.1f%%", efficiency);
}

static const char* laststats_format_percent(float value)
{
	if (value >= 100.0f)
	{
		return va("%.0f%%", value);
	}
	return va("%.1f%%", value);
}

static const char* laststats_format_percent_exact(float value)
{
	if (value == 100.0f)
	{
		return va("%.0f%%", value);
	}
	return va("%.1f%%", value);
}

static const char* laststats_format_time(int seconds)
{
	if (seconds == LS_INVALID_INT)
	{
		return "    -";
	}
	return va("%.2d:%.2d", seconds / 60, seconds % 60);
}

static const char* laststats_format_rune(int value)
{
	if (value == LS_INVALID_INT)
	{
		return " - ";
	}
	return va("%d%%", value);
}

// Per-player output sections (mode-specific).
static void laststats_print_player_midair_stats(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const laststats_player_t *p = &snap->players[index];
	const char *rl_eff = p->rl_pct ? va("%.1f%%", p->rl_pct) : "  0.0%";

	G_sprint(ed, 2, "\207 %s: %d\n", laststats_player_name(snap, index), p->frags_raw);
	G_sprint(ed, 2, "   %-13s: %d\n", redtext("total midairs"), p->mid_total);
	G_sprint(ed, 2, "    %12s: %d\n", "bronze", p->mid_bronze);
	G_sprint(ed, 2, "    %12s: %d\n", "silver", p->mid_silver);
	G_sprint(ed, 2, "    %12s: %d\n", "gold", p->mid_gold);
	G_sprint(ed, 2, "    %12s: %d\n", "platinum", p->mid_platinum);
	G_sprint(ed, 2, "   %-13s: %d\n", redtext("stomps"), p->mid_stomps);
	G_sprint(ed, 2, "   %-13s: %d\n", redtext("streak"), p->spree_max);
	G_sprint(ed, 2, "   %-13s: %d\n", redtext("spawnfrags"), p->spawn_frags);
	G_sprint(ed, 2, "   %-13s: %d\n", redtext("bonuses"), p->mid_bonus);
	G_sprint(ed, 2, "   %-13s: %.1f\n", redtext("max height"), p->mid_maxheight);
	G_sprint(ed, 2, "   %-13s: %.1f\n", redtext("avg height"), p->mid_avgheight);
	G_sprint(ed, 2, "   %-13s: %s\n", redtext("rl efficiency"), rl_eff);
	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

static void laststats_print_player_instagib_stats(const laststats_snapshot_t *snap, gedict_t *ed, int index, qbool tp)
{
	const laststats_player_t *p = &snap->players[index];
	int frags = p->i_cggibs + p->i_axegibs + p->i_stompgibs;
	float avg_height = p->i_airgibs ? (float)p->i_height / p->i_airgibs : 0.0f;

	G_sprint(ed, 2, "\n\207 %s: %s \207\n", "PLAYER", laststats_player_name(snap, index));
	G_sprint(ed, 2, " \220%s\221\n", "SCORES");
	G_sprint(ed, 2, "  %s: %.1f\n", redtext("Efficiency"), p->efficiency);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Points"), p->frags_raw);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Frags"), frags);
	if (tp)
	{
		G_sprint(ed, 2, "  %s: %d\n", redtext("Teamkills"), p->friendly);
	}
	G_sprint(ed, 2, "  %s: %d\n", redtext("Deaths"), p->deaths);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Streaks"), p->spree_max);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Spawns"), p->spawn_frags);
	G_sprint(ed, 2, " \220%s\221\n", "SPEED");
	G_sprint(ed, 2, "  %s: %.1f\n", redtext("Maximum"), p->speed_max);
	G_sprint(ed, 2, "  %s: %.1f\n", redtext("Average"), p->speed_avg);
	G_sprint(ed, 2, " \220%s\221\n", "WEAPONS");
	G_sprint(ed, 2, "  %s: %s", redtext("Coilgun"),
			(p->sg_attacks ? va("%.1f%% (%d)", p->sg_pct, p->i_cggibs) : ""));
	G_sprint(ed, 2, "%s\n", (p->sg_attacks ? "" : "n/u"));
	G_sprint(ed, 2, "  %s: %s", redtext("Axe"),
			(p->axe_attacks ? va("%.1f%% (%d)", p->axe_pct, p->i_axegibs) : ""));
	G_sprint(ed, 2, "%s\n", (p->axe_attacks ? "" : "n/u"));
	G_sprint(ed, 2, " \220%s\221\n", "GIBS");
	G_sprint(ed, 2, "  %s: %d\n", redtext("Coilgun"), p->i_cggibs);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Axe"), p->i_axegibs);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Stomps"), p->i_stompgibs);
	G_sprint(ed, 2, " \220%s\221\n", "MULTIGIBS");
	G_sprint(ed, 2, "  %s: %d\n", redtext("Total Multigibs"), p->i_multigibs);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Maximum Victims"), p->i_maxmultigibs);
	G_sprint(ed, 2, " \220%s\221\n", "AIRGIBS");
	G_sprint(ed, 2, "  %s: %d\n", redtext("Total"), p->i_airgibs);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Total Height"), p->i_height);
	G_sprint(ed, 2, "  %s: %d\n", redtext("Maximum Height"), p->i_maxheight);
	G_sprint(ed, 2, "  %s: %.1f\n", redtext("Average Height"), avg_height);

	if (!tp)
	{
		G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
	}
}

static void laststats_print_player_stats(const laststats_snapshot_t *snap, gedict_t *ed, int index, qbool tp)
{
	const laststats_player_t *p = &snap->players[index];

	if (tp)
	{
		G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\237\n");
	}

	G_sprint(ed, 2, "\207 %s:\n  %d (%d) %s%.1f%%\n",
			laststats_player_name(snap, index),
			p->frags,
			p->frag_diff,
			(tp ? va("%d ", p->friendly) : ""),
			p->efficiency);

	G_sprint(ed, 2, "%s:%s%s%s%s%s\n", redtext("Wp"),
			(p->lg_attacks ? va(" %s%.1f%% (%d/%d)", redtext("lg"), p->lg_pct,
						p->lg_hits, p->lg_attacks) : ""),
			(p->rl_pct ? va(" %s%.1f%% (%d/%d)", redtext("rl"), p->rl_pct,
						p->rl_vhits, p->rl_attacks) : ""),
			(p->gl_pct ? va(" %s%.1f%%", redtext("gl"), p->gl_pct) : ""),
			(p->sg_pct ? va(" %s%.1f%% (%d)", redtext("sg"), p->sg_pct, p->sg_damage) : ""),
			(p->ssg_pct ? va(" %s%.1f%% (%d)", redtext("ssg"), p->ssg_pct, p->ssg_damage) : ""));

	if (!snap->flags.lgc)
	{
		G_sprint(ed, 2, "%s: %s:%.1f %s:%.0f\n", redtext("RL skill"), redtext("ad"),
				p->rl_ad, redtext("dh"), (float)p->rl_hits);
	}

	if (snap->flags.duel)
	{
		G_sprint(ed, 2, "%s: %s:%.1f %s:%.1f\n", redtext("   Speed"), redtext("max"),
				p->speed_max, redtext("average"), p->speed_avg);
	}

	if (!p->bot && snap->flags.socd)
	{
		float perfect_pct = p->socd_changes > 0 ?
				(100.0f * p->socd_perfect / p->socd_changes) : 0.0f;

		G_sprint(ed, 2, "%s: %s:%.1f%% (%d/%d) %s:%d/%d%s [%s]\n",
			redtext("Movement"), redtext("Perfect strafes"),
			perfect_pct, p->socd_perfect, p->socd_changes,
			redtext("SOCD detections"), p->socd_detection, p->socd_validation,
			p->socd_assisted ? ". SOCD movement assistance detected!" : "",
			SOCD_DETECTION_VERSION);
	}

	if (!snap->flags.lgc)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("Armr&mhs"),
				redtext("ga"), p->ga, redtext("ya"), p->ya, redtext("ra"), p->ra,
				redtext("mh"), p->mh);
	}

	if (snap->flags.team || snap->flags.ctf)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d\n", redtext("Powerups"), redtext("Q"),
				p->quad, redtext("P"), p->pent, redtext("R"), p->ring);
	}

	if (snap->flags.ctf)
	{
		if (snap->flags.ctf_runes)
		{
			G_sprint(ed, 2, "%s: %s:%d%% %s:%d%% %s:%d%% %s:%d%%\n", redtext("RuneTime"),
					redtext("res"), p->res, redtext("str"), p->str, redtext("hst"), p->hst,
					redtext("rgn"), p->rgn);
		}

		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d\n", redtext("     CTF"), redtext("pickups"),
				p->pickups, redtext("caps"), p->caps, redtext("returns"), p->returns);
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" Defends"), redtext("flag"),
				p->f_defends, redtext("carrier"), p->c_defends);
	}

	if (snap->flags.team)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d%s\n", redtext("      RL"), redtext("Took"),
				p->rl_tooks, redtext("Killed"), p->rl_inv_ekills, redtext("Dropped"),
				p->rl_drops,
				(p->rl_xfer ? va(" %s:%d", redtext("Xfer"), p->rl_xfer) : ""));
		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d%s\n", redtext("      LG"), redtext("Took"),
				p->lg_tooks, redtext("Killed"), p->lg_inv_ekills, redtext("Dropped"),
				p->lg_drops,
				(p->lg_xfer ? va(" %s:%d", redtext("Xfer"), p->lg_xfer) : ""));
	}

	if (snap->flags.team && (snap->flags.deathmatch == 1))
	{
		G_sprint(ed, 2, "%s: %s:%.0f %s:%.0f %s:%.0f %s:%.0f %s:%.0f %s:%.0f\n",
				redtext("  Damage"), redtext("Tkn"), (float)p->dmg_t, redtext("Gvn"),
				(float)p->dmg_g, redtext("EWep"), (float)p->dmg_eweapon, redtext("Tm"),
				(float)p->dmg_team, redtext("Self"), (float)p->dmg_self, redtext("ToDie"),
				(float)p->dmg_todie);
	}
	else
	{
		G_sprint(ed, 2, "%s: %s:%.0f %s:%.0f %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"),
				redtext("Tkn"), (float)p->dmg_t, redtext("Gvn"), (float)p->dmg_g,
				redtext("Tm"), (float)p->dmg_team, redtext("Self"), (float)p->dmg_self,
				redtext("ToDie"), (float)p->dmg_todie);
	}

	if (snap->flags.time_valid)
	{
		if (snap->flags.duel)
		{
			G_sprint(ed, 2, "%s: %s:%d (%d%%)\n", redtext("    Time"), redtext("Control"),
					p->control_time, p->control_pct);
		}
		else
		{
			G_sprint(ed, 2, "%s: %s:%d\n", redtext("    Time"), redtext("Quad"),
					(int)p->quad_time);
		}
	}

	if (snap->flags.team || snap->flags.ctf)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext("Killed Q"), redtext("Enemy"),
				p->quad_ekills, redtext("Team"), p->quad_tkills);
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext("Killed R"), redtext("Enemy"),
				p->ring_ekills, redtext("Team"), p->ring_tkills);
	}

	if (p->axe_ekills || p->axe_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext("AXE KILL"), redtext("Enemy"),
				p->axe_ekills, redtext("Team"), p->axe_tkills);
	}

	if (p->sg_ekills || p->sg_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" SG KILL"), redtext("Enemy"),
				p->sg_ekills, redtext("Team"), p->sg_tkills);
	}

	if (p->ssg_ekills || p->ssg_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext("SSG KILL"), redtext("Enemy"),
				p->ssg_ekills, redtext("Team"), p->ssg_tkills);
	}

	if (p->ng_ekills || p->ng_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" NG KILL"), redtext("Enemy"),
				p->ng_ekills, redtext("Team"), p->ng_tkills);
	}

	if (p->sng_ekills || p->sng_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext("SNG KILL"), redtext("Enemy"),
				p->sng_ekills, redtext("Team"), p->sng_tkills);
	}

	if (p->gl_ekills || p->gl_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" GL KILL"), redtext("Enemy"),
				p->gl_ekills, redtext("Team"), p->gl_tkills);
	}

	if (p->rl_ekills || p->rl_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" RL KILL"), redtext("Enemy"),
				p->rl_ekills, redtext("Team"), p->rl_tkills);
	}

	if (p->lg_ekills || p->lg_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" LG KILL"), redtext("Enemy"),
				p->lg_ekills, redtext("Team"), p->lg_tkills);
	}

	if (p->discharge_ekills || p->discharge_tkills)
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" DC KILL"), redtext("Enemy"),
				p->discharge_ekills, redtext("Team"), p->discharge_tkills);
	}

	if (snap->flags.duel)
	{
		G_sprint(ed, 2, " %s: %s:%d %s:", redtext("EndGame"), redtext("H"), p->health,
				redtext("A"));
		if (p->armor)
		{
			G_sprint(ed, 2, "%s%d\n", armor_type(p->armor_items), p->armor);
		}
		else
		{
			G_sprint(ed, 2, "0\n");
		}

		if (snap->flags.overtime)
		{
			G_sprint(ed, 2, " %s: %s:%d %s:", redtext("OverTime"), redtext("H"), p->ot_h,
					redtext("A"));
			if (p->ot_a)
			{
				G_sprint(ed, 2, "%s%d\n", armor_type(p->ot_items), p->ot_a);
			}
			else
			{
				G_sprint(ed, 2, "0\n");
			}
		}
	}
	else
	{
		G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" Streaks"), redtext("Frags"),
				p->spree_max, redtext("QuadRun"), p->spree_max_q);
	}

	if (!snap->flags.ctf && !snap->flags.lgc)
	{
		G_sprint(ed, 2, "  %s: %d\n", redtext("SpawnFrags"), p->spawn_frags);
	}

	if (snap->flags.lgc && p->lg_attacks)
	{
		int over = p->lgc_over;
		int under = p->lgc_under;

		G_sprint(ed, 2, "  %s : \20%d\21\n", redtext("LGC Score"),
				(int)(p->lg_pct * p->frags_raw));
		G_sprint(ed, 2, "  %s : %.1f%% (%d/%d)\n", redtext("Overshaft"),
				(over * 100.0f) / p->lg_attacks, over, p->lg_attacks);
		G_sprint(ed, 2, "  %s: %.1f%% (%d/%d)\n", redtext("Undershaft"),
				(under * 100.0f) / p->lg_attacks, under, p->lg_attacks);
	}

	if (!tp)
	{
		G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\237\n");
	}
}

static void laststats_print_players_stats(const laststats_snapshot_t *snap, gedict_t *ed)
{
	qbool printed[MAX_CLIENTS];
	qbool tp = snap->flags.team || snap->flags.ctf;
	int i;
	int j;

	G_sprint(ed, 2, "\n%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Player statistics"));
	if (!snap->flags.midair)
	{
		G_sprint(ed, 2, "%s (%s) %s\217 %s\n", redtext("Frags"), redtext("rank"),
				(tp ? redtext("friendkills ") : ""), redtext("efficiency"));
	}

	memset(printed, 0, sizeof(printed));
	for (i = 0; i < snap->player_count; i++)
	{
		if (printed[i])
		{
			continue;
		}

		if (tp)
		{
			G_sprint(ed, 2, "Team \220%s\221:\n", snap->players[i].team);
		}

		for (j = 0; j < snap->player_count; j++)
		{
			if (!printed[j] && streq(snap->players[i].team, snap->players[j].team))
			{
				printed[j] = true;
				if (snap->flags.midair)
				{
					laststats_print_player_midair_stats(snap, ed, j);
				}
				else if (snap->flags.instagib)
				{
					laststats_print_player_instagib_stats(snap, ed, j, tp);
				}
				else
				{
					laststats_print_player_stats(snap, ed, j, tp);
				}
			}
		}
	}
}

static int laststats_team_index_v2(ls_team_stats_v2_t *teams, int *count, const char *team)
{
	int i;

	for (i = 0; i < *count; i++)
	{
		if (streq(teams[i].name, team))
		{
			return i;
		}
	}

	if (*count >= MAX_TM_STATS)
	{
		return -1;
	}

	strlcpy(teams[*count].name, team, sizeof(teams[*count].name));
	return (*count)++;
}

// Aggregate per-team totals from the per-player snapshot.
static void laststats_collect_team_stats_v2(const laststats_snapshot_t *snap, ls_team_stats_v2_t *teams, int *team_count)
{
	int i;
	int j;

	memset(teams, 0, sizeof(ls_team_stats_v2_t) * MAX_TM_STATS);
	*team_count = 0;

	for (i = 0; i < snap->player_count; i++)
	{
		const laststats_player_t *p = &snap->players[i];
		int idx;

		if (strnull(p->team))
		{
			continue;
		}

		idx = laststats_team_index_v2(teams, team_count, p->team);
		if (idx < 0)
		{
			continue;
		}

		if (p->ct == ctPlayer)
		{
			teams[idx].frags += p->frags_raw;
		}
		else
		{
			teams[idx].gfrags += p->frags_raw;
		}

		teams[idx].deaths += p->deaths;
		if (p->friendly != LS_INVALID_INT)
		{
			teams[idx].tkills += p->friendly;
		}
		teams[idx].dmg_t += p->dmg_t;
		teams[idx].dmg_g += p->dmg_g;
		teams[idx].dmg_team += p->dmg_team;
		teams[idx].dmg_eweapon += p->dmg_eweapon;

		teams[idx].ga += p->ga;
		teams[idx].ya += p->ya;
		teams[idx].ra += p->ra;
		teams[idx].mh += p->mh;
		teams[idx].quad += p->quad;
		teams[idx].pent += p->pent;
		teams[idx].ring += p->ring;
		teams[idx].quad_time += p->quad_time;

		teams[idx].rl_took += p->rl_tooks;
		teams[idx].rl_ekill += p->rl_inv_ekills;
		teams[idx].rl_drop += p->rl_drops;
		teams[idx].rl_xfer += p->rl_xfer;

		teams[idx].lg_took += p->lg_tooks;
		teams[idx].lg_ekill += p->lg_inv_ekills;
		teams[idx].lg_drop += p->lg_drops;
		teams[idx].lg_xfer += p->lg_xfer;

		teams[idx].lg_hits += p->lg_hits;
		teams[idx].lg_attacks += p->lg_attacks;
		teams[idx].rl_hits += p->rl_hits;
		teams[idx].gl_hits += p->gl_hits;
		teams[idx].sg_hits += p->sg_hits;
		teams[idx].sg_attacks += p->sg_attacks;
		teams[idx].ssg_hits += p->ssg_hits;
		teams[idx].ssg_attacks += p->ssg_attacks;

		teams[idx].pickups += p->pickups;
		teams[idx].caps += p->caps;
		teams[idx].returns += p->returns;
		teams[idx].f_defends += p->f_defends;
		teams[idx].c_defends += p->c_defends;
		teams[idx].res += p->res;
		teams[idx].str += p->str;
		teams[idx].hst += p->hst;
		teams[idx].rgn += p->rgn;
	}

	if (snap->team_count > 0)
	{
		for (i = 0; i < min(*team_count, MAX_TM_STATS); i++)
		{
			for (j = 0; j < min(snap->team_count, MAX_TM_STATS); j++)
			{
				if (streq(teams[i].name, snap->team_ctf[j].name))
				{
					teams[i].res = snap->team_ctf[j].res;
					teams[i].str = snap->team_ctf[j].str;
					teams[i].hst = snap->team_ctf[j].hst;
					teams[i].rgn = snap->team_ctf[j].rgn;
					break;
				}
			}
		}
	}
}

static void laststats_show_teams_banner(const ls_team_stats_v2_t *teams, int team_count, gedict_t *ed)
{
	int i;

	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
	for (i = 0; i < min(team_count, MAX_TM_STATS); i++)
	{
		G_sprint(ed, 2, "%s\220%s\221", (i ? " vs " : ""), teams[i].name);
	}
	G_sprint(ed, 2, " %s:\n", redtext("match statistics"));
	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

static void laststats_print_summary_tp_stats(const laststats_snapshot_t *snap, gedict_t *ed)
{
	ls_team_stats_v2_t teams[MAX_TM_STATS];
	int team_count = 0;
	int i;

	laststats_collect_team_stats_v2(snap, teams, &team_count);
	if (team_count == 0)
	{
		return;
	}

	laststats_show_teams_banner(teams, team_count, ed);

	G_sprint(ed, 2, "\n%s, %s, %s, %s\n", redtext("weapons"), redtext("powerups"),
			redtext("armors&mhs"), redtext("damage"));
	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");

	for (i = 0; i < min(team_count, MAX_TM_STATS); i++)
	{
		float h_sg = 100.0f * teams[i].sg_hits / max(1, teams[i].sg_attacks);
		float h_ssg = 100.0f * teams[i].ssg_hits / max(1, teams[i].ssg_attacks);
		float h_gl = teams[i].gl_hits;
		float h_rl = teams[i].rl_hits;
		float h_lg = 100.0f * teams[i].lg_hits / max(1, teams[i].lg_attacks);

		if (!snap->flags.instagib)
		{
			G_sprint(ed, 2, "\220%s\221: %s:%s%s%s%s%s\n", teams[i].name, redtext("Wp"),
					(h_lg ? va(" %s%.0f%%", redtext("lg"), h_lg) : ""),
					(h_rl ? va(" %s%.0f", redtext("rl"), h_rl) : ""),
					(h_gl ? va(" %s%.0f", redtext("gl"), h_gl) : ""),
					(h_sg ? va(" %s%.0f%%", redtext("sg"), h_sg) : ""),
					(h_ssg ? va(" %s%.0f%%", redtext("ssg"), h_ssg) : ""));

			G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d\n", redtext("Powerups"), redtext("Q"),
					teams[i].quad, redtext("P"), teams[i].pent, redtext("R"), teams[i].ring);
			G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("Armr&mhs"),
					redtext("ga"), teams[i].ga, redtext("ya"), teams[i].ya, redtext("ra"),
					teams[i].ra, redtext("mh"), teams[i].mh);
		}
		else
		{
			G_sprint(ed, 2, "\220%s\221: %s:%s\n", teams[i].name, redtext("Wp"),
					(h_sg ? va(" %s%.0f%%", redtext("cg"), h_sg) : ""));
		}

		if (snap->flags.ctf)
		{
			if (snap->flags.ctf_runes)
			{
				G_sprint(ed, 2, "%s: %s:%.0f%% %s:%.0f%% %s:%.0f%% %s:%.0f%%\n",
						redtext("RuneTime"), redtext("res"), (float)teams[i].res,
						redtext("str"), (float)teams[i].str, redtext("hst"),
						(float)teams[i].hst, redtext("rgn"), (float)teams[i].rgn);
			}

			G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d\n", redtext("     CTF"),
					redtext("pickups"), teams[i].pickups, redtext("caps"), teams[i].caps,
					redtext("returns"), teams[i].returns);
			G_sprint(ed, 2, "%s: %s:%d %s:%d\n", redtext(" Defends"), redtext("flag"),
					teams[i].f_defends, redtext("carrier"), teams[i].c_defends);
		}

		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("      RL"),
				redtext("Took"), teams[i].rl_took, redtext("Killed"), teams[i].rl_ekill,
				redtext("Dropped"), teams[i].rl_drop, redtext("Xfer"), teams[i].rl_xfer);
		G_sprint(ed, 2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("      LG"),
				redtext("Took"), teams[i].lg_took, redtext("Killed"), teams[i].lg_ekill,
				redtext("Dropped"), teams[i].lg_drop, redtext("Xfer"), teams[i].lg_xfer);

		if (snap->flags.deathmatch == 1)
		{
			G_sprint(ed, 2, "%s: %s:%.0f %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"),
					redtext("Tkn"), (float)teams[i].dmg_t, redtext("Gvn"),
					(float)teams[i].dmg_g, redtext("EWep"), (float)teams[i].dmg_eweapon,
					redtext("Tm"), (float)teams[i].dmg_team);
		}
		else
		{
			G_sprint(ed, 2, "%s: %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"),
					redtext("Tkn"), (float)teams[i].dmg_t, redtext("Gvn"),
					(float)teams[i].dmg_g, redtext("Tm"), (float)teams[i].dmg_team);
		}

		G_sprint(ed, 2, "%s: %s:%d\n", redtext("    Time"), redtext("Quad"),
				(int)teams[i].quad_time);
	}

	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

static void laststats_print_team_scores(const laststats_snapshot_t *snap, gedict_t *ed)
{
	ls_team_stats_v2_t teams[MAX_TM_STATS];
	int team_count = 0;
	int sumfrags = 0;
	int i;

	if (snap->flags.ca)
	{
		if (snap->ca_team1_score != snap->ca_team2_score)
		{
			const char *winner = snap->ca_team1_score > snap->ca_team2_score ?
					snap->ca_team1_name : snap->ca_team2_name;
			int high = snap->ca_team1_score > snap->ca_team2_score ?
					snap->ca_team1_score : snap->ca_team2_score;
			int low = snap->ca_team1_score > snap->ca_team2_score ?
					snap->ca_team2_score : snap->ca_team1_score;

			G_sprint(ed, 2, "%s \x90%s\x91 wins %d to %d\n", redtext("Team"),
					winner, high, low);
		}
		else
		{
			G_sprint(ed, 2, "%s have equal scores %d\n", redtext("Teams"), snap->ca_team1_score);
		}
		return;
	}

	laststats_collect_team_stats_v2(snap, teams, &team_count);
	if (!team_count)
	{
		return;
	}

	for (i = 0; i < min(team_count, MAX_TM_STATS); i++)
	{
		sumfrags += max(0, teams[i].frags + teams[i].gfrags);
	}

	G_sprint(ed, 2, "\n%s: %s\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Team scores"), redtext("frags \217 percentage"));

	for (i = 0; i < min(team_count, MAX_TM_STATS); i++)
	{
		G_sprint(ed, 2, "\220%s\221: %d", teams[i].name, teams[i].frags);
		if (teams[i].gfrags)
		{
			G_sprint(ed, 2, " + (%d) = %d", teams[i].gfrags, teams[i].frags + teams[i].gfrags);
		}
		G_sprint(ed, 2, " \217 %.1f%%\n",
				(sumfrags > 0 ? ((float)(teams[i].frags + teams[i].gfrags)) / sumfrags * 100 : 0.0f));
	}

	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

typedef void (*laststats_row_fn)(const laststats_snapshot_t *snap, gedict_t *ed, int index);

static void laststats_print_by_team(const laststats_snapshot_t *snap, gedict_t *ed, laststats_row_fn fn)
{
	qbool printed[MAX_CLIENTS];
	int i;
	int j;

	memset(printed, 0, sizeof(printed));

	for (i = 0; i < snap->player_count; i++)
	{
		if (printed[i])
		{
			continue;
		}
		for (j = 0; j < snap->player_count; j++)
		{
			if (!printed[j] && streq(snap->players[i].team, snap->players[j].team))
			{
				printed[j] = true;
				fn(snap, ed, j);
			}
		}
	}
}

static void laststats_print_midair_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_midair_row_t *row = &snap->midair[index];

	G_sprint(ed, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%5s|%5s|\n",
			laststats_player_name(snap, index),
			row->total,
			row->bronze,
			row->silver,
			row->gold,
			row->platinum,
			row->stomps,
			row->bonus,
			va("%.1f", row->max_height),
			va("%.1f", row->avg_height));
}

static void laststats_print_midair_kill_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_midair_kill_row_t *row = &snap->midair_kill[index];

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5s|\n",
			laststats_player_name(snap, index),
			row->frags,
			row->spawn_frags,
			row->spree,
			laststats_format_percent_exact(row->rl_eff));
}

static void laststats_print_insta_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_insta_row_t *row = &snap->insta[index];

	G_sprint(ed, 2, "%-20s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|\n",
			laststats_player_name(snap, index),
			row->cggibs,
			row->axegibs,
			row->stompgibs,
			row->multigibs,
			row->maxmultigibs,
			row->airgibs,
			row->total_height,
			row->max_height,
			row->rings);
}

static void laststats_print_insta_kill_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_insta_kill_row_t *row = &snap->insta_kill[index];
	const char *sg_eff = (row->sg_eff == LS_INVALID_FLOAT) ? "-" : va("%.1f%%", row->sg_eff);

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5s|\n",
			laststats_player_name(snap, index),
			row->frags,
			row->kills,
			row->deaths,
			row->spawn_frags,
			row->spree,
			sg_eff);
}

static void laststats_print_lgc_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_lgc_row_t *row = &snap->lgc[index];
	const char *eff = laststats_format_percent_exact(row->eff_pct);

	G_sprint(ed, 2, "%-20s|%5d|%5s|%5s|%5s|\n",
			laststats_player_name(snap, index),
			row->score,
			va("%.1f%%", row->over_pct),
			va("%.1f%%", row->under_pct),
			eff);
}

static void laststats_print_kill_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_kill_row_t *row = &snap->kill[index];
	const char *friendly = (row->friendly == LS_INVALID_INT) ? "-" : va("%d", row->friendly);

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5s|%5d|%5s|\n",
			laststats_player_name(snap, index),
			row->frags,
			row->kills,
			row->deaths,
			row->suicides,
			friendly,
			row->spawn_frags,
			laststats_format_efficiency(row->efficiency));
}

static void laststats_print_item_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_item_row_t *row = &snap->items[index];
	const char *quad = (row->quad == LS_INVALID_INT) ? "-" : va("%d", row->quad);
	const char *pent = (row->pent == LS_INVALID_INT) ? "-" : va("%d", row->pent);
	const char *ring = (row->ring == LS_INVALID_INT) ? "-" : va("%d", row->ring);

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5s|%5s|%5s|\n",
			laststats_player_name(snap, index),
			row->ga,
			row->ya,
			row->ra,
			row->mh,
			quad,
			pent,
			ring);
}

static void laststats_print_wep_eff_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_wep_eff_row_t *row = &snap->wep_eff[index];
	const char *lg = (row->lg == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->lg);
	const char *rl = (row->rl == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->rl);
	const char *gl = (row->gl == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->gl);
	const char *sng = (row->sng == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->sng);
	const char *ng = (row->ng == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->ng);
	const char *ssg = (row->ssg == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->ssg);
	const char *sg = (row->sg == LS_INVALID_FLOAT) ? "-" : laststats_format_percent(row->sg);

	G_sprint(ed, 2, "%-20s|%5s|%5s|%5s|%5s|%5s|%5s|%5s|\n",
			laststats_player_name(snap, index),
			lg,
			rl,
			gl,
			sng,
			ng,
			ssg,
			sg);
}

static void laststats_print_wep_dmg_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_wep_dmg_row_t *row = &snap->wep_dmg[index];

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			laststats_player_name(snap, index),
			row->lg,
			row->rl,
			row->gl,
			row->sng,
			row->ng,
			row->ssg,
			row->sg);
}

static void laststats_print_wep_simple_row(const laststats_snapshot_t *snap, gedict_t *ed, int index,
		const ls_wep_simple_row_t *rows)
{
	const ls_wep_simple_row_t *row = &rows[index];

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			laststats_player_name(snap, index),
			row->lg,
			row->rl,
			row->gl,
			row->sng,
			row->ng,
			row->ssg);
}

static void laststats_print_wep_taken_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	laststats_print_wep_simple_row(snap, ed, index, snap->wep_taken);
}

static void laststats_print_wep_drop_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	laststats_print_wep_simple_row(snap, ed, index, snap->wep_drop);
}

static void laststats_print_wep_kill_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	laststats_print_wep_simple_row(snap, ed, index, snap->wep_kill);
}

static void laststats_print_wep_ekill_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	laststats_print_wep_simple_row(snap, ed, index, snap->wep_ekill);
}

static void laststats_print_damage_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_damage_row_t *row = &snap->damage[index];

	G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			laststats_player_name(snap, index),
			row->taken,
			row->given,
			row->eweapon,
			row->team,
			row->self,
			row->per_death);
}

static void laststats_print_item_time_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_item_time_row_t *row = &snap->item_time[index];

	G_sprint(ed, 2, "%-20s|%s|%s|%s|%5s|%5s|%5s|\n",
			laststats_player_name(snap, index),
			laststats_format_time(row->ra),
			laststats_format_time(row->ya),
			laststats_format_time(row->ga),
			laststats_format_time(row->quad),
			laststats_format_time(row->pent),
			laststats_format_time(row->ring));
}

static void laststats_print_wep_time_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_wep_time_row_t *row = &snap->wep_time[index];

	G_sprint(ed, 2, "%-20s|%s|%s|%s|%s|%s|%s|\n",
			laststats_player_name(snap, index),
			laststats_format_time(row->lg),
			laststats_format_time(row->rl),
			laststats_format_time(row->gl),
			laststats_format_time(row->sng),
			laststats_format_time(row->ng),
			laststats_format_time(row->ssg));
}

static void laststats_print_ctf_row(const laststats_snapshot_t *snap, gedict_t *ed, int index)
{
	const ls_ctf_row_t *row = &snap->ctf[index];

	G_sprint(ed, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3s|%3s|%3s|%3s|\n",
			laststats_player_name(snap, index),
			row->pickups,
			row->caps,
			row->returns,
			row->f_defends,
			row->c_defends,
			laststats_format_rune(row->res),
			laststats_format_rune(row->str),
			laststats_format_rune(row->hst),
			laststats_format_rune(row->rgn));
}

static void laststats_print_top_int(gedict_t *ed, const laststats_snapshot_t *snap, const char *label,
		const ls_top_int_entry_t *entry, const char *suffix)
{
	int i;

	if (!entry->has_value)
	{
		return;
	}

	for (i = 0; i < entry->player_count; i++)
	{
		G_sprint(ed, 2, "   %-13s: %s (%d%s)\n", (i ? "" : redtext((char *)label)),
				laststats_player_name(snap, entry->players[i]), entry->value, suffix ? suffix : "");
	}
}

static void laststats_print_top_float(gedict_t *ed, const laststats_snapshot_t *snap, const char *label,
		const ls_top_float_entry_t *entry, const char *fmt)
{
	int i;

	if (!entry->has_value)
	{
		return;
	}

	for (i = 0; i < entry->player_count; i++)
	{
		G_sprint(ed, 2, "   %-13s: %s (%s)\n", (i ? "" : redtext((char *)label)),
				laststats_player_name(snap, entry->players[i]), va((char *)fmt, entry->value));
	}
}

static void laststats_print_top_stats(const laststats_snapshot_t *snap, gedict_t *ed)
{
	G_sprint(ed, 2, "\n%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Top performers"));

	laststats_print_top_int(ed, snap, "Frags", &snap->top.frags, "");
	laststats_print_top_int(ed, snap, "Deaths", &snap->top.deaths, "");
	laststats_print_top_int(ed, snap, "Friendkills", &snap->top.friendkills, "");
	laststats_print_top_int(ed, snap, "Efficiency", &snap->top.efficiency, "%");
	laststats_print_top_int(ed, snap, "Fragstreak", &snap->top.fragstreak, "");
	laststats_print_top_int(ed, snap, "Quadrun", &snap->top.quadrun, "");
	laststats_print_top_int(ed, snap, "Annihilator", &snap->top.annihilator, "");
	laststats_print_top_int(ed, snap, "Survivor", &snap->top.survivor, "");
	laststats_print_top_int(ed, snap, "Spawnfrags", &snap->top.spawnfrags, "");

	if (snap->flags.ctf)
	{
		laststats_print_top_int(ed, snap, "Captures", &snap->top.captures, "");
		laststats_print_top_int(ed, snap, "FlagDefends", &snap->top.flagdefends, "");
	}
}

static void laststats_print_top_midair(const laststats_snapshot_t *snap, gedict_t *ed)
{
	G_sprint(ed, 2, "\n%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Top performers"));

	laststats_print_top_int(ed, snap, "score", &snap->top_midair.score, "");
	laststats_print_top_int(ed, snap, "kills", &snap->top_midair.kills, "");
	laststats_print_top_int(ed, snap, "midairs", &snap->top_midair.midairs, "");
	laststats_print_top_int(ed, snap, "head stomps", &snap->top_midair.headstomps, "");
	laststats_print_top_int(ed, snap, "streak", &snap->top_midair.streak, "");
	laststats_print_top_int(ed, snap, "spawn frags", &snap->top_midair.spawnfrags, "");
	laststats_print_top_int(ed, snap, "bonus fiend", &snap->top_midair.bonus, "");
	laststats_print_top_float(ed, snap, "highest kill", &snap->top_midair.highest, "%.1f");
	laststats_print_top_float(ed, snap, "avg height", &snap->top_midair.avgheight, "%.1f");
	laststats_print_top_float(ed, snap, "rl efficiency", &snap->top_midair.rleff, "%.1f%%");

	G_sprint(ed, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n\n");
}

static void laststats_print_team_summary(const laststats_snapshot_t *snap, gedict_t *ed)
{
	int i;

	G_sprint(ed, 2, "\n%s    |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Team Items Taken"), redtext("RA"), redtext("YA"), redtext("GA"),
			redtext("MH"), redtext(" Q"), redtext(" P"), redtext(" R"));
	for (i = 0; i < min(snap->team_count, MAX_TM_STATS); i++)
	{
		const ls_team_items_row_t *row = &snap->team_items[i];
		G_sprint(ed, 2, "%-20s|%2d|%2d|%2d|%2d|%2d|%2d|%2d|\n",
				va("\220%s\221", row->name),
				row->ra,
				row->ya,
				row->ga,
				row->mh,
				row->quad,
				row->pent,
				row->ring);
	}

	G_sprint(ed, 2, "\n%s        |%s|%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Team Weapons"), redtext("LGT"), redtext("LGK"), redtext("LGD"),
			redtext("LGX"), redtext("RLT"), redtext("RLK"), redtext("RLD"), redtext("RLX"));
	for (i = 0; i < min(snap->team_count, MAX_TM_STATS); i++)
	{
		const ls_team_weapons_row_t *row = &snap->team_weapons[i];
		G_sprint(ed, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|\n",
				va("\220%s\221", row->name),
				row->lgt,
				row->lgk,
				row->lgd,
				row->lgx,
				row->rlt,
				row->rlk,
				row->rld,
				row->rlx);
	}

	G_sprint(ed, 2, "\n%s   |%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\237\n",
			redtext("Team Frags+Damage"), redtext("Frags"), redtext("Given"), redtext("Taken"),
			redtext(" EWep"), redtext(" Team"));
	for (i = 0; i < min(snap->team_count, MAX_TM_STATS); i++)
	{
		const ls_team_damage_row_t *row = &snap->team_damage[i];
		G_sprint(ed, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|\n",
				va("\220%s\221", row->name),
				row->frags,
				row->given,
				row->taken,
				row->eweapon,
				row->team);
	}

	if (snap->flags.ctf)
	{
		G_sprint(ed, 2, "\n%s |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Team CTF statistics"), redtext("Fla"), redtext("Cap"), redtext("Ret"),
				redtext("DFl"), redtext("DCa"), redtext("Res"), redtext("Str"), redtext("Hst"),
				redtext("Rgn"));
		for (i = 0; i < min(snap->team_count, MAX_TM_STATS); i++)
		{
			const ls_team_ctf_row_t *row = &snap->team_ctf[i];
			G_sprint(ed, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3s|%3s|%3s|%3s|\n",
					va("\220%s\221", row->name),
					row->pickups,
					row->caps,
					row->returns,
					row->f_defends,
					row->c_defends,
					laststats_format_rune(row->res),
					laststats_format_rune(row->str),
					laststats_format_rune(row->hst),
					laststats_format_rune(row->rgn));
		}
	}
}

static void laststats_collect_top_int(ls_top_int_entry_t *entry, int value, int player_index)
{
	if (entry->player_count >= MAX_CLIENTS)
	{
		return;
	}
	entry->players[entry->player_count++] = player_index;
	entry->value = value;
	entry->has_value = true;
}

static void laststats_collect_top_float(ls_top_float_entry_t *entry, float value, int player_index)
{
	if (entry->player_count >= MAX_CLIENTS)
	{
		return;
	}
	entry->players[entry->player_count++] = player_index;
	entry->value = value;
	entry->has_value = true;
}

// Compute top-performer rows when building the snapshot at match end.
static void laststats_compute_top_stats(laststats_snapshot_t *snap, gedict_t *players[], int count)
{
	float maxfrags = -99999;
	float maxdeaths = -1;
	float maxfriend = -1;
	float maxeffi = -1;
	float maxspree = -1;
	float maxspree_q = -1;
	float maxdmgg = -1;
	float maxdmgtd = -1;
	int maxspawnfrags = 0;
	int maxcaps = 0;
	int maxdefends = 0;
	int i;

	for (i = 0; i < count; i++)
	{
		gedict_t *p = players[i];
		float frags = (isCTF() ? p->s.v.frags - p->ps.ctf_points : p->s.v.frags);
		float eff = laststats_efficiency(p);
		float dmgtd = (p->deaths <= 0) ? 99999 : (int)(p->ps.dmg_t / p->deaths);

		maxfrags = max(frags, maxfrags);
		maxdeaths = max(p->deaths, maxdeaths);
		maxfriend = max(p->friendly, maxfriend);
		maxeffi = max(eff, maxeffi);
		maxspree = max(p->ps.spree_max, maxspree);
		maxspree_q = max(p->ps.spree_max_q, maxspree_q);
		maxdmgg = max(p->ps.dmg_g, maxdmgg);
		maxdmgtd = max(dmgtd, maxdmgtd);
		maxspawnfrags = max(p->ps.spawn_frags, maxspawnfrags);
		maxcaps = max(p->ps.caps, maxcaps);
		maxdefends = max(p->ps.f_defends, maxdefends);
	}

	for (i = 0; i < count; i++)
	{
		gedict_t *p = players[i];
		float eff = laststats_efficiency(p);
		float dmgtd = (p->deaths <= 0) ? 99999 : (p->ps.dmg_t / p->deaths);

		if ((!isCTF() && (p->s.v.frags == maxfrags))
				|| (isCTF() && ((p->s.v.frags - p->ps.ctf_points) == maxfrags)))
		{
			laststats_collect_top_int(&snap->top.frags, (int)maxfrags, i);
		}

		if (p->deaths == maxdeaths)
		{
			laststats_collect_top_int(&snap->top.deaths, (int)maxdeaths, i);
		}

		if ((maxfriend > 0) && (p->friendly == maxfriend))
		{
			laststats_collect_top_int(&snap->top.friendkills, (int)maxfriend, i);
		}

		if (eff == maxeffi)
		{
			laststats_collect_top_int(&snap->top.efficiency, (int)maxeffi, i);
		}

		if (p->ps.spree_max == maxspree)
		{
			laststats_collect_top_int(&snap->top.fragstreak, (int)maxspree, i);
		}

		if ((maxspree_q > 0) && (p->ps.spree_max_q == maxspree_q))
		{
			laststats_collect_top_int(&snap->top.quadrun, (int)maxspree_q, i);
		}

		if (p->ps.dmg_g == maxdmgg)
		{
			laststats_collect_top_int(&snap->top.annihilator, (int)maxdmgg, i);
		}

		if (dmgtd == maxdmgtd)
		{
			laststats_collect_top_int(&snap->top.survivor, (int)maxdmgtd, i);
		}

		if ((maxspawnfrags > 0) && (p->ps.spawn_frags == maxspawnfrags))
		{
			laststats_collect_top_int(&snap->top.spawnfrags, maxspawnfrags, i);
		}

		if (snap->flags.ctf)
		{
			if (p->ps.caps == maxcaps)
			{
				laststats_collect_top_int(&snap->top.captures, maxcaps, i);
			}
			if (p->ps.f_defends == maxdefends)
			{
				laststats_collect_top_int(&snap->top.flagdefends, maxdefends, i);
			}
		}
	}
}

static void laststats_compute_top_midair(laststats_snapshot_t *snap, gedict_t *players[], int count)
{
	float maxtopheight = 0;
	float maxtopavgheight = 0;
	float maxrlefficiency = 0;
	int maxscore = -99999;
	int maxkills = 0;
	int maxmidairs = 0;
	int maxstomps = 0;
	int maxstreak = 0;
	int maxspawnfrags = 0;
	int maxbonus = 0;
	int i;

	for (i = 0; i < count; i++)
	{
		gedict_t *p = players[i];
		float vh_rl = p->ps.wpn[wpRL].vhits;
		float a_rl = p->ps.wpn[wpRL].attacks;
		float ph_rl = 100.0f * vh_rl / max(1.0f, a_rl);

		maxscore = max(p->s.v.frags, maxscore);
		maxkills = max(p->ps.mid_total + p->ps.mid_stomps, maxkills);
		maxmidairs = max(p->ps.mid_total, maxmidairs);
		maxstomps = max(p->ps.mid_stomps, maxstomps);
		maxstreak = max(p->ps.spree_max, maxstreak);
		maxspawnfrags = max(p->ps.spawn_frags, maxspawnfrags);
		maxbonus = max(p->ps.mid_bonus, maxbonus);
		maxtopheight = max(p->ps.mid_maxheight, maxtopheight);
		maxtopavgheight = max(p->ps.mid_avgheight, maxtopavgheight);
		maxrlefficiency = max(ph_rl, maxrlefficiency);
	}

	for (i = 0; i < count; i++)
	{
		gedict_t *p = players[i];
		float vh_rl = p->ps.wpn[wpRL].vhits;
		float a_rl = p->ps.wpn[wpRL].attacks;
		float ph_rl = 100.0f * vh_rl / max(1.0f, a_rl);

		if (p->s.v.frags == maxscore)
		{
			laststats_collect_top_int(&snap->top_midair.score, maxscore, i);
		}
		if ((maxkills > 0) && ((p->ps.mid_total + p->ps.mid_stomps) == maxkills))
		{
			laststats_collect_top_int(&snap->top_midair.kills, maxkills, i);
		}
		if ((maxmidairs > 0) && (p->ps.mid_total == maxmidairs))
		{
			laststats_collect_top_int(&snap->top_midair.midairs, maxmidairs, i);
		}
		if ((maxstomps > 0) && (p->ps.mid_stomps == maxstomps))
		{
			laststats_collect_top_int(&snap->top_midair.headstomps, maxstomps, i);
		}
		if ((maxstreak > 0) && (p->ps.spree_max == maxstreak))
		{
			laststats_collect_top_int(&snap->top_midair.streak, maxstreak, i);
		}
		if ((maxspawnfrags > 0) && (p->ps.spawn_frags == maxspawnfrags))
		{
			laststats_collect_top_int(&snap->top_midair.spawnfrags, maxspawnfrags, i);
		}
		if ((maxbonus > 0) && (p->ps.mid_bonus == maxbonus))
		{
			laststats_collect_top_int(&snap->top_midair.bonus, maxbonus, i);
		}
		if ((maxtopheight > 0) && (p->ps.mid_maxheight == maxtopheight))
		{
			laststats_collect_top_float(&snap->top_midair.highest, maxtopheight, i);
		}
		if ((maxtopavgheight > 0) && (p->ps.mid_avgheight == maxtopavgheight))
		{
			laststats_collect_top_float(&snap->top_midair.avgheight, maxtopavgheight, i);
		}
		if (ph_rl == maxrlefficiency)
		{
			laststats_collect_top_float(&snap->top_midair.rleff, maxrlefficiency, i);
		}
	}
}

static int laststats_team_index(laststats_snapshot_t *snap, const char *team)
{
	int i;

	for (i = 0; i < snap->team_count; i++)
	{
		if (streq(snap->team_items[i].name, team))
		{
			return i;
		}
	}

	if (snap->team_count >= MAX_TM_STATS)
	{
		return -1;
	}

	strlcpy(snap->team_items[snap->team_count].name, team, sizeof(snap->team_items[0].name));
	strlcpy(snap->team_weapons[snap->team_count].name, team, sizeof(snap->team_weapons[0].name));
	strlcpy(snap->team_damage[snap->team_count].name, team, sizeof(snap->team_damage[0].name));
	strlcpy(snap->team_ctf[snap->team_count].name, team, sizeof(snap->team_ctf[0].name));
	return snap->team_count++;
}

static void laststats_collect_team_stats(laststats_snapshot_t *snap, gedict_t *players[], int count)
{
	float team_res[MAX_TM_STATS];
	float team_str[MAX_TM_STATS];
	float team_hst[MAX_TM_STATS];
	float team_rgn[MAX_TM_STATS];
	float duration = g_globalvars.time - match_start_time;
	int i;

	memset(team_res, 0, sizeof(team_res));
	memset(team_str, 0, sizeof(team_str));
	memset(team_hst, 0, sizeof(team_hst));
	memset(team_rgn, 0, sizeof(team_rgn));

	for (i = 0; i < count; i++)
	{
		gedict_t *p = players[i];
		const char *team = snap->players[i].team;
		int idx;

		if (strnull(team))
		{
			continue;
		}

		idx = laststats_team_index(snap, team);
		if (idx < 0)
		{
			continue;
		}

		if (p->ct == ctPlayer)
		{
			snap->team_damage[idx].frags += p->s.v.frags;
		}

		snap->team_damage[idx].given += (int)p->ps.dmg_g;
		snap->team_damage[idx].taken += (int)p->ps.dmg_t;
		snap->team_damage[idx].eweapon += (int)p->ps.dmg_eweapon;
		snap->team_damage[idx].team += (int)p->ps.dmg_team;

		snap->team_items[idx].ra += p->ps.itm[itRA].tooks;
		snap->team_items[idx].ya += p->ps.itm[itYA].tooks;
		snap->team_items[idx].ga += p->ps.itm[itGA].tooks;
		snap->team_items[idx].mh += p->ps.itm[itHEALTH_100].tooks;
		snap->team_items[idx].quad += p->ps.itm[itQUAD].tooks;
		snap->team_items[idx].pent += p->ps.itm[itPENT].tooks;
		snap->team_items[idx].ring += p->ps.itm[itRING].tooks;

		snap->team_weapons[idx].lgt += p->ps.wpn[wpLG].tooks;
		snap->team_weapons[idx].lgk += p->ps.wpn[wpLG].ekills;
		snap->team_weapons[idx].lgd += p->ps.wpn[wpLG].drops;
		snap->team_weapons[idx].lgx += p->ps.transferred_LGpacks;
		snap->team_weapons[idx].rlt += p->ps.wpn[wpRL].tooks;
		snap->team_weapons[idx].rlk += p->ps.wpn[wpRL].ekills;
		snap->team_weapons[idx].rld += p->ps.wpn[wpRL].drops;
		snap->team_weapons[idx].rlx += p->ps.transferred_RLpacks;

		snap->team_ctf[idx].pickups += p->ps.pickups;
		snap->team_ctf[idx].caps += p->ps.caps;
		snap->team_ctf[idx].returns += p->ps.returns;
		snap->team_ctf[idx].f_defends += p->ps.f_defends;
		snap->team_ctf[idx].c_defends += p->ps.c_defends;

		team_res[idx] += p->ps.res_time;
		team_str[idx] += p->ps.str_time;
		team_hst[idx] += p->ps.hst_time;
		team_rgn[idx] += p->ps.rgn_time;
	}

	for (i = 0; i < snap->team_count; i++)
	{
		if (!snap->flags.ctf || !snap->flags.ctf_runes)
		{
			snap->team_ctf[i].res = LS_INVALID_INT;
			snap->team_ctf[i].str = LS_INVALID_INT;
			snap->team_ctf[i].hst = LS_INVALID_INT;
			snap->team_ctf[i].rgn = LS_INVALID_INT;
			continue;
		}

		if (duration > 0.0f)
		{
			snap->team_ctf[i].res = (int)((team_res[i] / duration) * 100.0f);
			snap->team_ctf[i].str = (int)((team_str[i] / duration) * 100.0f);
			snap->team_ctf[i].hst = (int)((team_hst[i] / duration) * 100.0f);
			snap->team_ctf[i].rgn = (int)((team_rgn[i] / duration) * 100.0f);
		}
		else
		{
			snap->team_ctf[i].res = (int)team_res[i];
			snap->team_ctf[i].str = (int)team_str[i];
			snap->team_ctf[i].hst = (int)team_hst[i];
			snap->team_ctf[i].rgn = (int)team_rgn[i];
		}
	}
}

// Build a snapshot from live match data and persist it as JSON.
void LastStatsJsonSave(void)
{
	laststats_snapshot_t snap;
	gedict_t *players[MAX_CLIENTS];
	int from = 0;
	gedict_t *p;
	fileHandle_t handle;
	int i;
	qbool tp;
	float duration;

	if (isHoonyModeAny() && !HM_is_game_over())
	{
		return;
	}

	if (!lastStatsData)
	{
		return;
	}

	memset(&snap, 0, sizeof(snap));

	snap.version = LASTSTATS_JSON_VERSION;
	snap.flags.midair = cvar("k_midair") ? true : false;
	snap.flags.instagib = cvar("k_instagib") ? true : false;
	snap.flags.lgc = lgc_enabled() ? true : false;
	snap.flags.ctf = isCTF() ? true : false;
	snap.flags.team = isTeam() ? true : false;
	snap.flags.duel = isDuel() ? true : false;
	snap.flags.ctf_runes = cvar("k_ctf_runes") ? true : false;
	snap.flags.ra = isRA() ? true : false;
	snap.flags.ca = isCA() ? true : false;
	snap.flags.race = isRACE() ? true : false;
	snap.flags.socd = (cvar("k_socd") >= SOCD_STATS) ? true : false;
	snap.flags.overtime = k_overtime ? true : false;
	snap.flags.deathmatch = deathmatch;
	if (snap.flags.ca)
	{
		const char *team1 = cvar_string("_k_team1");
		const char *team2 = cvar_string("_k_team2");
		snap.ca_team1_score = CA_get_score_1();
		snap.ca_team2_score = CA_get_score_2();
		strlcpy(snap.ca_team1_name, team1 ? team1 : "", sizeof(snap.ca_team1_name));
		strlcpy(snap.ca_team2_name, team2 ? team2 : "", sizeof(snap.ca_team2_name));
	}

	for (p = find_plrghst(world, &from); p && snap.player_count < MAX_CLIENTS;
			p = find_plrghst(p, &from))
	{
		laststats_player_t *pl = &snap.players[snap.player_count];
		const char *team = getteam(p);

		strlcpy(pl->name, getname(p), sizeof(pl->name));
		strlcpy(pl->team, team ? team : "", sizeof(pl->team));
		pl->ghost = isghost(p);
		players[snap.player_count] = p;
		snap.player_count++;
	}

	if (snap.player_count == 0)
	{
		return;
	}

	tp = snap.flags.team || snap.flags.ctf;
	duration = g_globalvars.time - match_start_time;
	snap.flags.time_valid = (duration > 0.0f) ? true : false;

	for (i = 0; i < snap.player_count; i++)
	{
		gedict_t *pl = players[i];
		laststats_player_t *out = &snap.players[i];
		float h_lg = pl->ps.wpn[wpLG].hits;
		float a_lg = pl->ps.wpn[wpLG].attacks;
		float vh_rl = pl->ps.wpn[wpRL].vhits;
		float a_rl = pl->ps.wpn[wpRL].attacks;
		float h_rl = pl->ps.wpn[wpRL].hits;
		float vh_gl = pl->ps.wpn[wpGL].vhits;
		float a_gl = pl->ps.wpn[wpGL].attacks;
		float h_sg = pl->ps.wpn[wpSG].hits;
		float a_sg = pl->ps.wpn[wpSG].attacks;
		float h_ssg = pl->ps.wpn[wpSSG].hits;
		float a_ssg = pl->ps.wpn[wpSSG].attacks;
		float h_ax = pl->ps.wpn[wpAXE].hits;
		float a_ax = pl->ps.wpn[wpAXE].attacks;

		out->bot = pl->isBot;
		out->ct = pl->ct;
		out->frags_raw = (int)pl->s.v.frags;
		out->frags = snap.flags.ctf ? (int)(pl->s.v.frags - pl->ps.ctf_points) : (int)pl->s.v.frags;
		out->deaths = (int)pl->deaths;
		out->frag_diff = snap.flags.ctf ?
				(int)(pl->s.v.frags - pl->ps.ctf_points - pl->deaths) :
				(int)(pl->s.v.frags - pl->deaths);
		out->friendly = tp ? (int)pl->friendly : LS_INVALID_INT;
		out->efficiency = laststats_efficiency(pl);
		out->spawn_frags = pl->ps.spawn_frags;
		out->spree_max = pl->ps.spree_max;
		out->spree_max_q = pl->ps.spree_max_q;

		out->health = (int)pl->s.v.health;
		out->armor = (int)pl->s.v.armorvalue;
		out->armor_items = (int)pl->s.v.items;
		out->ot_h = pl->ps.ot_h;
		out->ot_a = pl->ps.ot_a;
		out->ot_items = pl->ps.ot_items;

		out->speed_max = pl->ps.velocity_max;
		out->speed_avg = (pl->ps.vel_frames > 0) ? (pl->ps.velocity_sum / pl->ps.vel_frames) : 0.0f;

		out->control_time = (int)pl->ps.control_time;
		out->control_pct = (duration > 0.0f) ? (int)((pl->ps.control_time / duration) * 100.0f) : 0;
		out->quad_time = pl->ps.itm[itQUAD].time;

		out->socd_perfect = pl->matchPerfectStrafeCount;
		out->socd_changes = pl->matchStrafeChangeCount;
		out->socd_detection = pl->socdDetectionCount;
		out->socd_validation = pl->socdValidationCount;
		out->socd_assisted = socd_movement_assisted(pl);

		out->ga = pl->ps.itm[itGA].tooks;
		out->ya = pl->ps.itm[itYA].tooks;
		out->ra = pl->ps.itm[itRA].tooks;
		out->mh = pl->ps.itm[itHEALTH_100].tooks;
		out->quad = pl->ps.itm[itQUAD].tooks;
		out->pent = pl->ps.itm[itPENT].tooks;
		out->ring = pl->ps.itm[itRING].tooks;

		out->quad_ekills = pl->ps.itm[itQUAD].ekills;
		out->quad_tkills = pl->ps.itm[itQUAD].tkills;
		out->ring_ekills = pl->ps.itm[itRING].ekills;
		out->ring_tkills = pl->ps.itm[itRING].tkills;

		out->dmg_t = (int)pl->ps.dmg_t;
		out->dmg_g = (int)pl->ps.dmg_g;
		out->dmg_team = (int)pl->ps.dmg_team;
		out->dmg_self = (int)pl->ps.dmg_self;
		out->dmg_eweapon = (int)pl->ps.dmg_eweapon;
		out->dmg_todie = (pl->deaths <= 0) ? 99999 : (int)(pl->ps.dmg_t / pl->deaths);

		out->rl_ad = (vh_rl > 0.0f) ? (pl->ps.dmg_g_rl / vh_rl) : 0.0f;
		out->rl_hits = (int)h_rl;

		out->lg_pct = 100.0f * h_lg / max(1.0f, a_lg);
		out->lg_hits = (int)h_lg;
		out->lg_attacks = (int)a_lg;
		out->lg_tooks = pl->ps.wpn[wpLG].tooks;
		out->lg_inv_ekills = pl->ps.wpn[wpLG].ekills;
		out->lg_drops = pl->ps.wpn[wpLG].drops;
		out->lg_xfer = pl->ps.transferred_LGpacks;

		out->rl_pct = 100.0f * vh_rl / max(1.0f, a_rl);
		out->rl_vhits = (int)vh_rl;
		out->rl_attacks = (int)a_rl;
		out->rl_tooks = pl->ps.wpn[wpRL].tooks;
		out->rl_inv_ekills = pl->ps.wpn[wpRL].ekills;
		out->rl_drops = pl->ps.wpn[wpRL].drops;
		out->rl_xfer = pl->ps.transferred_RLpacks;

		out->gl_pct = 100.0f * vh_gl / max(1.0f, a_gl);
		out->gl_hits = (int)pl->ps.wpn[wpGL].hits;

		out->sg_pct = 100.0f * h_sg / max(1.0f, a_sg);
		out->sg_hits = (int)h_sg;
		out->sg_attacks = (int)a_sg;
		out->sg_damage = pl->ps.wpn[wpSG].edamage;

		out->ssg_pct = 100.0f * h_ssg / max(1.0f, a_ssg);
		out->ssg_hits = (int)h_ssg;
		out->ssg_attacks = (int)a_ssg;
		out->ssg_damage = pl->ps.wpn[wpSSG].edamage;

		out->axe_pct = 100.0f * h_ax / max(1.0f, a_ax);
		out->axe_hits = (int)h_ax;
		out->axe_attacks = (int)a_ax;

		out->pickups = pl->ps.pickups;
		out->caps = pl->ps.caps;
		out->returns = pl->ps.returns;
		out->f_defends = pl->ps.f_defends;
		out->c_defends = pl->ps.c_defends;

		if (snap.flags.ctf && snap.flags.ctf_runes && duration > 0.0f)
		{
			out->res = (int)((pl->ps.res_time / duration) * 100.0f);
			out->str = (int)((pl->ps.str_time / duration) * 100.0f);
			out->hst = (int)((pl->ps.hst_time / duration) * 100.0f);
			out->rgn = (int)((pl->ps.rgn_time / duration) * 100.0f);
		}

		out->axe_ekills = pl->ps.wpn[wpAXE].kills;
		out->axe_tkills = pl->ps.wpn[wpAXE].tkills;
		out->sg_ekills = pl->ps.wpn[wpSG].kills;
		out->sg_tkills = pl->ps.wpn[wpSG].tkills;
		out->ssg_ekills = pl->ps.wpn[wpSSG].kills;
		out->ssg_tkills = pl->ps.wpn[wpSSG].tkills;
		out->ng_ekills = pl->ps.wpn[wpNG].kills;
		out->ng_tkills = pl->ps.wpn[wpNG].tkills;
		out->sng_ekills = pl->ps.wpn[wpSNG].kills;
		out->sng_tkills = pl->ps.wpn[wpSNG].tkills;
		out->gl_ekills = pl->ps.wpn[wpGL].kills;
		out->gl_tkills = pl->ps.wpn[wpGL].tkills;
		out->rl_ekills = pl->ps.wpn[wpRL].kills;
		out->rl_tkills = pl->ps.wpn[wpRL].tkills;
		out->lg_ekills = pl->ps.wpn[wpLG].kills;
		out->lg_tkills = pl->ps.wpn[wpLG].tkills;
		out->discharge_ekills = pl->ps.discharge_ekills;
		out->discharge_tkills = pl->ps.discharge_tkills;

		out->lgc_over = pl->ps.lgc_overshaft;
		out->lgc_under = pl->ps.lgc_undershaft;

		out->mid_total = pl->ps.mid_total;
		out->mid_bronze = pl->ps.mid_bronze;
		out->mid_silver = pl->ps.mid_silver;
		out->mid_gold = pl->ps.mid_gold;
		out->mid_platinum = pl->ps.mid_platinum;
		out->mid_stomps = pl->ps.mid_stomps;
		out->mid_bonus = pl->ps.mid_bonus;
		out->mid_maxheight = pl->ps.mid_maxheight;
		out->mid_avgheight = pl->ps.mid_maxheight ? pl->ps.mid_avgheight : 0.0f;

		out->i_cggibs = pl->ps.i_cggibs;
		out->i_axegibs = pl->ps.i_axegibs;
		out->i_stompgibs = pl->ps.i_stompgibs;
		out->i_multigibs = pl->ps.i_multigibs;
		out->i_maxmultigibs = pl->ps.i_maxmultigibs;
		out->i_airgibs = pl->ps.i_airgibs;
		out->i_height = pl->ps.i_height;
		out->i_maxheight = pl->ps.i_maxheight;
		out->i_rings = pl->ps.i_rings;
	}

	if (snap.flags.midair)
	{
		for (i = 0; i < snap.player_count; i++)
		{
			gedict_t *pl = players[i];
			ls_midair_row_t *row = &snap.midair[i];
			ls_midair_kill_row_t *krow = &snap.midair_kill[i];
			float vh_rl = pl->ps.wpn[wpRL].vhits;
			float a_rl = pl->ps.wpn[wpRL].attacks;

			row->total = pl->ps.mid_total;
			row->bronze = pl->ps.mid_bronze;
			row->silver = pl->ps.mid_silver;
			row->gold = pl->ps.mid_gold;
			row->platinum = pl->ps.mid_platinum;
			row->stomps = pl->ps.mid_stomps;
			row->bonus = pl->ps.mid_bonus;
			row->max_height = pl->ps.mid_maxheight;
			row->avg_height = pl->ps.mid_maxheight ? pl->ps.mid_avgheight : 0.0f;

			krow->frags = (int)pl->s.v.frags;
			krow->spawn_frags = pl->ps.spawn_frags;
			krow->spree = pl->ps.spree_max;
			krow->rl_eff = 100.0f * vh_rl / max(1.0f, a_rl);
		}

		snap.midair_count = snap.player_count;
		snap.midair_kill_count = snap.player_count;
		laststats_compute_top_midair(&snap, players, snap.player_count);
	}
	else if (snap.flags.instagib)
	{
		for (i = 0; i < snap.player_count; i++)
		{
			gedict_t *pl = players[i];
			ls_insta_row_t *row = &snap.insta[i];
			ls_insta_kill_row_t *krow = &snap.insta_kill[i];
			float h_sg = pl->ps.wpn[wpSG].hits;
			float a_sg = pl->ps.wpn[wpSG].attacks;

			row->cggibs = pl->ps.i_cggibs;
			row->axegibs = pl->ps.i_axegibs;
			row->stompgibs = pl->ps.i_stompgibs;
			row->multigibs = pl->ps.i_multigibs;
			row->maxmultigibs = pl->ps.i_maxmultigibs;
			row->airgibs = pl->ps.i_airgibs;
			row->total_height = pl->ps.i_height;
			row->max_height = pl->ps.i_maxheight;
			row->rings = pl->ps.i_rings;

			krow->frags = (int)pl->s.v.frags;
			krow->kills = pl->ps.i_cggibs + pl->ps.i_axegibs + pl->ps.i_stompgibs;
			krow->deaths = (int)pl->deaths;
			krow->spawn_frags = pl->ps.spawn_frags;
			krow->spree = pl->ps.spree_max;
			krow->sg_eff = (a_sg ? h_sg : LS_INVALID_FLOAT);
		}

		snap.insta_count = snap.player_count;
		snap.insta_kill_count = snap.player_count;
	}
	else if (snap.flags.lgc)
	{
		for (i = 0; i < snap.player_count; i++)
		{
			gedict_t *pl = players[i];
			ls_lgc_row_t *row = &snap.lgc[i];
			float a_lg = pl->ps.wpn[wpLG].attacks;
			float h_lg = pl->ps.wpn[wpLG].hits;
			float e_lg = 100.0f * h_lg / max(1.0f, a_lg);

			row->score = (int)(e_lg * pl->s.v.frags);
			row->over_pct = (a_lg != 0.0f) ? ((pl->ps.lgc_overshaft * 100.0f) / a_lg) : 0.0f;
			row->under_pct = (a_lg != 0.0f) ? ((pl->ps.lgc_undershaft * 100.0f) / a_lg) : 0.0f;
			row->eff_pct = e_lg;
		}

		snap.lgc_count = snap.player_count;
	}
	else
	{
		for (i = 0; i < snap.player_count; i++)
		{
			gedict_t *pl = players[i];
			ls_kill_row_t *krow = &snap.kill[i];
			ls_item_row_t *irow = &snap.items[i];
			ls_wep_eff_row_t *erow = &snap.wep_eff[i];
			ls_wep_dmg_row_t *drow = &snap.wep_dmg[i];
			ls_wep_simple_row_t *trow = &snap.wep_taken[i];
			ls_wep_simple_row_t *dprow = &snap.wep_drop[i];
			ls_wep_simple_row_t *krow2 = &snap.wep_kill[i];
			ls_wep_simple_row_t *ekrow = &snap.wep_ekill[i];
			ls_damage_row_t *dmrow = &snap.damage[i];
			ls_item_time_row_t *itrow = &snap.item_time[i];
			ls_wep_time_row_t *wtrow = &snap.wep_time[i];
			ls_ctf_row_t *ctfrow = &snap.ctf[i];

			float h_lg = pl->ps.wpn[wpLG].hits;
			float a_lg = pl->ps.wpn[wpLG].attacks;
			float vh_rl = pl->ps.wpn[wpRL].vhits;
			float a_rl = pl->ps.wpn[wpRL].attacks;
			float vh_gl = pl->ps.wpn[wpGL].vhits;
			float a_gl = pl->ps.wpn[wpGL].attacks;
			float h_sng = pl->ps.wpn[wpSNG].hits;
			float a_sng = pl->ps.wpn[wpSNG].attacks;
			float h_ng = pl->ps.wpn[wpNG].hits;
			float a_ng = pl->ps.wpn[wpNG].attacks;
			float h_ssg = pl->ps.wpn[wpSSG].hits;
			float a_ssg = pl->ps.wpn[wpSSG].attacks;
			float h_sg = pl->ps.wpn[wpSG].hits;
			float a_sg = pl->ps.wpn[wpSG].attacks;

			krow->frags = snap.flags.ctf ? (int)(pl->s.v.frags - pl->ps.ctf_points) : (int)pl->s.v.frags;
			krow->kills = (int)pl->kills;
			krow->deaths = snap.flags.ctf ? (int)(pl->ps.ctf_points - pl->deaths) : (int)pl->deaths;
			krow->suicides = (int)pl->suicides;
			krow->friendly = tp ? (int)pl->friendly : LS_INVALID_INT;
			krow->spawn_frags = pl->ps.spawn_frags;
			krow->efficiency = laststats_efficiency(pl);

			irow->ga = pl->ps.itm[itGA].tooks;
			irow->ya = pl->ps.itm[itYA].tooks;
			irow->ra = pl->ps.itm[itRA].tooks;
			irow->mh = pl->ps.itm[itHEALTH_100].tooks;
			irow->quad = tp ? pl->ps.itm[itQUAD].tooks : LS_INVALID_INT;
			irow->pent = tp ? pl->ps.itm[itPENT].tooks : LS_INVALID_INT;
			irow->ring = tp ? pl->ps.itm[itRING].tooks : LS_INVALID_INT;

			erow->lg = a_lg ? (100.0f * h_lg / max(1.0f, a_lg)) : LS_INVALID_FLOAT;
			erow->rl = a_rl ? (100.0f * vh_rl / max(1.0f, a_rl)) : LS_INVALID_FLOAT;
			erow->gl = a_gl ? (100.0f * vh_gl / max(1.0f, a_gl)) : LS_INVALID_FLOAT;
			erow->sng = a_sng ? (100.0f * h_sng / max(1.0f, a_sng)) : LS_INVALID_FLOAT;
			erow->ng = a_ng ? (100.0f * h_ng / max(1.0f, a_ng)) : LS_INVALID_FLOAT;
			erow->ssg = a_ssg ? (100.0f * h_ssg / max(1.0f, a_ssg)) : LS_INVALID_FLOAT;
			erow->sg = a_sg ? (100.0f * h_sg / max(1.0f, a_sg)) : LS_INVALID_FLOAT;

			drow->lg = pl->ps.wpn[wpLG].edamage;
			drow->rl = pl->ps.wpn[wpRL].edamage;
			drow->gl = pl->ps.wpn[wpGL].edamage;
			drow->sng = pl->ps.wpn[wpSNG].edamage;
			drow->ng = pl->ps.wpn[wpNG].edamage;
			drow->ssg = pl->ps.wpn[wpSSG].edamage;
			drow->sg = pl->ps.wpn[wpSG].edamage;

			trow->lg = pl->ps.wpn[wpLG].tooks;
			trow->rl = pl->ps.wpn[wpRL].tooks;
			trow->gl = pl->ps.wpn[wpGL].tooks;
			trow->sng = pl->ps.wpn[wpSNG].tooks;
			trow->ng = pl->ps.wpn[wpNG].tooks;
			trow->ssg = pl->ps.wpn[wpSSG].tooks;

			dprow->lg = pl->ps.wpn[wpLG].drops;
			dprow->rl = pl->ps.wpn[wpRL].drops;
			dprow->gl = pl->ps.wpn[wpGL].drops;
			dprow->sng = pl->ps.wpn[wpSNG].drops;
			dprow->ng = pl->ps.wpn[wpNG].drops;
			dprow->ssg = pl->ps.wpn[wpSSG].drops;

			krow2->lg = pl->ps.wpn[wpLG].kills;
			krow2->rl = pl->ps.wpn[wpRL].kills;
			krow2->gl = pl->ps.wpn[wpGL].kills;
			krow2->sng = pl->ps.wpn[wpSNG].kills;
			krow2->ng = pl->ps.wpn[wpNG].kills;
			krow2->ssg = pl->ps.wpn[wpSSG].kills;

			ekrow->lg = pl->ps.wpn[wpLG].ekills;
			ekrow->rl = pl->ps.wpn[wpRL].ekills;
			ekrow->gl = pl->ps.wpn[wpGL].ekills;
			ekrow->sng = pl->ps.wpn[wpSNG].ekills;
			ekrow->ng = pl->ps.wpn[wpNG].ekills;
			ekrow->ssg = pl->ps.wpn[wpSSG].ekills;

			dmrow->taken = (int)pl->ps.dmg_t;
			dmrow->given = (int)pl->ps.dmg_g;
			dmrow->eweapon = (int)pl->ps.dmg_eweapon;
			dmrow->team = (int)pl->ps.dmg_team;
			dmrow->self = (int)pl->ps.dmg_self;
			dmrow->per_death = (pl->deaths <= 0) ? 99999 : (int)(pl->ps.dmg_t / pl->deaths);

			itrow->ra = (int)pl->ps.itm[itRA].time;
			itrow->ya = (int)pl->ps.itm[itYA].time;
			itrow->ga = (int)pl->ps.itm[itGA].time;
			itrow->quad = tp ? (int)pl->ps.itm[itQUAD].time : LS_INVALID_INT;
			itrow->pent = tp ? (int)pl->ps.itm[itPENT].time : LS_INVALID_INT;
			itrow->ring = tp ? (int)pl->ps.itm[itRING].time : LS_INVALID_INT;

			wtrow->lg = (int)pl->ps.wpn[wpLG].time;
			wtrow->rl = (int)pl->ps.wpn[wpRL].time;
			wtrow->gl = (int)pl->ps.wpn[wpGL].time;
			wtrow->sng = (int)pl->ps.wpn[wpSNG].time;
			wtrow->ng = (int)pl->ps.wpn[wpNG].time;
			wtrow->ssg = (int)pl->ps.wpn[wpSSG].time;

			if (snap.flags.ctf)
			{
				int res = 0;
				int str = 0;
				int hst = 0;
				int rgn = 0;

				if (!snap.flags.ctf_runes)
				{
					res = LS_INVALID_INT;
					str = LS_INVALID_INT;
					hst = LS_INVALID_INT;
					rgn = LS_INVALID_INT;
				}
				else if (duration > 0.0f)
				{
					res = (int)((pl->ps.res_time / duration) * 100.0f);
					str = (int)((pl->ps.str_time / duration) * 100.0f);
					hst = (int)((pl->ps.hst_time / duration) * 100.0f);
					rgn = (int)((pl->ps.rgn_time / duration) * 100.0f);
				}

				ctfrow->pickups = pl->ps.pickups;
				ctfrow->caps = pl->ps.caps;
				ctfrow->returns = pl->ps.returns;
				ctfrow->f_defends = pl->ps.f_defends;
				ctfrow->c_defends = pl->ps.c_defends;
				ctfrow->res = res;
				ctfrow->str = str;
				ctfrow->hst = hst;
				ctfrow->rgn = rgn;
			}
		}

		snap.kill_count = snap.player_count;
		snap.item_count = snap.player_count;
		snap.wep_eff_count = snap.player_count;
		snap.wep_dmg_count = snap.player_count;
		snap.wep_taken_count = snap.player_count;
		snap.wep_drop_count = snap.player_count;
		snap.wep_kill_count = snap.player_count;
		snap.wep_ekill_count = snap.player_count;
		snap.damage_count = snap.player_count;
		snap.item_time_count = snap.player_count;
		snap.wep_time_count = snap.player_count;
		if (snap.flags.ctf)
		{
			snap.ctf_count = snap.player_count;
		}

		if (snap.flags.team || snap.flags.ctf)
		{
			laststats_collect_team_stats(&snap, players, snap.player_count);
		}

		if (!snap.flags.duel)
		{
			laststats_compute_top_stats(&snap, players, snap.player_count);
		}
	}

	handle = std_fwopen(LASTSTATS_FILENAME);
	if (handle < 0)
	{
		return;
	}

	S2di(handle, "{");
	S2di(handle, "\"version\":%d,", snap.version);
	S2di(handle, "\"flags\":{\"midair\":%d,\"instagib\":%d,\"lgc\":%d,\"ctf\":%d,\"team\":%d,\"duel\":%d,"
			"\"ctf_runes\":%d,\"ra\":%d,\"ca\":%d,\"race\":%d,\"socd\":%d,\"time_valid\":%d,"
			"\"overtime\":%d,\"deathmatch\":%d},",
			snap.flags.midair ? 1 : 0,
			snap.flags.instagib ? 1 : 0,
			snap.flags.lgc ? 1 : 0,
			snap.flags.ctf ? 1 : 0,
			snap.flags.team ? 1 : 0,
			snap.flags.duel ? 1 : 0,
			snap.flags.ctf_runes ? 1 : 0,
			snap.flags.ra ? 1 : 0,
			snap.flags.ca ? 1 : 0,
			snap.flags.race ? 1 : 0,
			snap.flags.socd ? 1 : 0,
			snap.flags.time_valid ? 1 : 0,
			snap.flags.overtime ? 1 : 0,
			snap.flags.deathmatch);
	S2di(handle, "\"ca_team1_name\":\"%s\",\"ca_team1_score\":%d,"
			"\"ca_team2_name\":\"%s\",\"ca_team2_score\":%d,",
			json_string(snap.ca_team1_name),
			snap.ca_team1_score,
			json_string(snap.ca_team2_name),
			snap.ca_team2_score);

	laststats_write_players(handle, &snap);

	if (snap.flags.midair)
	{
		S2di(handle, ",");
		laststats_write_midair(handle, &snap);
		S2di(handle, ",");
		laststats_write_midair_kill(handle, &snap);
		S2di(handle, ",");
		laststats_write_top_midair(handle, &snap);
	}
	else if (snap.flags.instagib)
	{
		S2di(handle, ",");
		laststats_write_insta(handle, &snap);
		S2di(handle, ",");
		laststats_write_insta_kill(handle, &snap);
	}
	else if (snap.flags.lgc)
	{
		S2di(handle, ",");
		laststats_write_lgc(handle, &snap);
	}
	else
	{
		S2di(handle, ",");
		laststats_write_kill(handle, &snap);
		S2di(handle, ",");
		laststats_write_items(handle, &snap);
		S2di(handle, ",");
		laststats_write_wep_eff(handle, &snap);
		S2di(handle, ",");
		laststats_write_wep_dmg(handle, &snap);
		S2di(handle, ",");
		laststats_write_wep_simple(handle, "wep_taken", snap.wep_taken, snap.wep_taken_count);
		S2di(handle, ",");
		laststats_write_wep_simple(handle, "wep_drop", snap.wep_drop, snap.wep_drop_count);
		S2di(handle, ",");
		laststats_write_wep_simple(handle, "wep_kill", snap.wep_kill, snap.wep_kill_count);
		S2di(handle, ",");
		laststats_write_wep_simple(handle, "wep_ekill", snap.wep_ekill, snap.wep_ekill_count);
		S2di(handle, ",");
		laststats_write_damage(handle, &snap);
		S2di(handle, ",");
		laststats_write_item_time(handle, &snap);
		S2di(handle, ",");
		laststats_write_wep_time(handle, &snap);
		if (snap.flags.ctf)
		{
			S2di(handle, ",");
			laststats_write_ctf(handle, &snap);
		}
		if (snap.flags.team || snap.flags.ctf)
		{
			S2di(handle, ",");
			laststats_write_team_items(handle, &snap);
			S2di(handle, ",");
			laststats_write_team_weapons(handle, &snap);
			S2di(handle, ",");
			laststats_write_team_damage(handle, &snap);
			if (snap.flags.ctf)
			{
				S2di(handle, ",");
				laststats_write_team_ctf(handle, &snap);
			}
		}
		if (!snap.flags.duel)
		{
			S2di(handle, ",");
			laststats_write_top(handle, &snap);
		}
	}

	S2di(handle, "}");
	std_fclose(handle);
}

// Load laststats.json and format /laststats output.
qbool LastStatsJsonPrint(gedict_t *ed)
{
	laststats_snapshot_t snap;
	int i;
	qbool has_ghost = false;

	if (!laststats_json_load(&snap))
	{
		return false;
	}

	if (!laststats_snapshot_valid(&snap))
	{
		return false;
	}

	if (snap.version == LASTSTATS_JSON_VERSION)
	{
		if (snap.flags.race)
		{
			G_sprint(ed, 2, "Race stats unavailable for /laststats\n");
			return true;
		}

		laststats_print_players_stats(&snap, ed);

		if (!snap.flags.midair)
		{
			if (snap.flags.team || snap.flags.ctf)
			{
				laststats_print_summary_tp_stats(&snap, ed);
			}

			if (!snap.flags.duel)
			{
				laststats_print_top_stats(&snap, ed);
			}
		}
		else
		{
			laststats_print_top_midair(&snap, ed);
		}

		if (snap.flags.team || snap.flags.ctf)
		{
			laststats_print_team_scores(&snap, ed);
		}

		for (i = 0; i < snap.player_count; i++)
		{
			if (snap.players[i].ghost)
			{
				has_ghost = true;
				break;
			}
		}

		if (has_ghost)
		{
			G_sprint(ed, 2, "\n\203 - %s player\n\n", redtext("disconnected"));
		}

		return true;
	}

	if (snap.flags.midair)
	{
		G_sprint(ed, 2, "\n%s   |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Midair statistics"), redtext("Tot"), redtext("Bro"),
				redtext("Sil"), redtext("Gol"), redtext("Pla"), redtext("Sto"),
				redtext("Bon"), redtext("Max H"), redtext("Avg H"));
		laststats_print_by_team(&snap, ed, laststats_print_midair_row);

		G_sprint(ed, 2, "\n%s     |%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\237\n",
				redtext("Kill statistics"), redtext(" Frag"), redtext("SpwnF"),
				redtext(" Strk"), redtext(" RL %"));
		laststats_print_by_team(&snap, ed, laststats_print_midair_kill_row);

		laststats_print_top_midair(&snap, ed);
		return true;
	}

	if (snap.flags.instagib)
	{
		G_sprint(ed, 2, "\n%s |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\237\n",
				redtext("Instagib statistics"), redtext("Coil"), redtext(" Axe"), redtext("Stmp"),
				redtext("Mult"), redtext("MMax"), redtext(" Air"), redtext("TotH"),
				redtext("MaxH"), redtext("Ring"));
		laststats_print_by_team(&snap, ed, laststats_print_insta_row);

		G_sprint(ed, 2, "\n%s     |%s|%s|%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Kill statistics"), redtext("Point"), redtext(" Frag"), redtext("Death"),
				redtext("SpwnF"), redtext(" Strk"), redtext(" CG %"));
		laststats_print_by_team(&snap, ed, laststats_print_insta_kill_row);
		return true;
	}

	if (snap.flags.lgc)
	{
		G_sprint(ed, 2, "\n%s     |%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\237\n",
				redtext("Kill statistics"), redtext("Score"), redtext(" Over"), redtext("Under"),
				redtext("  Eff"));
		laststats_print_by_team(&snap, ed, laststats_print_lgc_row);
		return true;
	}

	G_sprint(ed, 2, "\n%s     |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Kill Statistics"), redtext(" Frag"), redtext(" Kill"), redtext("Death"),
			redtext("Suici"), redtext("TKill"), redtext("SpwnF"), redtext("Effic"));
	laststats_print_by_team(&snap, ed, laststats_print_kill_row);

	G_sprint(ed, 2, "\n%s          |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Item Taken"), redtext("   GA"), redtext("   YA"), redtext("   RA"),
			redtext("   MH"), redtext("   Q"), redtext("   P"), redtext("   R"));
	laststats_print_by_team(&snap, ed, laststats_print_item_row);

	G_sprint(ed, 2, "\n%s   |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Weapon Efficiency"), redtext("  LG%"), redtext("  RL%"), redtext("  GL%"),
			redtext(" SNG%"), redtext("  NG%"), redtext(" SSG%"), redtext("  SG%"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_eff_row);

	G_sprint(ed, 2, "\n%s       |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon Damage"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"), redtext("   SG"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_dmg_row);

	G_sprint(ed, 2, "\n%s        |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\237\n",
			redtext("Weapon Taken"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_taken_row);

	G_sprint(ed, 2, "\n%s      |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\237\n",
			redtext("Weapon Dropped"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_drop_row);

	G_sprint(ed, 2, "\n%s        |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\237\n",
			redtext("Weapon Kills"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_kill_row);

	G_sprint(ed, 2, "\n%s |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\237\n",
			redtext("Enemy Weapon Killed"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_ekill_row);

	G_sprint(ed, 2, "\n%s   |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Damage statistics"), redtext("Taken"), redtext("Given"), redtext("EWeap"),
			redtext(" Team"), redtext(" Self"), redtext("ToDie"));
	laststats_print_by_team(&snap, ed, laststats_print_damage_row);

	G_sprint(ed, 2, "\n%s          |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Item times"), redtext("   RA"), redtext("   YA"), redtext("   GA"),
			redtext(" Quad"), redtext(" Pent"), redtext(" Ring"));
	laststats_print_by_team(&snap, ed, laststats_print_item_time_row);

	G_sprint(ed, 2, "\n%s        |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon times"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));
	laststats_print_by_team(&snap, ed, laststats_print_wep_time_row);

	if (snap.flags.ctf)
	{
		G_sprint(ed, 2, "\n%s      |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("CTF statistics"), redtext("Fla"), redtext("Cap"), redtext("Ret"),
				redtext("DFl"), redtext("DCa"), redtext("Res"), redtext("Str"), redtext("Hst"),
				redtext("Rgn"));
		laststats_print_by_team(&snap, ed, laststats_print_ctf_row);
	}

	if (snap.flags.team || snap.flags.ctf)
	{
		laststats_print_team_summary(&snap, ed);
	}

	if (!snap.flags.duel)
	{
		laststats_print_top_stats(&snap, ed);
	}

	for (i = 0; i < snap.player_count; i++)
	{
		if (snap.players[i].ghost)
		{
			has_ghost = true;
			break;
		}
	}

	if (has_ghost)
	{
		G_sprint(ed, 2, "\n\203 - %s player\n\n", redtext("disconnected"));
	}

	return true;
}

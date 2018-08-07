#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "common/hercules.h"
#include "common/HPMi.h"

#include "common/db.h"
#include "common/ers.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/random.h"

#include "common/mmo.h"
#include "common/socket.h"
#include "common/mapindex.h"
#include "common/strlib.h"
#include "common/cbasetypes.h"
#include "common/timer.h"
#include "common/showmsg.h"
#include "common/conf.h"
#include "common/sql.h"
#include "common/utils.h"

#include "map/atcommand.h"
#include "map/battle.h"
#include "map/battleground.h"
#include "map/channel.h"
#include "map/clif.h"
#include "map/chrif.h"
#include "map/elemental.h"
#include "map/guild.h"
#include "map/homunculus.h"
#include "map/instance.h"
#include "map/intif.h"
#include "map/itemdb.h"
#include "map/mail.h"
#include "map/map.h"
#include "map/mapreg.h"
#include "map/mercenary.h"
#include "map/messages.h"
#include "map/mob.h"
#include "map/npc.h"
#include "map/party.h"
#include "map/pc.h"
#include "map/pet.h"
#include "map/quest.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/unit.h"

#include "HPMHooking.h"
#include "common/HPMDataCheck.h"

#define EBG_MAP
#include "eBG_common.h"

HPExport struct hplugin_info pinfo = {
	"ExtendedBG",		// Plugin name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"BitBucket Version",// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};
/*
Changelogs:	See BitBucket Changelog Issue.
*/

/// Some Macro Definations

/**
 * Checks if given pointer is null
 * @param data Any pointer.
 **/
#define checkNull_void(data)	\
		if ((data) == NULL) { \
			return; \
		}

/**
 * Checks if given pointer is null
 * @see checkNull_void
 * @param data Any pointer.
 * @return NULL if it is NULL.
 **/
#define checkNull(data)	\
		if ((data) == NULL) { \
			return NULL; \
		}

/**
 * It Fetches the data corresponding to constant,
 * saves it in variable(v_name) and adds value specified.
 * @param sd map_session_data structure
 * @param v_name Variable Name(must be a pointer)
 * @param constant Which data to be altered
 * @param value Value to be added to variable
 * @param ptr_type DataType of the variable
 * Note: int64 is typecasted to int in ShowError
 **/
#define SET_VARIABLE_ADD(sd, v_name, constant, value, ptr_type) (v_name) = GET_VARIABLE_SIZE((sd), (constant), true, ptr_type); \
		if ((v_name) == NULL) { \
			ShowError("SET_VARIABLE_ADD: Cannot Add %d to %d (AccountID:%d, Name:%s)\n", (int)value, constant, (sd)->status.account_id, (sd)->status.name); \
		} else { \
			*(v_name) = *((v_name))+(value); \
		}

/**
 * It Fetches the data corresponding to constant,
 * saves it in variable(v_name) and replace value specified.
 * @param v_name Variable Name(must be a pointer)
 * @param constant Which data to be altered
 * @param value Value to be added to variable
 * @param ptr_type DataType of the variable
 **/
#define SET_VARIABLE_REPLACE_(v_name, constant, value, ptr_type) \
		(v_name) = GET_VARIABLE_SIZE(sd, (constant), true, ptr_type); \
		if ((v_name) == NULL) { \
			ShowError("SET_VARIABLE_REPLACE_: Cannot Replace %ld in %ld.\n", (long int)value, (long int)constant); \
		} else { \
			*(v_name) = (value); \
		}

/**
 * It Fetches the data corresponding to constant,
 * saves it in variable(v_name) and replace value specified.
 * @param v_name Variable Name(must be a pointer)
 * @param constant Which data to be altered
 * @param value Value to be added to variable
 * @param i Index Value to increment.
 * @param ptr_type DataType of the variable
 **/
#define SET_VARIABLE_REPLACE(v_name, constant, value, i, ptr_type) \
		(v_name) = NULL; \
		(v_name) = GET_VARIABLE_SIZE(sd, (constant), true, ptr_type); \
		if ((v_name) == NULL) { \
			ShowError("SET_VARIABLE_REPLACE: Cannot Replace %ld in %ld.\n", (long int)value, (long int)constant); \
		} else { \
			*(v_name) = (value); \
		} \
		i++

/**
 * Converts GuildID to VirtualGuildID
 * @param guild_id Original Guild ID (1 to TOTAL_GUILD)
 * @see TOTAL_GUILD
 **/
#ifdef VIRT_GUILD
	#define GET_EBG_GUILD_ID(guild_id) INT_MAX - (TOTAL_GUILD + 1) + (guild_id)
	#define GET_ORIG_GUILD_ID(ebg_guild_id) ebg_guild_id - (INT_MAX - TOTAL_GUILD - 1)
#else
	#define GET_EBG_GUILD_ID(guild_id) EBG_GUILDSTART + (guild_id)
	#define GET_ORIG_GUILD_ID(ebg_guild_id) (ebg_guild_id) - EBG_GUILDSTART
#endif

/**
 * Saves the value of map(if its bg, true, else false) on the is_bg variable
 * @param sd Map_Session_Data
 * @param sd_data sd_p_data structure
 * @param is_bg Variable name where data will be saved
 * @see sd_p_data
 **/
#define EBG_OR_WOE(sd, sd_data, is_bg) \
			if (map->list[(sd)->bl.m].flag.battleground && ((sd)->bg_id || (sd_data && sd_data->eBG))) {  \
				is_bg = MAP_IS_BG; \
			} else if ((sd)->status.guild_id && map_flag_gvg2((sd)->bl.m)) { \
				is_bg = MAP_IS_WOE; \
			} else { \
				is_bg = MAP_IS_NONE; \
			}

/**
 * Returns the Address of the said type
 * @param sd Map_Session_Data
 * @param type Check Player_Data enum
 * @param create Create the memory?
 * @param ptr_type DataType of the variable
 * @return Address of the type.
 **/				
#define GET_VARIABLE_SIZE(sd, type, create, ptr_type) (ptr_type *)get_variable((sd), type, create)

/* For the Time Being, till I rearrange all the Functions to suitable order. **/
// Move to Header File
int viewpointmap_sub(struct block_list *bl, va_list ap);
void pc_addpoint_bg(struct map_session_data *sd, int count, int type, bool leader, int extra_count);
void bg_team_rewards(int bg_id, int nameid, int amount, int kafrapoints, int quest_id, const char *var, int add_value, int bg_a_type, int bg_win_loss);
int bg_hp_loss_function(int tid, int64 tick, int id, intptr_t data);
int bg_sp_loss_function(int tid, int64 tick, int id, intptr_t data);
void bg_save_data(struct map_session_data *sd);
void *get_variable(struct map_session_data *sd, int type, bool create);
int get_variable_(struct map_session_data *sd, int type, bool create, int retVal);
void bg_load_char_data(struct map_session_data *sd);
int eBG_Guildadd(struct map_session_data *sd, struct guild* g);
int eBG_Guildremove(struct map_session_data *sd, struct guild* g);
int update_bg_ranking(struct map_session_data *sd, int type2);
int bg_e_team_join(int bg_id, struct map_session_data *sd, int guild_id);
void send_bg_memberlist(struct map_session_data *sd);
void bg_send_char_info(struct map_session_data *sd);
int bg_timer_function(int tid, int64 tick, int id, intptr_t data);
void eBG_script_bg_end(struct map_session_data *sd, int bg_type);
bool ebg_clear_hpsp_timer(struct map_session_data *sd, int type);

/**
 * Battleground Result
 **/
enum Battleground_Condition {
	BGC_WON = 0,        ///< Won the Match
	BGC_LOSS = 1,       ///< Loss the Match
	BGC_TIE = 2,        ///< Match Tie.
};

/**
 * Types of BattleGrounds
 **/ 
enum Battleground_Types {
	BGT_COMMON = -1,    ///< Common - Used for Saving all Common BG Values.
	BGT_CTF = 0,        ///< Flavius CTF
	BGT_BOSS = 1,       ///< Tierra Bossnia
	BGT_TI,             ///< Tierra Triple Infierno
	BGT_EOS,            ///< Tierra Valley Eye of Storm
	BGT_TD,             ///< Flavius TeamDeathMatch
	BGT_SC,             ///< Flavius StoneControl
	BGT_CONQ,           ///< Conquest
	BGT_RUSH,           ///< Rush
	BGT_DOM,            ///< Tierra Domination
	/// Custom Types
	BGT_TOUCHDOWN,      ///< TouchDown
	BGT_MAX             ///< Max BG Type.
};

/// BattleConf Settings Start
int bg_reward_rates = 100;      ///< Reward Rates? (100=Normal)(Default: 100)
int bg_ranked_mode = 1;         ///< Enable Rank Mode? (1=Yes, 0=No)(Default: 1)
int bg_max_rank_game = 50;      ///< Total Ranked Matches in a Day(Default: 50)
int bg_rank_bonus = 1;          ///< Enable Rank Bonus (1=Yes, 0=No)(Default: 1)
int bg_rank_rates = 150;        ///< Ranked Games Extra Rewards (100 = Normal)(Default: 150)
int bg_queue_townonly = 1;      ///< Can Only Join from Town (1=Yes, 0=No)(Default: 1)
int bg_idle_announce = 30;      ///< Idle time after which player is marked afk(Default: 300s)
int bg_kick_idle = 0;           ///< Auto Kick Idle Players(Default: 0)
int bg_reportafk_leader = 1;    ///< Only Leader can use \@reportafk(Default: 1)
int bg_log_kill = 7;            ///< Log Kills, 0=None, 1=Log BG Kills, 2= Log WoE Kills, 4 = Log Kills on all other maps.
int bg_reserved_char_id = INT_MAX-100;  ///< Reserved CharID for BG Items
int woe_reserved_char_id = INT_MAX-101; ///< Reserved CharID for WoE Items

/**
 * Battleground Data
 **/
struct bg_extra_info {
	int64 creation_time;                 ///< Creation Time of BG
	bool init;                           ///< Is Creation Time Initialized?
	bool reveal_pos;                     ///< Reveal Positions?
	bool reveal_flag;                    ///< Reveal Flag
#ifdef LEADER_INT
	int leader;                          ///< Leader CharacterID
#else
	struct bg_leader_info *leader;       ///< Leader Information
#endif
	short points;                        ///< Team Points
#ifdef VIRT_GUILD
	struct guild *g;                     ///< Guild Information
#endif
};

/**
 * Battleground Leader information are saved here
 **/
struct bg_leader_info {
	int char_id;                     ///< CharacterID
};

/**
 * Additional Monster Information are saved here
 **/
struct mob_extra_data {
	bool immunity;                   ///< Takes 1 damage only.
	bool can_move;                   ///< Can Monster Move
};

int timer_uid = 0;                   ///< Timer UniqueID, starts from 0, and increments itself

/**
 * BG Timer Operations
 * Constants related to Operations on Timer
 * @see bg_timerdb_struct
 **/
enum e_bg_timer {
	BG_T_O_MIN           = 0x0000,      ///< Minimum Operation Constant
	BG_T_O_AREAHEALER    = 0x0001,      ///< AreaHeal
	BG_T_O_AREAWARP      = 0x0002,      ///< AreaWarp
	BG_T_O_ADDTIMER      = 0x0004,      ///< Add Another Timer
	BG_T_O_SHOWEFFECT    = 0x0008,      ///< ShowEffect on NPC
	BG_T_O_SELFDELETE    = 0x0010,      ///< Self Delete the Timer?
	BG_T_O_ADDSELF       = 0x0020,      ///< Add Same time to Self
	BG_T_O_ADDTIMERSELF  = 0x0040,      ///< Needs BG_T_O_ADDTIMER, so it adds its timer + own timer for next iteration.
	BG_T_O_VIEWPOINTMAP  = 0x0080,      ///< ViewPointMap ScriptCommand
	BG_T_O_NPC_CONDITION = 0x0100,      ///< Check NPC Variable
	BG_T_O_REVEAL_MOB    = 0x0200,      ///< bg_revealmob(Uses npc_value(0=ID,1=Color))
	/// Unused:            0x0400,
	BG_T_O_ARITHMETIC_OP = 0x0800,      ///< Perform Arithmetic Operations on npc_extra_data
	BG_T_O_DO_EVENT      = 0x1000,      ///< Calls an EventLabel.
	BG_T_O_MAPANNOUNCE   = 0x2000,      ///< MapAnnounce.
	BG_T_O_DELAY         = 0x4000,      ///< Delay 2 Seconds
	BG_T_O_SCRIPT        = 0x8000,      ///< Execute Script
	BG_T_O_MAX,                         ///< Max Operation Constant
};

/**
 * BG Timer Data
 * Timer Performs various tasks which are saved here
 **/
struct bg_timerdb_struct {                     ///< Timer Unique ID to get this struct.
	int operations;                            ///< enum e_bg_timer.
	int timer_id;                              ///< Timer ID
	int timer_;                                ///< Only if interval = 0
	short effect[BMAX_EFFECT];                 ///< BG_T_O_SHOWEFFECT
	bool interval;                             ///< is timer a interval?
	short random_timer[2];                     ///< Random Time to start(20-50sec)? (should be in seconds.)
	short link_fail;                           ///< How many times link have failed(so that we don't mess up with times)
	/// Operations
	struct bg_areaheal_struct *areaheal;       ///< AreaHeal Data
	struct bg_areawarp_struct *areawarp;       ///< AreaWarp Data
	struct bg_timer_struct *timer;             ///< Additional Timer's Data
	struct bg_viewpoint_struct *viewpointmap;  ///< ViewPointMap Data
	struct bg_mapannounce_struct *mapannounce; ///< MapAnnounce Data
	/// Common
	struct bg_cond_struct *npcs;               ///< Conditions and Operations related to conditions are stored here
	struct bg_arith_struct *arith;             ///< Arithmetic stuff on bg_npc_data
	struct npc_data *nd;                       ///< npc_data to access_variable
	/** ToDo: Convert to bit form **/
	struct {
		int operation_independent;             ///< This Operation will be executed independently
		int operation_dependent;               ///< This Operation will be executed only if operation_independent has some return value.
		int retVal;                            ///< Return Value by operation_independent is stored here
	} link;
	/// Nested Timer Structure
	int parent_uid;                            ///< Parent Timer UniqueID (Only set on child)
	int child_uid[MAX_CHILD_TIMER];            ///< Child UID's
	/// Script
	struct script_code *script;                ///< Script is stored here.
};

/**
 * Used with NPC Conditions and Other,
 * to perform certain operations on data.
 * @see bg_arith_struct
 * @see bg_cond_struct
 **/
enum e_bg_arith_operation {                    ///< Arithmetic Operation Constants
	EBG_A_ADD = 0,                             ///< Arithmetic Add Constant, Adds the number
	EBG_A_SUBSTRACT,                           ///< Arithmetic Subtract Constant, Subtract the number
	EBG_A_MULTIPLY,                            ///< Arithmetic Multiply Constant, Multiplies the Number
	EBG_A_DIVIDE,                              ///< Arithmetic Divide Constant, Divides the Number
	EBG_A_ZEROED,                              ///< Arithmetic Zero, Sets the value to 0
	EBG_A_MAX                                  ///< MAX Arithmetic Constant, Referring to Maximum ID
};

/**
 * Used by Timer
 * to perform certain operations on data.
 * @see bg_timerdb_struct
 **/
struct bg_arith_struct {
	char name[NAME_LENGTH];                    ///< Stores NPC Name
	int value;                                 ///< Second Value, in which operation will be performed
	/**
	 * Index of the First Value (Stored in some index of other structure)
	 * @see npc_extra_data
	 **/
	int index_at;
	/**
	 * Operation to be Performed
	 * @see e_bg_arith_operation
	 **/
	int operation;
};

/**
 * Used by Timer
 * Heals Certain Area by some Percentage
 * @see bg_timerdb_struct
 **/
struct bg_areaheal_struct {
	short x1;                                   ///< Co-Ordinate x1
	short x2;                                   ///< Co-Ordinate x2
	short y1;                                   ///< Co-Ordinate y1
	short y2;                                   ///< Co-Ordinate y2
	short hp;                                   ///< Percentage of HP to be healed
	short sp;                                   ///< Percentage of SP to be healed
	int m;                                      ///< MapID
//	int uid;
};

/**
 * Used by Timer
 * Warp Certain Area to other Location
 * @see bg_timerdb_struct
 **/
struct bg_areawarp_struct {
	short x0;                                   ///< Co-Ordinate x1 (Source CoOrdinate)
	short x1;                                   ///< Co-Ordinate x2 (Source CoOrdinate)
	short x2;                                   ///< Co-Ordinate x1 (Destination CoOrdinate)
	short x3;                                   ///< Co-Ordinate x2 (Destination CoOrdinate)
	short y0;                                   ///< Co-Ordinate y1 (Source CoOrdinate)
	short y1;                                   ///< Co-Ordinate y2 (Source CoOrdinate)
	short y2;                                   ///< Co-Ordinate y1 (Destination CoOrdinate)
	short y3;                                   ///< Co-Ordinate y2 (Destination CoOrdinate)
	int m;                                      ///< Source MapID
//	int uid;
	int index;                                  ///< Destination MapID
};

/**
 * Used by Timer
 * Adds Additional Timer
 * @see bg_timerdb_struct
 **/
struct bg_timer_struct {
	int timer_;              ///< Timer Duration
	int operations;          ///< Operations by this Timer
};

/**
 * Used by Timer
 * Data for mapannounce
 * @see bg_timerdb_struct
 **/
struct bg_mapannounce_struct {
	char mes[200];           ///< Message to be announce
	char fontColor[10];      ///< FontColor (0xRRGGBB)
	int16 m;                 ///< MapID
	int flag;                ///< Flag
	bool announce;           ///< Unknown
};

/**
 * Used by Timer
 * Data for ViewPointMap
 * @see bg_timerdb_struct
 **/
struct bg_viewpoint_struct {
	int type;                ///< ViewPoint Map
	short x;                 ///< X CoOrdinate
	short y;                 ///< Y CoOrdinate
	short m;                 ///< MapID
	int id;                  ///< ViewPoint ID
	int color;               ///< Color
};

enum { /// enum for bg_cond_struct::condition
	EBG_CONDITION_LESS = 0x1,
	EBG_CONDITION_EQUAL = 0x2,
	EBG_CONDITION_GREATER = 0x4,
	EBG_CONDITION_ELSE = 0x8,
};

enum ebg_hpsp_timer { /// enum for Referring to HP/SP Time
	EBG_HP_TIME = 0x1,
	EBG_SP_TIME = 0x2,
	EBG_EXTRA_TIME = 0x4,
};

/**
 * Used by Timer
 * Additional NPC Data
 * @see bg_timerdb_struct
 **/
struct npc_extra_data {
	int value[BEXTRA_MAX_VALUE];             ///< Values of NPC's are stored here
};

/**
 * Mapflags Variable are stored here
 **/
struct ebg_mapflags {
	unsigned bg_3 : 1;           ///< equivalent to battleground 3
	unsigned no_ec : 1;          ///< No Emergency Call
	unsigned short bg_topscore;	 ///< Highest Score allowed
};

#ifdef TIMER_LOG

struct timer_log {
	char log[30];
	int sequence;
	bool init;
	int removed;
};

struct timer_log tdblog[1000];
struct timer_log rtdblog[1000];
int timer_seq = 1;

#endif

struct DBMap *bg_timer_db = NULL;                        ///< Timer Data
#ifdef VIRT_GUILD
	struct guild bg_guild[TOTAL_GUILD];                      ///< BG Guild Data
#endif
void bg_save_all(struct map_session_data *sd, enum Battleground_Types bg_type);

/**
 * Creates memory for hooking map_data
 * @param mapflag Variable to be set
 * @param value Value of variable to be set
 **/
#define CREATE_MAPD(mapflag, value) \
		if (!( mf_data = getFromMAPD(&map->list[m], 0))) { \
			CREATE(mf_data, struct ebg_mapflags, 1); \
			addToMAPD(&map->list[m], mf_data, 0, true); \
		} \
		init = true; \
		mf_data->mapflag = value

/**
 * Searches for BG Data(hooked in battleground_data)
 * @param bgd battleground_data
 * @param create if true, it allocates memory if not yet allocated
 * @return data as (bg_extra_info) 
*/
struct bg_extra_info* bg_extra_create(struct battleground_data* bgd, bool create)
{
	struct bg_extra_info* bg_data_t;

	if (bgd == NULL)
		return NULL;

	bg_data_t = getFromBGDATA(bgd, 0);
	if (bg_data_t == NULL && create) {
		CREATE(bg_data_t, struct bg_extra_info, 1);
		memset(bg_data_t, 0, sizeof(struct bg_extra_info));
		addToBGDATA(bgd, bg_data_t, 0, true);
#ifndef LEADER_INT
		CREATE(bg_data_t->leader, struct bg_leader_info, 1);
#endif
	}
	return bg_data_t;
}

/**
 * Searches for BGTimer Data
 * @param uid Unique TimerID
 * @return bg_timer data as (bg_timerdb_struct)
 **/
struct bg_timerdb_struct* bg_timer_search(int uid)
{
	return (struct bg_timerdb_struct*)idb_get(bg_timer_db, uid);
}

/**
 * Searches for NPC Data(hooked in npc_data)
 * @param nd npc_data
 * @param create if true, it allocates memory if not yet allocated
 * @return data as (npc_extra_data) 
 **/
struct npc_extra_data* npc_e_search(struct npc_data* nd, bool create)
{
	struct npc_extra_data *data;

	if (nd == NULL)
		return NULL;

	data = getFromNPCD(nd,0);
	if (data == NULL && create) {
		CREATE(data, struct npc_extra_data, 1);
		memset(data, 0, sizeof(struct npc_extra_data));
		addToNPCD(nd,data,0,true);
	}
	return data;
}

/**
 * Searches for Monster Data(hooked in mob_data)
 * @param md mob_data
 * @param create if true, it allocates memory if not yet allocated
 * @return data as (mob_extra_data) 
 **/
struct mob_extra_data* mob_e_search(struct mob_data* md, bool create)
{
	struct mob_extra_data *data;

	if (md == NULL)
		return NULL;

	data = getFromMOBDATA(md, 0);
	if (data == NULL && create) {
		CREATE(data, struct mob_extra_data, 1);
		memset(data, 0, sizeof(struct mob_extra_data));
		data->immunity = false;
		data->can_move = true;
		addToMOBDATA(md, data, 0, true);
	}
	return data;
}

/**
 * Searches for Player Data(hooked in map_session_data)
 * @param sd Map_Session_Data
 * @param create if true, it allocates memory if not yet allocated
 * @return data as (sd_p_data)
 **/
struct sd_p_data* pdb_search(struct map_session_data *sd, bool create)
{
	struct sd_p_data *data;
	
	if (sd == NULL)
		return NULL;

	data = getFromMSD(sd, 0);

	if (data == NULL && create) {
		CREATE(data, struct sd_p_data, 1);
		memset(data, 0, sizeof(struct sd_p_data));
		addToMSD(sd, data, 0, true);
	}
	
	return data;
}

/**
 * Saves Player Data, called by other function.
 * Also sets the timer to be saved.
 * @param sd Map_Session_Data
 **/
void bg_save_data(struct map_session_data *sd)
{
	if (sd == NULL)
		return;
	bg_save_all(sd,BGT_MAX);
}


/**
 * chrif_save preHooked
 * @param sd Map_Session_Data
 * @param flag Saving Flag
 * Flag = 1: Character is quitting
 * Flag = 2: Character is changing map-servers
 * @return true
 **/
bool bg_save_timer(struct map_session_data *sd, int *flag)
{
	nullpo_ret(sd);
	
	bg_save_data(sd); // Save the Player's Data.
	return 1;
}

#ifdef TIMER_LOG

void remove_timerlog(int uid, char log[]) {
	if (!tdblog[uid].init) {
		ShowError("remove_timerlog: Timer isn't found : %d(Reason:%s, Sequence: %d)\n",uid,log,timer_seq);
		return;
	}
	eShowDebug("remove_timerlog: Timer %d Removed\n",uid);
	sprintf(rtdblog[uid].log,"%s",tdblog[uid].log);
	//tdblog[uid].log = log;
	rtdblog[uid].sequence = tdblog[uid].sequence;
	rtdblog[uid].init = tdblog[uid].init;
	if (rtdblog[uid].removed > 0) {
		rtdblog[uid].removed++;
	} else {
		rtdblog[uid].removed = 1;
	}
	tdblog[uid].init = false;
	tdblog[uid].sequence = 0;
}

void insert_timerlog(int uid, char log[]) {
	if (tdblog[uid].sequence > 0 && tdblog[uid].init) {
		ShowError("insert_timerlog: Timer Already Free'd : %d(Reason:%s, Sequence: %d:%d)\n",uid,log,tdblog[uid].sequence,timer_seq);
		return;
	}
	sprintf(tdblog[uid].log,"%s",log);
	//tdblog[uid].log = log;
	tdblog[uid].sequence = timer_seq++;
	tdblog[uid].init = true;
	tdblog[uid].removed = 0;
	ShowDebug("insert_timerlog: Inserted %d(Reason:%s, Sequence: %d)\n",uid,log,timer_seq);
}

void check_timerlog(int uid) {
	if (tdblog[uid].init || tdblog[uid].sequence > 0) {
		ShowError("check_timerlog: Heap-use-after-free : %d(Reason:%s, Sequence: %d:%d)\n",uid, tdblog[uid].log, tdblog[uid].sequence, timer_seq);
		return;
	} else {
		if (!tdblog[uid].init && rtdblog[uid].sequence > 0) {
			ShowDebug("check_timerlog: Previously been removed,\n");
			ShowDebug("Log: %d:%d:%d, Reason: %s, Removed: %d\n",uid, rtdblog[uid].sequence, timer_seq, rtdblog[uid].log, rtdblog[uid].removed);
		}
		ShowDebug("check_timerlog: No Log found for UID:%d\n",uid);
	}
}
#endif

/**
 * Free's the TimerData
 * @param tdb TimerDB Struct
 * @param bg_timer_uid Unique TimerID
 * @param op Which Operations to perform(1=Delete Timer,2=Free the Struct,4=Remove from DBMap)
 **/
void bg_timer_free(struct bg_timerdb_struct* tdb,int bg_timer_uid, int op)
{
	if (tdb == NULL) {
		eShowDebug("bg_timer_db: Timer not found..\n");
		return;
	}
	if (op & 0x1) {
		if (tdb != NULL) {
			if (tdb->timer_id != INVALID_TIMER) {
				eShowDebug("Timer_ID(To be Free'd): %d\n",tdb->timer_id);
				timer->delete(tdb->timer_id, bg_timer_function);
				tdb->timer_id = INVALID_TIMER;
				eShowDebug("Timer_ID Free'd\n");
			}
		}
	}
	if ((op & 0x8) && tdb != NULL) {
		int i;
		struct bg_timerdb_struct *child_tdb = NULL;
		for (i = 0; i < MAX_CHILD_TIMER; i++) {
			if (tdb->child_uid[i] <= 0)
				continue;
			child_tdb = bg_timer_search(tdb->child_uid[i]);
			if (child_tdb != NULL) {
				bg_timer_free(child_tdb, tdb->child_uid[i], 0x7);
			}
			tdb->child_uid[i] = -1;
		}
	}
	if (op & 0x4) {
		if (tdb != NULL) {
			if (tdb->areaheal != NULL) {
				aFree(tdb->areaheal);
			}
			if (tdb->areawarp != NULL) {
				aFree(tdb->areawarp);
			}
			if (tdb->timer != NULL) {
				aFree(tdb->timer);
			}
			if (tdb->viewpointmap != NULL) {
				aFree(tdb->viewpointmap);
			}
			if (tdb->npcs != NULL) {
				aFree(tdb->npcs);
			}
			if (tdb->arith != NULL) {
				aFree(tdb->arith);
			}
			if (tdb->mapannounce != NULL) {
				aFree(tdb->mapannounce);
			}
			if (tdb->script != NULL) {
				script->free_code(tdb->script);
				tdb->script = NULL;
			}
			aFree(tdb);
			tdb = NULL;
		}
	}
	if (op & 0x2) {
		if (tdb != NULL && !(op & 4)) {
			aFree(tdb);
			tdb = NULL;
		}
		idb_remove(bg_timer_db, bg_timer_uid);
	}
}

/**
 * Generates an unique TimerID to be used
 * @return the Free Timer UniqueID
 **/
int bg_get_unique_timer(void)
{
	if (timer_uid >= INT_MAX) {
		timer_uid = 0;
	}
	do {
		timer_uid++;
	} while (bg_timer_search(timer_uid) != NULL);
	return timer_uid;
}

/**
 * Creates a DBMap of Timer function.
 * timer performs various operations.
 * @param timer_ Time after which timer will execute
 * @param interval Add timer as interval(0/1)
 * @param oid Script OID
 * @param parent_uid Parent Unique Timer ID, Needed in case if another child timer is added
 * @param op Operations to Replace with
 *		If op == -1, Don't replace timer_ in the tdb.
 * @param retUID Return UID?(true=Return UID,false=Return TimerID)
 * @param tdb TimerDB Struct , to copy if provided.
 * @param rnd_times Array of range of Random timer, (in seconds)
 * @return Unique TimerID if retUID is provided, as returns TimerID
 **/
int bg_timer_create(int timer_, int interval, int oid, int parent_uid, int op, bool retUID, struct bg_timerdb_struct* tdb, short rnd_times[2])
{
	struct bg_timerdb_struct* etimer_db = NULL;
	int uid_ = 0;
	bool random = false;
	int orig_timer = timer_;
	bool delay = false;
	
	if (tdb != NULL && parent_uid > 0) {
		/* Pass UID and use that ID. **/
		uid_ = parent_uid;
		if (op == -1) {
			orig_timer = tdb->timer_;
		}
		if (timer_ == -1) {
			int timer_left;
			int time_increment = 2000; ///< Time Gap every each recheck.
			timer_ = (time_increment*(++tdb->link_fail));
			timer_left = tdb->timer_ - timer_; ///< Timer goes on decreasing by 2,4,6,8 seconds every link fail.

			if (timer_left <= time_increment) {    // Check if time is already < 2 sec, reset link_fail to 0
				tdb->link_fail = 0;
				timer_ = timer_left+time_increment; ///< TimerLeft is 0, so increment it with time_increment.
			}
			delay = true;
			eShowDebug("Timer:%d:%d:%d:%d\n",tdb->link_fail, timer_, timer_left, uid_);
		}
		
		CREATE(etimer_db, struct bg_timerdb_struct, 1);
		/// Copy All Timer Data 
		memcpy(etimer_db, tdb, sizeof(struct bg_timerdb_struct));
		if (tdb->areaheal != NULL) {
			CREATE(etimer_db->areaheal, struct bg_areaheal_struct, 1);
			memcpy(etimer_db->areaheal, tdb->areaheal, sizeof(struct bg_areaheal_struct));
		}
		if (tdb->areawarp != NULL) {
			CREATE(etimer_db->areawarp, struct bg_areawarp_struct, 1);
			memcpy(etimer_db->areawarp, tdb->areawarp, sizeof(struct bg_areawarp_struct));
		}
		if (tdb->timer != NULL) {
			CREATE(etimer_db->timer, struct bg_timer_struct, 1);
			memcpy(etimer_db->timer, tdb->timer, sizeof(struct bg_timer_struct));
		}
		if (tdb->viewpointmap != NULL) {
			CREATE(etimer_db->viewpointmap, struct bg_viewpoint_struct, 1);
			memcpy(etimer_db->viewpointmap, tdb->viewpointmap, sizeof(struct bg_viewpoint_struct));
		}
		if (tdb->mapannounce != NULL) {
			CREATE(etimer_db->mapannounce, struct bg_mapannounce_struct, 1);
			memcpy(etimer_db->mapannounce, tdb->mapannounce, sizeof(struct bg_mapannounce_struct));
		}
		if (tdb->npcs != NULL) {
			CREATE(etimer_db->npcs, struct bg_cond_struct, 1);
			memcpy(etimer_db->npcs, tdb->npcs, sizeof(struct bg_cond_struct));
		}
		if (tdb->arith != NULL) {
			CREATE(etimer_db->arith, struct bg_arith_struct, 1);
			memcpy(etimer_db->arith, tdb->arith, sizeof(struct bg_arith_struct));
		}
		if (tdb->nd != NULL) {
			etimer_db->nd = tdb->nd;
		}

		/* Deep Copy Code Ended. **/
		if (etimer_db->random_timer[0] > 0 && etimer_db->random_timer[1] > 0) {
			random = true;
		}
		
		/// We do not really need to end this, else it might end up waiting or delaying itself, Maybe we do..
		if (tdb->operations & BG_T_O_DELAY) {
			tdb->operations &= ~BG_T_O_DELAY;
		}
		
		if (interval == 0 || delay > 0) {
#ifdef TIMER_LOG
			if (delay > 0) {
				insert_timerlog(parent_uid,"Delay Operation: Free Parent");
			} else {
				insert_timerlog(parent_uid,"create: Free Parent");
			}
#endif
			bg_timer_free(tdb, parent_uid, 0x7); /// Just Remove the Timer
		}
	} else if (tdb == NULL) {
		// Create Child UID
		CREATE(etimer_db, struct bg_timerdb_struct, 1);
		etimer_db->interval = (interval) ? true : false;
		etimer_db->operations = op;
		etimer_db->parent_uid = parent_uid;
		if (rnd_times[0] > 0 && rnd_times[1] > 0) {
			etimer_db->random_timer[0] = rnd_times[0];
			etimer_db->random_timer[1] = rnd_times[1];
			random = true;
		}
		uid_ = bg_get_unique_timer();
		// Save the child data in the parent.
		if (parent_uid > 0 && (tdb = bg_timer_search(parent_uid)) != NULL) {
			int i;
			ARR_FIND(0, MAX_CHILD_TIMER, i, tdb->child_uid[i] ==  -1 || bg_timer_search(tdb->child_uid[i]) == NULL);
			if (i == MAX_CHILD_TIMER) {
				ShowError("bg_timer_create: Parent cannot have more child timer.\n");
				return -1;
			}
			tdb->child_uid[i] = uid_;   // Set Child UID
		}
	} else {
		ShowError("bg_timer_create: tdb Passed with no Parent UID\n");
		return -1;
	}
	
	if (random) {
		int range = etimer_db->random_timer[1] - etimer_db->random_timer[0] + 1;
		orig_timer = timer_ = (rnd()%range + etimer_db->random_timer[0])*1000;
	}
	
	if (interval > 0) {
		etimer_db->timer_id = timer->add_interval(timer->gettick() + timer_, bg_timer_function, oid, (intptr_t)uid_, timer_);
	} else {
		etimer_db->timer_id = timer->add(timer->gettick() + timer_, bg_timer_function, oid, (intptr_t)uid_);
	}
	etimer_db->timer_ = orig_timer;
	eShowDebug("Timer Created with UID(%d)-TID(%d)-Time(%d)-Interval(%d)-Random(%d).\n",uid_,etimer_db->timer_id, timer_, interval, (random==true)?1:0);
#ifdef TIMER_LOG
	if ((parent_uid == uid_ && interval == 0) || delay > 0) {
		if (delay == 0) {
			remove_timerlog(uid_, "Parent--");
		} else {
			remove_timerlog(uid_, "Delay- Parent Created");
		}
	}
#endif
	idb_put(bg_timer_db, uid_, etimer_db);
	
	if (retUID) {
		return uid_;
	}
	
	return etimer_db->timer_id;
}

/**
 * see \@DBMap
 * Runs from db->foreach
 * Clear's all the Timer Data.
 **/
int bg_timer_db_clear(union DBKey key, struct DBData *data, va_list ap)
{
	struct bg_timerdb_struct *tdb = (struct bg_timerdb_struct *)DB->data2ptr(data);
	if (tdb != NULL) {
#ifdef TIMER_LOG
		insert_timerlog(key.i, "ForEach Clear");
#endif
		bg_timer_free(tdb, key.i, 0x5);
	}
	return 0;
}

/**
 * PreHook of npc->reload
 * Clear's all the Timer Data and initializes new DB.
 **/
int npc_reload_pre(void)
{
	/// Destroy and remake the timer 
	bg_timer_db->foreach(bg_timer_db, bg_timer_db_clear);
	db_destroy(bg_timer_db);
	//bg_timer_db->destroy(bg_timer_db, bg_timer_db_clear);
	bg_timer_db = idb_alloc(DB_OPT_BASE);
	return 0;
}

/**
 * areawarp Copy for EoS/DoM, to warp them according to variable.
 **/
int eBG_warp_eos(struct block_list *bl, va_list ap)
{
	int x2, y2, x3, y3;
	bool ebg_custom;
	unsigned int index;
	struct map_session_data *sd = BL_CAST(BL_PC, bl);
	int bg_team = pc->readreg(sd, script->add_str("@BG_Team"));

	index = va_arg(ap, unsigned int);
	x2 = va_arg(ap, int);
	y2 = va_arg(ap, int);
	x3 = va_arg(ap, int);
	y3 = va_arg(ap, int);
	ebg_custom = va_arg(ap, int);
	
	if (ebg_custom == 1) {
		int value = pc->readreg(sd, script->add_str("@eBG_eos_w"));
		eShowDebug("EOS/DOM Warp\n");
		switch(value) {
		case 0:	// No Variable set, warp to boat
			x2 = 353;
			if (bg_team == 1) {
				y2 = 344;
			} else {
				y2 = 52;
			}
			break;
		case 1: // North Base
			x2 = 115;
			y2 = 346;
			break;
		case 2: // South Base
			x2 = 106;
			y2 = 48;
			break;
		case 3: // Domination
			x2 = 260;
			y2 = 183;
			break;
		default:
			return 0;
		}
	}

	if (index == 0) {
		pc->randomwarp(sd,CLR_TELEPORT);
	} else if (x3 > 0 && y3 > 0) {
		int max, tx, ty, j = 0;

		/// choose a suitable max number of attempts
		if ((max = (y3 - y2 + 1) * (x3 - x2 + 1) * 3) > 1000) {
			max = 1000;
		}

		/// find a suitable map cell
		do {
			tx = rnd() % (x3 - x2 + 1) + x2;
			ty = rnd() % (y3 - y2 + 1) + y2;
			j++;
		} while(map->getcell(index,bl,tx,ty,CELL_CHKNOPASS) && j < max);

		pc->setpos(sd, index, tx, ty, CLR_OUTSIGHT);
	} else {
		pc->setpos(sd, index, x2, y2, CLR_OUTSIGHT);
	}

	if (ebg_custom == 1) {
		pc->setreg(sd, script->add_str("@eBG_eos_w"), 0);
	}
	return 1;
}

/**
 * Heals the Player by some percentage
 * @param bl block_list of player to be healed
 * @param ap Contains Hp/Sp Percentage Value
 **/
int areaheal_percent_sub(struct block_list *bl, va_list ap)
{
	pc->percentheal(BL_CAST(BL_PC, bl), va_arg(ap, int), va_arg(ap, int));
	return 1;
}

/**
 * Checks if operation is dependent or independent.
 * If Operation to be performed is dependent, and the timer have
 * return value, then it is executed.
 * If operation is independent, then it is anyhow executed.
 * @param operations Pointer to operation variable
 * @param operation1 Operation to be checked
 * @param tdb TimerDB Structure
 * @return false, if the current operation can be executed, else true.
 **/
bool bg_timer_function_sub(int *operations, enum e_bg_timer operation1, struct bg_timerdb_struct* tdb)
{
	nullpo_ret(tdb);
	nullpo_ret(operations);
	eShowDebug("Dependent: %d, Independent:%d Operation:%u retVal:%d\n",tdb->link.operation_dependent, tdb->link.operation_independent, operation1, tdb->link.retVal);
	if (tdb->link.operation_dependent & operation1) {	/// Dependent on Operation1, but retVal is 0, Set the Delay
		if (tdb->link.retVal == 0) {
			*operations |= BG_T_O_DELAY;
			return true;
		} else {
			return false;
		}
	}
	return false; /// Not dependent
}

/**
 * Called by Timer, its a timer function to heal All PLAYERS in the area
 * Also sets the timer to be saved.
 * @param tid TimerID
 * @param tick Timer
 * @param id ScriptID
 * @param data intptr_t
 **/
int bg_timer_function(int tid, int64 tick, int id, intptr_t data)
{
/**
 * Checks if operation the op is dependent or independent of operation1
 * Block executes if op is independent or op is dependent but have retVal.
 * @param op All operation
 * @param operation1 Operation to be checked
 * @param tdb bg_timerdb_struct
 * @see bg_timerdb_struct
 * @see bg_timer_function_sub
 **/
#define OPERATION_CHECK(op, operation1, tdb)	\
						(op)&(operation1) &&	\
						( tdb->link.operation_independent&(operation1) ||	\
							!bg_timer_function_sub(&(op), (operation1), (tdb)))

/**
 * Checks if operation is dependent or independent.
 * If operation is independent, adds retVal and adds BG_T_O_DELAY,
 * else if operation is dependent, removes BG_T_O_DELAY.
 * @param count All operation
 * @param operation Operation to be checked
 * @param tdb bg_timerdb_struct
 * @see bg_timerdb_struct
 **/
#define OPERATION_SET(count, operation, tdb)	\
						if ((tdb)->link.operation_independent&(operation)) {	\
							(tdb)->link.retVal += (count);	\
							(tdb)->operations |= BG_T_O_DELAY;	\
						}	\
						if ((tdb)->link.operation_dependent&(operation)) {	\
							(tdb)->operations &= ~BG_T_O_DELAY;	\
						}

	int bg_timer_uid = (int)data;
	struct bg_timerdb_struct* tdb = bg_timer_search(bg_timer_uid);
	struct bg_timerdb_struct* tdb_p;
	int operations = 0;
	bool free = false, freed = false;
	struct npc_data *nd = NULL;
	struct npc_extra_data* ned = NULL;
	short success_npc = -1;
	/*
	If need condition_value,
	if (tdb && tdb->npcs && tdb->npcs->npc_or_var) {
		if (ned) {
			cond_value = ned->value[tdb->npcs->checkat[i]];
		}
	} else {
		cond_value = pc->readreg(sd, script->add_str(tdb->npcs->var));
	}
	*/
	
	if (tdb == NULL) {
		ShowError("bg_timer_function: BG Timer Data for UID:%d not found...\n",bg_timer_uid);
		return 0;
	}
	
	eShowDebug("bg_timer_function: Timer with UID(%d)-TID(%d)-Timer(%d) Running\n",bg_timer_uid,tdb->timer_id,tdb->timer_);

	if (tdb->operations&BG_T_O_DELAY) { /// Remove BG_T_O_DELAY if found...
		tdb->operations &= ~BG_T_O_DELAY;
	}

	if (tdb->npcs && tdb->npcs->npc_or_var) {
		nd = npc->name2id(tdb->npcs->name);
		if (nd == NULL) {
			ShowError("bg_timer_function: NPC Data for NPCName(%s) not found.\n",tdb->npcs->name);
			return 0;
		}
		ned = npc_e_search(nd, false);
	}
	
	if (OPERATION_CHECK(tdb->operations, BG_T_O_ARITHMETIC_OP, tdb)) { /// Arithmetic Before Condition Check
		struct npc_extra_data *ned_temp;
		struct npc_data *nd_temp = NULL;
		nd_temp = npc->name2id(tdb->arith->name);
		if (nd_temp == NULL) {
			ShowError("bg_timer_function: NPC Data for NPCName(%s)(BG_T_O_ARITHMETIC_OP) not found.\n",tdb->npcs->name);
			return 0;
		}
		if (tdb->arith == NULL) {
			ShowError("bg_timer_function: Arith Data (BG_T_O_ARITHMETIC_OP) not found.\n");
			return 0;
		}
		ned_temp = npc_e_search(nd_temp, false);
		if (ned_temp == NULL) {
			ShowError("bg_timer_function: NPC Extra Data not found for Arithmetic Operations(BG_T_O_ARITHMETIC_OP), NPC(%s)\n",tdb->npcs->name);
		} else {
			int index_temp = tdb->arith->index_at;
			switch(tdb->arith->operation) {
				case EBG_A_ADD:
					ned_temp->value[index_temp] += tdb->arith->value;
					break;
				case EBG_A_SUBSTRACT:
					ned_temp->value[index_temp] -= tdb->arith->value;
					break;
				case EBG_A_MULTIPLY:
					ned_temp->value[index_temp] *= tdb->arith->value;
					break;
				case EBG_A_DIVIDE:
					ned_temp->value[index_temp] /= tdb->arith->value;
					break;
				case EBG_A_ZEROED:
					ned_temp->value[index_temp] = 0;
					break;
				default:
					ShowError("bg_timer_function: Invalid Operation(%d) for Timer with UID(%d)\n", tdb->arith->operation, bg_timer_uid);
					break;
			}
			OPERATION_SET(ned_temp->value[index_temp], BG_T_O_ARITHMETIC_OP, tdb)
		}
	}
	
	if (OPERATION_CHECK(tdb->operations,BG_T_O_NPC_CONDITION,tdb)) {
		int value, i = 0;
		struct map_session_data *sd = NULL;
		if (!tdb->npcs->npc_or_var) {
			sd = map->nick2sd(tdb->npcs->name);
			if (sd == NULL) {
				ShowError("bg_timer_function: Player(%s) is offline.\n",tdb->npcs->name);
#ifdef TIMER_LOG
				insert_timerlog(bg_timer_uid,"Cond-No Player");
#endif
				bg_timer_free(tdb, bg_timer_uid, 0x7);
				return 0;
			}
		}
		for (i = 0; i < BNPC_MAX_CONDITION; i++) {
			if (tdb->npcs->operations[i]) {
				if (tdb->npcs->npc_or_var) {
					if (ned == NULL) {
						ShowError("bg_timer_function: No NPC Data Found. Condition Checking Terminated for NPC:%s\n",tdb->npcs->name);
						break;
					}
					value = ned->value[tdb->npcs->checkat[i]];
				} else 
					value = pc->readreg(sd, script->add_str(tdb->npcs->var));
				eShowDebug("Value[%d]:%d, Condition: %d, value: %d\n",tdb->npcs->checkat[i], value, tdb->npcs->condition[i], tdb->npcs->value[i]);
				
				if (tdb->npcs->condition[i]&EBG_CONDITION_LESS && value < tdb->npcs->value[i]) {
					success_npc = i;
					break;
				}
				if (tdb->npcs->condition[i]&EBG_CONDITION_EQUAL && value == tdb->npcs->value[i]) {
					success_npc = i;
					break;
				}
				if (tdb->npcs->condition[i]&EBG_CONDITION_GREATER && value > tdb->npcs->value[i]) {
					success_npc = i;
					break;
				}
				if (tdb->npcs->condition[i]&EBG_CONDITION_ELSE) {
					success_npc = i;
					break;
				}
			}
		}
		if (success_npc >= 0) {
			operations = tdb->npcs->operations[success_npc];
		} else {
			operations = tdb->operations;
		}
		OPERATION_SET(10 + success_npc, BG_T_O_NPC_CONDITION, tdb)
	}
	else {
		operations = tdb->operations;
	}
	
	if (tdb->parent_uid > 0) {
		struct bg_timerdb_struct* temp = NULL;
		tdb_p = bg_timer_search(tdb->parent_uid);
		if (tdb_p == NULL) {
			ShowError("bg_timer_function: ParentUID(%d) not found...Deleting this UID.\n",tdb->parent_uid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"Parent Null");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7); /// Free Data if ParentUID not found.
			return 0;
		}
		if (!tdb->interval)
			temp = tdb;
		tdb = tdb_p;
		free = false; /// Don't Free the data if it's run from child.
		eShowDebug("ParentUID is been Free'd");
		if (temp) {
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"Parent-Free");
#endif
			bg_timer_free(temp, bg_timer_uid, 0x7);	/// Free the Old Data...
		}
	}
	
	if (OPERATION_CHECK(operations, BG_T_O_ADDTIMER, tdb)) {	/// Add a New Timer with Self Timer 
		if (tdb->timer == NULL) {
			ShowError("bg_timer_function: Timer with UID(%d) Does not have timer Data(BG_T_O_ADDTIMER). Deleting Timer.\n",bg_timer_uid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"AddTimer");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7);
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_ADDTIMER)\nMore Details: Operations:%d\n",bg_timer_uid,tdb->timer->operations);
		OPERATION_SET(1, BG_T_O_ADDTIMER, tdb) // Do it before bg_timer_create, bg_timer_create can destroy the tdb
		bg_timer_create(tdb->timer->timer_, 0, id, bg_timer_uid, tdb->timer->operations, true, NULL, tdb->random_timer);
		if (OPERATION_CHECK(operations, BG_T_O_ADDTIMERSELF, tdb)) {
			eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_ADDTIMERSELF)\nMore Details: Timer:%d\n",bg_timer_uid,tdb->timer_+tdb->timer->timer_);
			OPERATION_SET(1, BG_T_O_ADDTIMERSELF, tdb) // Do it before bg_timer_create, bg_timer_create destroys the tdb
			bg_timer_create(tdb->timer->timer_ + tdb->timer_, tdb->interval, id, bg_timer_uid, 0, true, tdb, tdb->random_timer);
			freed = true;
		}
	}
	
	/// Basic Operations
	if (!freed && OPERATION_CHECK(operations, BG_T_O_MAPANNOUNCE, tdb)) {
		if (tdb->mapannounce == NULL) {
			ShowError("bg_timer_function: Timer with UID(%d) Does not have timer Data(BG_T_O_MAPANNOUNCE). Deleting Timer.\n",bg_timer_uid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"Mapannounce");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7);
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_MAPANNOUNCE)\nMore Details: m/fontColor/mes:%d/%s/%s\n",bg_timer_uid,(int)tdb->mapannounce->m,tdb->mapannounce->fontColor,tdb->mapannounce->mes);
		map->foreachinmap(script->buildin_announce_sub, tdb->mapannounce->m, BL_PC,
	                  tdb->mapannounce->mes, strlen(tdb->mapannounce->mes)+1, tdb->mapannounce->flag&BC_COLOR_MASK, tdb->mapannounce->fontColor, 0x190, 12, 0, 0);
		OPERATION_SET(1, BG_T_O_MAPANNOUNCE, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_SHOWEFFECT, tdb)) {
		struct block_list *bl = map->id2bl(id);
		int i;
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_SHOWEFFECT)\n",bg_timer_uid);
		
		for (i = 0; i < BMAX_EFFECT; i++) {
			if (tdb->effect[i])
				eShowDebug("More Details: Effect:%d\n",tdb->effect[i]);
		}
		if (bl) {
			for (i = 0; i < BMAX_EFFECT; i++) {
				if (tdb->effect[i])
					clif->specialeffect(bl,tdb->effect[i],AREA);
			}
		}
		OPERATION_SET(1, BG_T_O_SHOWEFFECT, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_AREAHEALER, tdb)) {
		int c = 0;
		if (tdb->areaheal == NULL) {
			ShowError("bg_timer_function: Timer with UID(%d) Does not have timer Data(BG_T_O_AREAHEALER). Deleting Timer.\n",bg_timer_uid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"AreaHealer");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7);
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_AREAHEALER)\nMore Details: m/x1/y1/x2/y2/hp/sp:%d/%d/%d/%d/%d/%d/%d\n",bg_timer_uid,tdb->areaheal->m,tdb->areaheal->x1,tdb->areaheal->y1,tdb->areaheal->x2,tdb->areaheal->y2,tdb->areaheal->hp,tdb->areaheal->sp);
		c = map->foreachinarea(areaheal_percent_sub,tdb->areaheal->m,tdb->areaheal->x1,tdb->areaheal->y1,tdb->areaheal->x2,tdb->areaheal->y2,BL_PC,tdb->areaheal->hp,tdb->areaheal->sp);
		OPERATION_SET(c, BG_T_O_AREAHEALER, tdb)
	}
		
	if (!freed && OPERATION_CHECK(operations, BG_T_O_AREAWARP, tdb)) {
		int c = 0;
		int x2, y2;
		if (tdb->areawarp == NULL) {
			ShowError("bg_timer_function: Timer with UID(%d) Does not have timer Data(BG_T_O_AREAWARP). Deleting Timer.\n",bg_timer_uid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"AreaWarp");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7);
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_AREAWARP)\nMore Details: m/x0/y0/x1/y1/x2/y2:%d/%d/%d/%d/%d/%d/%d\n",bg_timer_uid,tdb->areawarp->m,tdb->areawarp->x0,tdb->areawarp->y0,tdb->areawarp->x1,tdb->areawarp->y1,tdb->areawarp->x2,tdb->areawarp->y2);
		x2 = tdb->areawarp->x2;
		y2 = tdb->areawarp->y2;
		c = map->foreachinarea(eBG_warp_eos, tdb->areawarp->m, tdb->areawarp->x0, tdb->areawarp->y0, tdb->areawarp->x1, tdb->areawarp->y1, BL_PC, tdb->areawarp->index, x2, y2, tdb->areawarp->x3, tdb->areawarp->y3, (x2 == -1) ? 1 : 0);
		OPERATION_SET(c, BG_T_O_AREAWARP, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_VIEWPOINTMAP, tdb)) {
		int16 m = -1, x = -1, y = -1;
		int viewpoint_id = 0, c = 0;
		unsigned int color = 0;
		if (tdb->viewpointmap == NULL) {
			ShowError("bg_timer_function: Timer with UID(%d) Does not have timer Data(BG_T_O_VIEWPOINTMAP). Deleting Timer.\n",bg_timer_uid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"ViewPointMap");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7);
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running (BG_T_O_VIEWPOINTMAP).\n",bg_timer_uid);
		color = tdb->viewpointmap->color;
		if (nd) {
			if (tdb->viewpointmap->m == -1)
				m = nd->bl.m;
			if (tdb->viewpointmap->x == -1)
				x = nd->bl.x;
			if (tdb->viewpointmap->y == -1)
				y = nd->bl.y;
		}
		if (m == -1)
			m = tdb->viewpointmap->m;
		if (x == -1)
			x = tdb->viewpointmap->x;
		if (y == -1)
			y = tdb->viewpointmap->y;
		viewpoint_id = tdb->viewpointmap->id;
		if (viewpoint_id >= 900) {
			struct npc_extra_data *ned_temp = NULL;
			struct npc_data *nd_temp = NULL;
			if (tdb->npcs == NULL) {
				ShowError("bg_timer_function: NPC Data (BG_T_O_VIEWPOINTMAP) not found.\n");
				return 0;
			}
			nd_temp = npc->name2id(tdb->npcs->name);
			if (nd == NULL) {
				ShowError("bg_timer_function: NPC Data for NPCName(%s)(BG_T_O_VIEWPOINTMAP) not found.\n",tdb->npcs->name);
				return 0;
			}
			ned_temp = npc_e_search(nd_temp, false);
			if (ned_temp == NULL)
				ShowError("bg_timer_function: NPC extra Data not found for NPCName(%s)(BG_T_O_VIEWPOINTMAP), Trying to get view ID(%d).\n",tdb->npcs->name,viewpoint_id);
			else
				viewpoint_id = ned_temp->value[(viewpoint_id-900)];
		}
		if (tdb->viewpointmap->color == -2) {	/// Boss NPC 
			success_npc = cap_value(success_npc, 0, 3);
			color = bg_colors[success_npc];
		}
		c = map->foreachinmap(viewpointmap_sub, m, BL_PC, id, tdb->viewpointmap->type, x, y, viewpoint_id, color);
		OPERATION_SET(c, BG_T_O_VIEWPOINTMAP, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_REVEAL_MOB, tdb)) {
		struct block_list *mbl;
		int mid, c = 0;
		if (ned == NULL) {
			ShowError("bg_timer_function: No npc_extra_data found for UID(%d) (BG_T_O_REVEAL_MOB)\n",bg_timer_uid);
			return 0;
		}
		mid = ned->value[0];
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_REVEAL_MOB)\n",bg_timer_uid);
		if (mid == 0 || (mbl = map->id2bl(mid)) == NULL || mbl->type != BL_MOB) {
			ShowError("bg_timer_function: Timer with UID(%d) Does not have Valid ID(%d) (BG_T_O_REVEAL_MOB). Deleting Timer.\n",bg_timer_uid,mid);
#ifdef TIMER_LOG
			insert_timerlog(bg_timer_uid,"RevealMob");
#endif
			bg_timer_free(tdb, bg_timer_uid, 0x7);
			return 0;
		}
		c = map->foreachinmap(viewpointmap_sub, mbl->m, BL_PC, id, 1, mbl->x, mbl->y, mbl->id, ned->value[1]);
		OPERATION_SET(c, BG_T_O_REVEAL_MOB, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_DO_EVENT, tdb)) {
		char output_temp[NAME_LENGTH+100];
		if (tdb->npcs == NULL) {
			ShowError("bg_timer_function: NPC Data (BG_T_O_DO_EVENT) not found.\n");
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_DO_EVENT)\n",bg_timer_uid);
		sprintf(output_temp,"%s::%s",tdb->npcs->name, tdb->npcs->event);
		npc->event_do(output_temp);
		OPERATION_SET(1, BG_T_O_DO_EVENT, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_SCRIPT, tdb)) {
		if (tdb->script == NULL) {
			ShowError("bg_timer_function: Script Data (BG_T_O_SCRIPT) not found.\n");
			return 0;
		}
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_SCRIPT)\n", bg_timer_uid);
		script->run(tdb->script, 0, npc->fake_nd->bl.id, npc->fake_nd->bl.id);
		
		OPERATION_SET(1, BG_T_O_SCRIPT, tdb)
	}
	
	if (!freed && OPERATION_CHECK(operations, BG_T_O_ADDSELF, tdb)) {	/// Add below DoEvent
#ifdef TIMER_LOG
		check_timerlog(bg_timer_uid);
#endif
		eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_ADDSELF)\n",bg_timer_uid);
		OPERATION_SET(1, BG_T_O_ADDSELF, tdb) // Do it before bg_timer_create, bg_timer_create destroys the tdb
		bg_timer_create(tdb->timer_, tdb->interval, id, bg_timer_uid, 0, true, tdb, tdb->random_timer);
		freed = true;
	}
	
	if (!freed && operations&BG_T_O_DELAY) {
		bg_timer_create(-1, tdb->interval, id, bg_timer_uid, -1, true, tdb, tdb->random_timer);
		freed = true;	// It will be removed by bg_timer_create
	}
	else if (!freed && tdb->link.operation_dependent&operations)
		tdb->link.retVal = 0;
	
	if (!freed) {	// Timer isn't free'd yet
		if (tdb->interval == false) { /// No Interval is there, so free it.
			eShowDebug("bg_timer_function: Deleting UID(%d) - No Interval\n",bg_timer_uid);
			free = true;
		}
		
		if (OPERATION_CHECK(operations, BG_T_O_SELFDELETE, tdb)) { // Check if SELFDELETE is there.
			eShowDebug("bg_timer_function: Timer with UID(%d) Running(BG_T_O_SELFDELETE)\n",bg_timer_uid);
			if (tdb) { // Also Check if tdb exists.
#ifdef TIMER_LOG
				insert_timerlog(bg_timer_uid,"SelfDelete");
#endif
				bg_timer_free(tdb, bg_timer_uid, 0x7);
			} else
				eShowDebug("bg_timer_function: No Timer Found.\n");
		} else if (free) {
			eShowDebug("bg_timer_function: Timer with UID(%d) Running(free = 1)\n",bg_timer_uid);
			if (tdb) {
#ifdef TIMER_LOG
				insert_timerlog(bg_timer_uid,"FREE");
#endif
				bg_timer_free(tdb, bg_timer_uid, 0x7);
			} else
				eShowDebug("bg_timer_function: No Timer Found(FREE).\n");
		}
	}
#undef OPERATION_CHECK
#undef OPERATION_END
	return 1;
}


/// BattleConf Function Starts
/**
 * Checks for valid minimum and maximum value of a settings, and return the specific value.
 * @param val Value which is passed from .config
 * @param min Minimum Value
 * @param max Maximum Value
 * @param setting Setting Name to display in error.
 * @return 0 if val is have invalid setting, else val
 **/
int battle_conf_check(const char *val, int min, int max, const char* setting)
{
	int value = atoi(val);
	if (value < min || value > max) {
		ShowError("%s is set to %d,(Min:%d,Max:%d)", setting, value, min, max);
		return 0;
	}
	return value;
}

/**
 * Sets the Value of the battle Configuration
 * @param key Config Name
 * @param val The value captured by emulator
 **/
void ebg_battleconf(const char *key, const char *val)
{
#define BC_CHECK(settingName, settingVariable, minVal, maxVal) \
	if (strcmp(key, settingName) == 0) { \
		settingVariable = battle_conf_check(val, minVal, maxVal, settingName); \
		return; \
	}
	BC_CHECK("battle_configuration/bg_reward_rates", bg_reward_rates, 0, 1000000);     ///< bg_reward_rates - Rates to be given upon reward, Default: 100x
	BC_CHECK("battle_configuration/bg_ranked_mode", bg_ranked_mode, 0, 1);             ///< bg_ranked_mode - Enable Ranked mode? (1=Yes,0=No), Default: 1
	BC_CHECK("battle_configuration/bg_rank_bonus", bg_rank_bonus, 0, 1);               ///< bg_rank_bonus - Enable Ranked Bonus? (1=Yes,0=No), Default: 1
	BC_CHECK("battle_configuration/bg_rank_rates", bg_rank_rates, 0, 10000000);        ///< bg_rank_rates - Rates to be given upon reward(Ranked Mode), Default: 150x
	BC_CHECK("battle_configuration/bg_max_rank_game", bg_max_rank_game, 0, 10000000);  ///< bg_max_rank_game - Maximum Rank Games that a Player can play(in a day), Default: 50
	BC_CHECK("battle_configuration/bg_queue_townonly", bg_queue_townonly, 0, 1);       ///< bg_queue_townonly - Can Only Join from Town? Default: 1
	BC_CHECK("battle_configuration/bg_idle_announce", bg_idle_announce, 0, 86400);     ///< bg_idle_announce - Time after which player is marked as afk. Default: 300
	BC_CHECK("battle_configuration/bg_kick_idle", bg_kick_idle, 0, 1);                 ///< bg_kick_idle - AutoKick Idle Player
	BC_CHECK("battle_configuration/bg_reportafk_leader", bg_reportafk_leader, 0, 1);   ///< bg_reportafk_leader - Only leader can use @reportafk
#ifdef EBG_RANKING
	BC_CHECK("battle_configuration/bg_log_kill", bg_log_kill, 0, 1);                   ///< bg_log_kill - Log Kills
#endif
	BC_CHECK("battle_configuration/bg_reserved_char_id", bg_reserved_char_id, 0, 1);   ///< bg_reserved_char_id - BG CharID for Items
	BC_CHECK("battle_configuration/woe_reserved_char_id", woe_reserved_char_id, 0, 1); ///< woe_reserved_char_id - WoE CharID for Items

#undef BC_CHECK
	ShowWarning("ebg_battleconf: Unknown Config '%s'.\n", key);
	return;
}

/**
 * Returns the Value of the battle Configuration
 * @param key Config Name
 * @return The value of the config
 **/
int ebg_battleconf_return(const char *key)
{
	#define BC_CHECK(settingName, settingVariable) \
	if (strcmp(key, settingName) == 0) { \
		return settingVariable; \
	}
	BC_CHECK("battle_configuration/bg_reward_rates", bg_reward_rates);
	BC_CHECK("battle_configuration/bg_ranked_mode", bg_ranked_mode);
	BC_CHECK("battle_configuration/bg_rank_bonus", bg_rank_bonus);
	BC_CHECK("battle_configuration/bg_rank_rates", bg_rank_rates);
	BC_CHECK("battle_configuration/bg_max_rank_game", bg_max_rank_game);
	BC_CHECK("battle_configuration/bg_queue_townonly", bg_queue_townonly);
	BC_CHECK("battle_configuration/bg_idle_announce", bg_idle_announce);
	BC_CHECK("battle_configuration/bg_kick_idle", bg_kick_idle);
	BC_CHECK("battle_configuration/bg_reportafk_leader", bg_reportafk_leader);
#ifdef EBG_RANKING
	BC_CHECK("battle_configuration/bg_log_kill", bg_log_kill);
#endif
	BC_CHECK("battle_configuration/bg_reserved_char_id", bg_reserved_char_id);
	BC_CHECK("battle_configuration/woe_reserved_char_id", woe_reserved_char_id);
#undef BC_CHECK

	ShowWarning("ebg_battleconf_return: Unknown Config '%s'.\n", key);
	return 0;
}
 
/// BattleConf Functions End 

/**
 * Returns the Value of the variable.
 * @param sd Map_Session_Data
 * @param type Check Player_Data enum
 * @param create Create the memory?
 * @param retVal Returns retVal if memory is not to be created.
 * @return value of the parameter
 **/
int get_variable_(struct map_session_data *sd, int type, bool create, int retVal)
{
	int *val;
	
	if (sd == NULL) {
		ShowError("get_variable_: Player is not attached(Type:%d)\n", type);
		return 0;	/// SD doesn't exists
	}
	
	val = GET_VARIABLE_SIZE(sd, type, create, int);
	
	if (val == NULL) {
		return retVal;
	}

	return *val;
}

/**
 * Returns the Address of the said type
 * @param sd Map_Session_Data
 * @param type Check Player_Data enum
 * @param create Create the memory?
 * @return Address of the type.
 **/
void *get_variable(struct map_session_data *sd, int type, bool create)
{

	struct sd_p_data *data;
	int is_bg = MAP_IS_NONE; ///< Player is on BG/WoE/None?

	if (sd == NULL) {
		ShowError("get_variable: Player is not attached(Type:%d)\n", type);
		return NULL;	/// SD doesn't exists
	}

	data = pdb_search(sd, true);

	CHECK_CREATE(data)
	CHECK_CREATE(data->esd)

	if (data->esd == NULL) {
		CREATE(data->esd, struct ebg_save_data, 1);
	}
	
	switch (type) {
		default: { // Handles Common Data that can be Shared.
			void *retVal;
#ifdef EBG_RANKING
			EBG_OR_WOE(sd, data, is_bg);
			retVal = bg_get_variable_common(data->esd, type, create, &data->save_flag, is_bg);
#else
			retVal = bg_get_variable_common(data->esd, type, create, &data->save_flag);
#endif
			// Error Processing should be done in this function.
			if (retVal == NULL) {
				break;
			}
			return retVal;
		}
		/// HP Loss 
		case BG_HP_VALUE:
		case BG_HP_RATE:
		case BG_HP_TID:
			if (create && data->save.hp_loss == NULL) {
				CREATE(data->save.hp_loss, struct bg_loss, 1);
				data->save.hp_loss->tid = INVALID_TIMER;
				data->save.hp_loss->rate = data->save.hp_loss->value = 0;
			} else if (data->save.hp_loss == NULL) {
				return NULL;
			}
			if (type == BG_HP_VALUE) {
				return (&data->save.hp_loss->value);
			} else if (type == BG_HP_RATE) {
				return (&data->save.hp_loss->rate);
			} else if (type == BG_HP_TID) {
				return (&data->save.hp_loss->tid);
			}
			break;
		/// SP Loss 
		case BG_SP_VALUE:
		case BG_SP_RATE:
		case BG_SP_TID:
			if (create && data->save.sp_loss == NULL) {
				CREATE(data->save.sp_loss, struct bg_loss, 1);
				data->save.sp_loss->tid = INVALID_TIMER;
				data->save.hp_loss->rate = data->save.hp_loss->value = 0;
			} else if (data->save.sp_loss == NULL) {
				return NULL;
			}
			if (type == BG_SP_VALUE) {
				return (&data->save.sp_loss->value);
			} else if (type == BG_SP_RATE) {
				return (&data->save.sp_loss->rate);
			} else if (type == BG_SP_TID) {
				return (&data->save.sp_loss->tid);
			}
			break;
		/// Save 
		case BG_SAVE_TID:
			return (&data->save.tid);
#ifdef EBG_RANKING
		case RANKING_SKILL_USED:
			return (&data->skill_id);
#endif
	}
	
	ShowError("get_variable: NULL RETURNED. %d-%d\n", type, sd->status.char_id);

	return NULL;
}

/**
 * Returns restriction to use item/skill
 * @param sd Map_Session_Data
 * @return Value of the restriction 
 **/
int get_itemskill_restriction(struct map_session_data *sd)
{
	struct sd_p_data *data;

	if (sd == NULL) {
		ShowError("get_itemskill_restriction: SD doesn't exist");
		return 0;
	}
	data = pdb_search(sd, false);
	if (data == NULL)
		return 0;
	eShowDebug("%d - Walk_only",data->flag.only_walk);
	return (data->flag.only_walk);
}

/**
 * pc_useitem PreHooked for itemRestriction (only_walk parameter)
 * @see pc_useitem
 **/
int pc_useitem_pre(struct map_session_data **sd_, int *n_)
{
	int n = *n_;
	int is_bg = MAP_IS_NONE;
	struct sd_p_data *data;
	struct map_session_data *sd = *sd_;

	nullpo_ret(sd);

	if (sd->npc_id || sd->state.workinprogress&1) {
		return 0;
	}

	if (sd->status.inventory[n].nameid <= 0 || sd->status.inventory[n].amount <= 0)
		return 0;

	if (get_itemskill_restriction(sd)) {
		hookStop();
		return 0;
	}

	data = pdb_search(sd, false);
	EBG_OR_WOE(sd, data, is_bg);

	// Check BG Item
	if (sd->status.inventory[n].card[0] == CARD0_CREATE) {
		if (MakeDWord(sd->status.inventory[n].card[2], sd->status.inventory[n].card[3]) == bg_reserved_char_id && is_bg != MAP_IS_BG) {
			hookStop();
			return 0;
		}
		if (MakeDWord(sd->status.inventory[n].card[2], sd->status.inventory[n].card[3]) == woe_reserved_char_id && is_bg != MAP_IS_WOE) {
			hookStop();
			return 0;
		}
	}

	return 1;
}

/**
 * Changes the Leader of Current BG.
 * @param data sd_p_data Structure
 * @param bg_id BG_ID of which leader is to be changed
 **/
void eBG_ChangeLeader(struct sd_p_data *data, int bg_id)
{
	struct bg_extra_info *bg_data_t;
	struct battleground_data *bgd = bg->team_search(bg_id);
	bg_data_t = bg_extra_create(bgd, false);
	if (bg_data_t && bg_data_t->init) {
		// Set Leader to 0
#ifdef LEADER_INT
		bg_data_t->leader = 0;
#else
		if (bg_data_t->leader != NULL)
			bg_data_t->leader->char_id = 0;
#endif
		if (bgd->count > 1) { /// More than 1 Players in BG
			if (data->leader && bg_data_t->leader) { /// Current is the Leader, so Change it.
				int i;

				data->leader = false;
				for (i = 0; i < MAX_BG_MEMBERS; i++) { /// Update other BG members
					struct map_session_data *bg_sd;
					struct sd_p_data *bg_data;
					if ((bg_sd = bgd->members[i].sd) == NULL)
						continue;
					bg_data = pdb_search(bg_sd, false);
					if (bg_data == NULL) {
						ShowWarning("eBG_ChangeLeader: Cannot find Player Data of CharacterID: %d\n",bg_sd->status.char_id);
						return;
					}
					bg_data->leader = true;
#ifdef LEADER_INT
					bg_data_t->leader = bg_sd->status.char_id;
#else
					bg_data_t->leader->char_id = bg_sd->status.char_id;
#endif
					break;
				}
			}
		}
	}
}

void clear_bg_guild_data(struct map_session_data *sd, struct battleground_data *bgd) {
	int i;
	struct guild *g;
	nullpo_retv(sd);
	
	// Remove Guild Buff
	status_change_end(&sd->bl, SC_GUILDAURA, INVALID_TIMER);
	status_change_end(&sd->bl, SC_GDSKILL_BATTLEORDER, INVALID_TIMER);
	status_change_end(&sd->bl, SC_GDSKILL_REGENERATION, INVALID_TIMER);
	
	// Set BgId to 0
	sd->bg_id = 0;
	
	clif->guild_broken(sd, 0);	// Guild Broken Packet, so Client no longer shows cached info
	
	// Update Name and Emblem
	clif->charnameupdate(sd);
	
	// Send Guild Info if player was in guild
	if (sd->status.guild_id && (g = guild->search(sd->status.guild_id)) != NULL) {
		clif->guild_belonginfo(sd, g);
		clif->guild_basicinfo(sd);
		clif->guild_allianceinfo(sd);
		clif->guild_memberlist(sd);
		clif->guild_skillinfo(sd);
		clif->guild_emblem(sd, g);
	}
	
	// Update Emblem
	clif->guild_emblem_area(&sd->bl);
	
	if (bgd == NULL)
		return;
	ARR_FIND(0, MAX_BG_MEMBERS, i, bgd->members[i].sd == sd);
	if (i == MAX_BG_MEMBERS) {	// Not Found
		return;
	}
	if (sd->bg_queue.arena) {
		bg->queue_pc_cleanup(sd);
	}
	memset(&bgd->members[i], 0, sizeof(bgd->members[0]));
}

/**
 * Clears All the variables and settings that are set upon registering to eBG.
 * @param sd Map_Session_Data
 **/
void eBG_turnOff(struct map_session_data *sd)
{
	struct sd_p_data *data;
	struct battleground_data *bgd = bg->team_search(sd->bg_id);
#ifdef VIRT_GUILD
	struct bg_extra_info *bg_data_t;
#endif
	data = pdb_search(sd, false);
	
	if (data == NULL)
		return;
	
	/// Set Ranked and eBG Mode to Off 
	if (data->leader) {
		eBG_ChangeLeader(data, sd->bg_id);
	}
	data->ranked.match = false;
	data->eBG = false;
	data->flag.ebg_afk = 0;	// Set AFK Timer to 0
	eShowDebug("Checking data->g bg_end...");
	/// Remove Guild Data if found 
	if (
#ifndef VIRT_GUILD
	data->g
#else
	bgd != NULL &&
	((bg_data_t = bg_extra_create(bgd, false)) != NULL) &&
	bg_data_t->g
#endif
	) {
		eShowDebug("BG Leave: Guild Found\n");
#ifdef VIRT_GUILD
		bg_data_t->g->connect_member--;
#else
		if (sd->guild) {
			sd->guild->connect_member--;
			eBG_Guildremove(sd, sd->guild);
		}
#endif
		eShowDebug("BG Leave: Guild Removed\n");
#ifndef VIRT_GUILD
		if (data->g) {
			sd->status.guild_id = data->g->guild_id;
			sd->guild = data->g;
			eShowDebug("BG Leave: sd->g = data->g\n");
		} else {
			sd->status.guild_id = 0;
			sd->guild = NULL;
			eShowDebug("BG Leave: sd->g = 0\n");
		}
		data->g = NULL;
#endif
	}
	clear_bg_guild_data(sd, bgd);

	ebg_clear_hpsp_timer(sd, EBG_HP_TIME | EBG_SP_TIME | EBG_EXTRA_TIME); // Clear HpSp Timer

	/* Save the Data and Delete the Timer. **/
	bg_save_data(sd);
	
	//bg_clear_char_data(sd);	/* Don't Clear, we would not load data from SQL everytime. **/
	
	return;
}

/**
 * To be Executed upon Player Logout, Clears the timer and memory allocated.
 * @param sd Map_Session_Data
 **/
void pc_logout_action(struct map_session_data *sd)
{
	struct sd_p_data *data;
	if (sd != NULL && (data = pdb_search(sd, false)) != NULL) {
		// Clear the Timer
		bg_save_data(sd);
		bg_clear_char_data(data->esd, data);
		eBG_turnOff(sd);	//Turn off the BG , if its not done so far. @todo Test this
	}
}


/**
 * pc_setpos PreHook.
 * Does the Necessary Restriction for Item/Skill, calling logout actions and stopping random warps.
 * @see pc_setpos
 **/
int pc_setpos_pre(struct map_session_data **sd, unsigned short *map_index_, int *x_, int *y_, clr_type *clrtype_)
{
	int16 m;
	int changemap;
	unsigned short map_index = *map_index_;
	int x = *x_, y = *y_;
	struct sd_p_data *data;
	bool stopHook = false;

	nullpo_ret(*sd);
	eShowDebug("%d(%d) x:%d y:%d\n",map_index, (*sd)->mapindex, x, y);
	data = pdb_search(*sd, false);
	if (map_index <= 0 || !mapindex_id2name(map_index) || (m = map->mapindex2mapid(map_index)) == -1) {
		eShowDebug("pc_setpos_pre: Passed mapindex(%d) is invalid!\n", map_index);
		return 1;
	}
	
	if (data == NULL)
		return 1;

	changemap = ((*sd)->mapindex != map_index);
	if (changemap) { // Misc map-changing settings		
		data->flag.only_walk = 0;
		if (!map->list[m].flag.battleground && data->eBG) {
			data->eBG = false;
			eBG_turnOff(*sd);
			eShowDebug("pc_setpos_pre: eBG Set to Off for Character %d\n", (*sd)->status.char_id);
		}
	}
	if (data->eBG) {
		if (x == EBG_WARP && y == EBG_WARP && map->list[m].flag.battleground) {
			/**
			 * Stops the hook to avoid been warped.
			 * Needed to know if person is on eBG..
			 **/
			stopHook = true;
		}
	}
	if (m < 0)
		pc_logout_action(*sd);
	if (stopHook)
		hookStop();
	return 0;
}

/**
 * skill_notok PreHook. Allows the Usage of Guild Skill for leaders, Restriction of Item/Skills.
 * @see skill_notok
 **/
int skill_notok_pre(uint16 *skill_id, struct map_session_data **sd)
{
	if (get_itemskill_restriction(*sd)) {
		hookStop();
		return 1;
	}
	eShowDebug("NotOk: Emergency Skill1 - 1\n");
	if (*skill_id == GD_EMERGENCYCALL) {
		struct sd_p_data *data = pdb_search(*sd, false);
		struct ebg_mapflags *mf_data = getFromMAPD(&map->list[(*sd)->bl.m], 0);

		if (mf_data && mf_data->no_ec) {
			clif->message((*sd)->fd, "Emergency Call cannot be cast here.");
			hookStop();
			return 1;
		}

		eShowDebug("NotOk: Emergency Skill1 - 2: %d\n", (data && data->leader)? 1: 0);
		if (data && map->list[(*sd)->bl.m].flag.battleground && data->eBG == true && data->leader) {
			eShowDebug("NotOk: Emergency Skill1 - 3\n");
			hookStop();
			return 0;
		}
	}
	return 1;
}

/**
 * Increases Player's Fame Points and Displays.
 * @param sd Map_Session_Data
 * @param count Number of points to be increased
 * @param type Which Points to be increased?(0=Normal,other=Ranked)
 * @param leader To Know if the player is leader or not.
 * @param extra_count Extra Points to be given if any..(Only Applicable for Leaders)
 **/
void pc_addpoint_bg(struct map_session_data *sd, int count, int type, bool leader, int extra_count)
{
	char message[100];
	int *points;
	
	nullpo_retv(sd);
	switch (type) {
		case 0: /// BG Normal Matches
			SET_VARIABLE_ADD(sd, points, BG_POINTS, count, int);
			sprintf(message, "[Your Battleground Rank +%d = %d points]", count, *points);
			if (leader && extra_count != 0) {
				clif->message(sd->fd, message);
				SET_VARIABLE_ADD(sd, points, BG_POINTS, extra_count, int);
				sprintf(message, "[Your Battleground Rank(Leader Bonus) +%d = %d points]", extra_count, *points);
			}
			break;
		default: /// Bg Ranked Matches
			SET_VARIABLE_ADD(sd, points, BG_RANKED_POINTS, count, int);
			sprintf(message, "[Your Battleground Rank +%d = %d points]", count, *points);
			if (leader && extra_count != 0) {
				clif->message(sd->fd, message);
				SET_VARIABLE_ADD(sd, points, BG_RANKED_POINTS, extra_count, int);
				sprintf(message, "[Your Battleground Rank(Leader Bonus) +%d = %d points]", extra_count, *points);
			}
			break;
	}
	
	update_bg_ranking(sd, (type+1)); /// Update Rank(map_side)
	clif->message(sd->fd, message);
	return;
}

/**
 * Returns the PlayerName after Querying from SQL.
 * @param char_id CharacterID
 * @param name Pointer to name variable.
 * @return 1 if name is fetched.
 **/
int load_playername(int char_id, char* name)
{
	char* data;
	size_t len;

	if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `name` FROM `char` WHERE `char_id`='%d'", char_id))
		Sql_ShowDebug(map->mysql_handle);
	else if (SQL_SUCCESS == SQL->NextRow(map->mysql_handle)) {
		SQL->GetData(map->mysql_handle, 0, &data, &len);
		safestrncpy(name, data, NAME_LENGTH);
		return 1;
	} else {
		sprintf(name, "Unknown");
	}
	return 0;
}

/**
 * Updates the Fame list of BG(Normal/Ranked)
 * @param sd Map_Session_Data
 * @param type2 Type of BG(1=Normal,Else=Ranked)
 * @return 0
 **/
int update_bg_ranking(struct map_session_data *sd, int type2)
{
	// Todo: Move to char server
	int cid = sd->status.char_id, points, size, player_pos, fame_pos;
	struct fame_list *list;
	//int num;
	
	switch (type2) {
		case 1:
			points = get_variable_(sd, BG_POINTS, false, 0);
			size = fame_list_size_bg;
			list = bg_fame_list;
			break;
		default:
			points = get_variable_(sd, BG_RANKED_POINTS, false, 0);
			size = fame_list_size_bgrank;
			list = bgrank_fame_list;
			break;
	}

	ARR_FIND(0, size, player_pos, list[player_pos].id == cid);// position of the player
	ARR_FIND(0, size, fame_pos, list[fame_pos].fame <= points);
	
	if (player_pos == size && fame_pos == size) {
		// Not On List
	} else if (fame_pos == player_pos) {
		// same position
		list[player_pos].fame = points;
	} else {
		// move in the list
		if (player_pos == size) {
			// new ranker - not in the list
			ARR_MOVE(size - 1, fame_pos, list, struct fame_list);
			list[fame_pos].id = cid;
			list[fame_pos].fame = points;
			load_playername(cid, list[fame_pos].name);
		} else {
			// already in the list
			if (fame_pos == size)
				--fame_pos;// move to the end of the list
			ARR_MOVE(player_pos, fame_pos, list, struct fame_list);
			list[fame_pos].fame = points;
		}
	}

	return 0;
}

/**
 * Gives the Reward to Respected BG
 * @param bg_id BG_ID to whom reward has to be given
 * @param nameid ItemID to be given
 * @param amount Amount to be given
 * @param kafrapoints KafraPoints to be added
 * @param quest_id QuestID that will be added
 * @param var Variables that should be increased
 * @param add_value How much variable should be increased
 * @param bg_a_type Type of Match (struct Battleground_Types)
 * @param bg_win_loss Won/Loss or Tie?(0=Won/1=Tie/2=Loss)
 **/
void bg_team_rewards(int bg_id, int nameid, int amount, int kafrapoints, int quest_id, const char *var, int add_value, int bg_a_type, int bg_win_loss)
{
	struct battleground_data *bgd;
	struct item it;
	int j, flag, get_amount, type;
	bool rank = false;

	if ((bgd = bg->team_search(bg_id)) == NULL || itemdb->exists(nameid) == NULL) {
		ShowDebug("bg_team_rewards: Cannot give x%d %d to BG_ID: %d\n", amount, nameid, bg_id);
		return;
	}

	if (bg_reward_rates != 100) { // BG Reward Rates
		amount = amount * bg_reward_rates / 100;
		kafrapoints = kafrapoints * bg_reward_rates / 100;
	}

	bg_win_loss = cap_value(bg_win_loss, 0, 2);
	memset(&it, 0, sizeof(it));
	if (nameid == 7828 || nameid == 7829 || nameid == 7773) {
		it.nameid = nameid;
		it.identify = 1;
	}
	else
		nameid = 0;

	for (j = 0; j < MAX_BG_MEMBERS; j++) {
		struct map_session_data *sd;
		struct sd_p_data *data;
		int *point,*point_,*leader_stat = NULL;
		bool is_leader = false;
		if ((sd = bgd->members[j].sd) == NULL)
			continue;
		
		data = pdb_search(sd, true);
		
		if (bg_rank_bonus && data && data->ranked.match) { // Rank bonus enabled and was match ranked?
			int i;
			ARR_FIND(0, fame_list_size_bgrank, i, bgrank_fame_list[i].id == sd->status.char_id);
			if (i < fame_list_size_bgrank)
				rank = true;
			else {
				ARR_FIND(0, fame_list_size_bg, i, bg_fame_list[i].id == sd->status.char_id);
				if (i < fame_list_size_bg)
					rank = true;
			}
		}
		if (data != NULL && data->leader) {
			is_leader = data->leader;
		}

		if (quest_id)
			quest->add(sd, quest_id, 0);
		pc_setglobalreg(sd, script->add_str(var), pc_readglobalreg(sd, script->add_str(var)) + add_value);

		if (kafrapoints > 0) {
			get_amount = kafrapoints;
			if (rank)
				get_amount += bg_rank_rates * get_amount / 100;
			pc->getcash(sd, 0, get_amount);
		}

		if (nameid && amount > 0) {
			get_amount = amount;
			if (rank)
				get_amount += bg_rank_rates * get_amount / 100;

			if ((flag = pc->additem(sd, &it, get_amount, LOG_TYPE_SCRIPT)))
				clif->additem(sd, 0, 0, flag);
		}

		type = mapreg->readreg(script->add_str("$BGRanked"));
		
		switch (bg_win_loss) {
		case BGC_WON: // Won
			SET_VARIABLE_ADD(sd, point_, BG_WON, 1, int);
			if (is_leader) {
				SET_VARIABLE_ADD(sd, leader_stat, BG_LEADER_WON, 1, int);
			}
			pc_addpoint_bg(sd, 100, type, is_leader, 25);
			switch (bg_a_type) {
				case BGT_EOS:
					SET_VARIABLE_ADD(sd, point, BG_EOS_WON, 1, int);
					break;
				case BGT_BOSS:
					SET_VARIABLE_ADD(sd, point, BG_BOSS_WON, 1, int);
					break;
				case BGT_TI:
					SET_VARIABLE_ADD(sd, point, BG_TI_WON, 1, int);
					break;
				case BGT_TD:
					SET_VARIABLE_ADD(sd, point, BG_TD_WON, 1, int);
					break;
				case BGT_SC:
					SET_VARIABLE_ADD(sd, point, BG_SC_WON, 1, int);
					break;
				case BGT_CONQ:
					SET_VARIABLE_ADD(sd, point, BG_CONQ_WON, 1, int);
					break;
				case BGT_RUSH:
					SET_VARIABLE_ADD(sd, point, BG_RUSH_WON, 1, int);
					break;
				case BGT_DOM:
					SET_VARIABLE_ADD(sd, point, BG_DOM_WON, 1, int);
					break;
				case BGT_CTF:
					SET_VARIABLE_ADD(sd, point, BG_CTF_WON, 1, int);
					break;
			}
			break;
		case BGC_TIE: // Tie
			SET_VARIABLE_ADD(sd, point, BG_TIE, 1, int);
			if (is_leader) {
				SET_VARIABLE_ADD(sd, leader_stat, BG_LEADER_TIE, 1, int);
			}
			pc_addpoint_bg(sd, 75, type, is_leader, 10);
			switch (bg_a_type) {
				case BGT_EOS:
					SET_VARIABLE_ADD(sd, point, BG_EOS_TIE, 1, int);
					break;
				case BGT_BOSS:
					SET_VARIABLE_ADD(sd, point, BG_BOSS_TIE, 1, int);
					break;
				case BGT_TI:
					SET_VARIABLE_ADD(sd, point, BG_TI_TIE, 1, int);
					break;
				case BGT_TD:
					SET_VARIABLE_ADD(sd, point, BG_TD_TIE, 1, int);
					break;
				case BGT_SC:
					SET_VARIABLE_ADD(sd, point, BG_SC_TIE, 1, int);
					break;
				case BGT_DOM:
					SET_VARIABLE_ADD(sd, point, BG_DOM_TIE, 1, int);
					break;
				///Conquest and Rush are No Tie 
				case BGT_CTF:
					SET_VARIABLE_ADD(sd, point, BG_CTF_TIE, 1, int);
					break;
			}
			break;
		case BGC_LOSS: // Lost
			SET_VARIABLE_ADD(sd, point_, BG_LOSS, 1, int);
			if (is_leader) {
				SET_VARIABLE_ADD(sd, leader_stat, BG_LEADER_LOSS, 1, int);
			}
			pc_addpoint_bg(sd, 50, type, is_leader, 0);
			switch (bg_a_type) {
				case BGT_EOS:
					SET_VARIABLE_ADD(sd, point, BG_EOS_LOSS, 1, int);
					break;
				case BGT_BOSS:
					SET_VARIABLE_ADD(sd, point, BG_BOSS_LOSS, 1, int);
					break;
				case BGT_TI:
					SET_VARIABLE_ADD(sd, point, BG_TI_LOSS, 1, int);
					break;
				case BGT_TD:
					SET_VARIABLE_ADD(sd, point, BG_TD_LOSS, 1, int);
					break;
				case BGT_SC:
					SET_VARIABLE_ADD(sd, point, BG_SC_LOSS, 1, int);
					break;
				case BGT_CONQ:
					SET_VARIABLE_ADD(sd, point, BG_CONQ_LOSS, 1, int);
					break;
				case BGT_RUSH:
					SET_VARIABLE_ADD(sd, point, BG_RUSH_LOSS, 1, int);
					break;
				case BGT_DOM:
					SET_VARIABLE_ADD(sd, point, BG_DOM_LOSS, 1, int);
					break;
				case BGT_CTF:
					SET_VARIABLE_ADD(sd, point, BG_CTF_LOSS, 1, int);
					break;
			}
			break;
		}
	}
}

#ifdef EBG_RANKING
// Pow Alternative
int ipow(int base, int exp)
{
    int result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

/**
 * Stores Kill Log and records skills too
 * @param killed Data of Player who died
 * @param killer Data of Player who killed other player.
 **/
void ebg_kill(struct map_session_data *killed, struct map_session_data *killer)
{
	int is_bg;
	struct sd_p_data* killer_data = pdb_search(killer, false);
	if (killed == NULL || killer == NULL || killer == killed)
		return;
	if (bg_log_kill <= 0 || bg_log_kill > 7)
		return;
	EBG_OR_WOE(killer, killer_data, is_bg);
	if ((is_bg == MAP_IS_NONE && (bg_log_kill&3)) ||
		(is_bg == MAP_IS_BG && (bg_log_kill&1)) ||
		(is_bg == MAP_IS_WOE && (bg_log_kill&2)) ) {
		char killer_name[NAME_LENGTH*2+1];
		char killed_name[NAME_LENGTH*2+1];
		uint16 skill_id = 0;
		
		if (killer_data) {
			skill_id = killer_data->skill_id;
		}

		SQL->EscapeStringLen(map->mysql_handle, killer_name, killer->status.name, strnlen(killer->status.name, NAME_LENGTH));
		SQL->EscapeStringLen(map->mysql_handle, killed_name, killed->status.name, strnlen(killed->status.name, NAME_LENGTH));
		
		ShowDebug("%s Killed %s by %d", killer_name, killed_name, (int)skill_id);

		if (SQL_ERROR == SQL->Query(map->mysql_handle,
									"INSERT INTO `char_kill_log` (`time`,`killer_name`,`killer_id`,`killed_name`,`killed_id`,`map`,`skill`,`map_type`) VALUES (NOW(), '%s', '%d', '%s', '%d', '%s', '%d', '%d')",
									killer_name, killer->status.char_id,
									killed_name, killed->status.char_id,
									map->list[killed->bl.m].name, skill_id, is_bg))
			Sql_ShowDebug(map->mysql_handle);

	}
	if (is_bg == MAP_IS_WOE) {
		// Guild Ranking - War of Emperium
		// Own Guild Ranking
		// tsd = Killed
		// ssd = Killer
		int32 elo;
		unsigned int *killed_score, *killer_score;
		killed_score = GET_VARIABLE_SIZE(killed, MakeDWord(RANKING_SCORE, RANKING_WOE_FORCE), true, unsigned int);
		killer_score = GET_VARIABLE_SIZE(killer, MakeDWord(RANKING_SCORE, RANKING_WOE_FORCE), true, unsigned int);
		elo = cap_value(50 / (1 + ipow(10, (*killer_score - *killed_score) / 2000)), INT32_MIN, INT32_MAX);
		*killer_score = cap_value(*killer_score+elo, 0, 4000);
		*killed_score = cap_value(*killed_score-elo, 0, 4000);
	} else if (is_bg == MAP_IS_BG) {
		int elo;
		struct battleground_data *killer_bgd, *killed_bgd;
		struct map_session_data *killer_pl[MAX_BG_MEMBERS], *killed_pl[MAX_BG_MEMBERS];
		unsigned int killer_rate = 0, killed_rate = 0;
		int killer_count, killed_count, s_Elo, t_Elo;
		unsigned int *p_score;
		int i;

		if ((killer_bgd = bg->team_search(killer->bg_id)) == NULL || (killed_bgd = bg->team_search(killed->bg_id)) == NULL)
			return;

		// Source(Killer)
		for (i = killer_count = 0; i < MAX_BG_MEMBERS; i++) {
			if( (killer_pl[killer_count] = killer_bgd->members[i].sd) == NULL || killer_pl[killer_count]->bl.m != killer->bl.m )
				continue;
			p_score = GET_VARIABLE_SIZE(killer_pl[killer_count], MakeDWord(RANKING_SCORE, RANKING_BG_FORCE), true, unsigned int);
			killer_rate += *p_score;
			killer_count++;
		}
		if (killer_count < 1)
			return;
		else
			killer_rate /= killer_count; // Average Score

		// Target(Killed)
		for (i = killed_count = 0; i < MAX_BG_MEMBERS; i++) {
			if( (killed_pl[killed_count] = killed_bgd->members[i].sd) == NULL || killed_pl[killed_count]->bl.m != killed->bl.m )
				continue;
			p_score = GET_VARIABLE_SIZE(killed_pl[killed_count], MakeDWord(RANKING_SCORE, RANKING_BG_FORCE), true, unsigned int);
			killed_rate += *p_score;
			killed_count++;
		}
		if (killed_count < 1)
			return;
		else
			killed_rate /= killed_count; // Average Target Rate

		elo = cap_value(50 / (1 + ipow(10, (killer_rate - killed_rate) / 2000)), INT32_MIN, INT32_MAX);
		s_Elo = elo / killer_count;
		for( i = 0; i < killer_count; i++ ) {
			p_score = GET_VARIABLE_SIZE(killer_pl[i], MakeDWord(RANKING_SCORE, RANKING_BG_FORCE), true, unsigned int);
			*p_score = cap_value(*p_score+s_Elo, 0, 4000);
		}

		t_Elo = elo / killed_count;
		for( i = 0; i < killed_count; i++ ) {
			p_score = GET_VARIABLE_SIZE(killed_pl[i], MakeDWord(RANKING_SCORE, RANKING_BG_FORCE), true, unsigned int);
			*p_score = cap_value(*p_score-t_Elo, 0, 4000);
		}
	}
}
#endif

/**
 * pc_dead PreHooked.
 * It sets the killer_bg_id(Temporary player variable) and killer_bg_src(Temporary player variable) and Executes DieEvent for the killer.
 * sd = Killed
 * src = Killer
 * @see pc_dead
 **/
int pc_dead_pre(struct map_session_data **sd_, struct block_list **src)
{
	struct map_session_data *ssd;
	struct map_session_data *sd = *sd_;
	struct block_list *master = NULL;    ///< Killer
	
	nullpo_retr(0, sd);
	
	if (*src != NULL)
		master = battle->get_master(*src);

	ssd = BL_CAST(BL_PC, master);
	if (sd != NULL && ssd != NULL) {
		struct sd_p_data* data = pdb_search(ssd, false);
#ifdef EBG_RANKING
		int is_bg = MAP_IS_NONE;
#endif
		if (sd->bg_id) {
			if (bg->team_search(sd->bg_id) != NULL) {
				pc->setreg(sd, script->add_str("@killer_bg_id"), bg->team_get_id(*src));                                 // Killer Battleground ID
				pc->setreg(sd, script->add_str("@killer_bg_src"), ssd && ssd->bg_id ? ssd->bl.id : 0);                  // Killer BL ID
				pc->setreg(sd, script->add_str("@killer_bg_acc_id"), ssd && ssd->bg_id ? ssd->status.account_id : 0);   // Killer AccountID
				if (data && data->eBG) {
					data->kills = data->kills+1;
				}
			}
		}
#ifdef EBG_RANKING
		EBG_OR_WOE(sd, data, is_bg);
		if (is_bg != MAP_IS_NONE) {
			unsigned int *kill_death;
			SET_VARIABLE_ADD(ssd, kill_death, RANKING_KILLS, 1, unsigned int);
			SET_VARIABLE_ADD(sd, kill_death, RANKING_DEATHS, 1, unsigned int);
		}
		ebg_kill(sd, ssd);
#endif
	}
	return 1;
}

/**
 * Updates the viewpoint(Basically for showing flags now)
 **/
int viewpointmap_sub(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	int npc_id, type, x, y, id, color;
	npc_id = va_arg(ap, int);
	type = va_arg(ap, int);
	x = va_arg(ap, int);
	y = va_arg(ap, int);
	id = va_arg(ap, int);
	color = va_arg(ap, int);
	sd = (struct map_session_data *)bl;
	clif->viewpoint(sd, npc_id, type, x, y, id, color);
	return 1;
}

/**
 * HpSp Common Timer Function.
 * Shows Color Update in MiniMap and shows effect and Help Emotion.
 * @param sd Map_Session_Data
 **/
int bg_hpsp_common_function(struct map_session_data *sd)
{
	struct sd_p_data *data;
	
	data = pdb_search(sd, false);
	if (data == NULL) {
		ShowError("bg_hpsp_common_function: Player with null sd_p_data(%d:%s)...\n",sd->status.char_id,sd->status.name);
		return 0;
	}
	if (data->save.extra == NULL) {
		ShowError("bg_hpsp_common_function: Player with No ExtraData(%d:%s)...\n",sd->status.char_id,sd->status.name);
		return 0;
	}
	if (data->save.extra->npcs && data->save.extra->npcs->npc_or_var) {
		struct bg_cond_struct *npcd = data->save.extra->npcs;
		struct npc_data *nd = npc->name2id(npcd->name);
		struct npc_extra_data *ned;
		int index = -1;;
		if (nd == NULL) {
			ShowError("bg_hpsp_common_function: NPC Data for NPCName(%s) not found.\n",npcd->name);
			return 0;
		}
		ned = npc_e_search(nd, false);
		
		if (ned) {
			int i;
			// Check Conditions
			for (i = 0; i < BNPC_MAX_CONDITION; i++) {
				int value;
				eShowDebug("index: %d, i:%d, Condition: %d\n", index, i, npcd->condition[i]);
				if (index != -1)
					break;
				index = i;
				value = ned->value[npcd->checkat[i]];

				if (npcd->condition[i]&EBG_CONDITION_LESS)
					if (value > npcd->value[i])
						index = -1;
				if (npcd->condition[i]&EBG_CONDITION_EQUAL)
					if (value != npcd->value[i])
						index = -1;
				if (npcd->condition[i]&EBG_CONDITION_GREATER)
					if (value < npcd->value[i])
						index = -1;
				if (npcd->condition[i]&EBG_CONDITION_ELSE)
					index = i;
				
			}
		}
		if (index == -1) {  // Condition Failed. Remove Save Data and clear it.
			ebg_clear_hpsp_timer(sd, EBG_HP_TIME | EBG_SP_TIME | EBG_EXTRA_TIME);
			return 0;
		}
	}
	
	map->foreachinmap(viewpointmap_sub, sd->bl.m, BL_PC, npc->fake_nd->bl.id, 1, sd->bl.x, sd->bl.y, data->save.extra->viewpoint.id, data->save.extra->viewpoint.color);
	clif->specialeffect(&sd->bl, data->save.extra->effect.id, AREA);
	clif->emotion(&sd->bl, data->save.extra->emotion.id);

	if (pc_isdead(sd))
		return 0;
	return 1;
}

/**
 * HP Loss Timer Function
 * Reduces HP. Cannot Kill you
 **/
int bg_hp_loss_function(int tid, int64 tick, int id, intptr_t data)
{
	struct map_session_data *sd;
	int value, *hpsp_tid;
	unsigned int hp = 0;
	/**/
	sd = map->id2sd(id);
	if (sd == NULL) {
		ShowError("bg_hp_loss_function: Deleting Timer %d, Player(%d) offline.\n", tid, id);
		timer->delete(tid, bg_hp_loss_function);
		return 0;
	}
	if (!bg_hpsp_common_function(sd))
		return 0;

	hpsp_tid = GET_VARIABLE_SIZE(sd, BG_HP_TID, false, int);
	if (hpsp_tid == NULL) {
		ShowError("BG_HP_TID: NULL\n");
		return 0;
	}
	if (*hpsp_tid != tid) {
		ShowError("BG_HP_TID: %d != %d\n",*hpsp_tid,tid);
		return 0;
	}
	value = get_variable_(sd, BG_HP_VALUE, false, 1);
	hp = sd->battle_status.max_hp * value / 100;
	// Don't kill
	if (hp >= sd->battle_status.hp)
		hp = sd->battle_status.hp - 1;

	if (hp > 0)
		status_zap(&sd->bl, hp, 0);
	
	return 1;
}

/**
 * SP Loss Timer Function
 * Reduces SP.
 **/
int bg_sp_loss_function(int tid, int64 tick, int id, intptr_t data)
{
	struct map_session_data *sd;
	int value, *hpsp_tid;
	unsigned int sp = 0;
	/**/
	sd = map->id2sd(id);
	if (sd == NULL) {
		ShowError("bg_sp_loss_function: Deleting Timer %d, Player(%d) offline.\n", tid, id);
		timer->delete(tid, bg_sp_loss_function);
		return 0;
	}
		
	if (!bg_hpsp_common_function(sd))
		return 0;

	hpsp_tid = GET_VARIABLE_SIZE(sd, BG_SP_TID, false, int);
	if (hpsp_tid == NULL) {
		ShowError("BG_SP_TID: Null\n");
		return 0;
	}

	if (*hpsp_tid != tid) {
		ShowError("BG_SP_TID: %d != %d\n",*hpsp_tid,tid);
		return 0;
	}

	value = get_variable_(sd, BG_SP_VALUE, false, 1);
	sp = sd->battle_status.max_sp * value / 100;

	if (sp > sd->battle_status.sp)
		sp = sd->battle_status.sp;

	if (sp > 0)
		status_zap(&sd->bl, 0, sp);
	
	return 1;
}

/**
 * Drops item in the given area.
 * @param m MapID
 * @param x x CoOrdinate
 * @param y y CoOrdinate
 * @param itemid ItemID to Drop
 * @param amount amount of ItemID to drop
 **/
void area_flooritem(int m, short x, short y, int itemid, int amount)
{
	struct item item_tmp;
	int range, i;

	memset(&item_tmp, 0, sizeof(item_tmp));
	item_tmp.nameid = itemid;
	item_tmp.identify = 1;

	range = (int)sqrt(amount) + rnd()%2;
	for (i = 0; i < amount; i++) {
		map->search_freecell(NULL, m, &x, &y, range, range, 1);
		map->addflooritem(NULL, &item_tmp, 1, m, x, y, 0, 0, 0, 0, false); //ToDo: Greed Check
	}
}

/**
 * pc_dead PostHook function.
 * Removes HP/SP Timer and sets \@bg_killer_id.
 * @see pc_dead
 **/
int pc_dead_post(int retVal, struct map_session_data *sd, struct block_list *src)
{
	ebg_clear_hpsp_timer(sd, EBG_HP_TIME | EBG_SP_TIME | EBG_EXTRA_TIME);
	
	if (retVal != 0) {
		pc->setreg(sd,script->add_str("@bg_killer_id"), src ? src->id : 0);
	}
	return retVal;
}


/**
 * clif_parse_LoadEndAck PreHooked.
 * If New Player Connects, it Sets Save Timer, and loads all char_data.
 * @see clif_parse_LoadEndAck
 **/
void clif_parse_LoadEndAck_pre(int *fd, struct map_session_data **sd_)
{
	struct map_session_data *sd = *sd_;
	if (sd->state.connect_new) {
		int *hp_tid, *sp_tid, *bg_sv_tid;
		hp_tid = GET_VARIABLE_SIZE(sd, BG_HP_TID, true, int);
		sp_tid = GET_VARIABLE_SIZE(sd, BG_SP_TID, true, int);
		bg_sv_tid = GET_VARIABLE_SIZE(sd, BG_SAVE_TID, true, int);
		*hp_tid = INVALID_TIMER;
		*sp_tid = INVALID_TIMER;
		*bg_sv_tid = INVALID_TIMER;
		bg_load_char_data(sd);
	}
	return;
}

/**
 * Checks PlayerIP based on same map
 * @param bl Block_List
 * @param ap va_list
 * @return 1 if found, else 0.
 **/
int bg_ipcheck_sub(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd,*pl_sd;
	pl_sd = (struct map_session_data *)bl;
	sd = va_arg(ap,struct map_session_data *);
	if (sockt->session[sd->fd]->client_addr == sockt->session[pl_sd->fd]->client_addr)
		return 1;
	return 0;
}

/**
 * Checks PlayerIP based on map(or not)
 * @param sd Map_Session_Data
 * @param same_map Check if player is on same map as Map_Name?
 * @param map_name MapName to check.
 * @return Total Count of Users on that map.
 **/
int bg_ipcheck(struct map_session_data *sd, bool same_map, const char* map_name)
{
	int c = 0;
	int16 m = map->mapname2mapid(map_name);
	nullpo_ret(sd);
	
	if (same_map) {
		c = map->foreachinmap(bg_ipcheck_sub, m, BL_PC, sd);
	} else {
		struct map_session_data *pl_sd;
		struct s_mapiterator* iter;
		iter = mapit_getallusers();
		for (pl_sd = (struct map_session_data *)mapit->first(iter); mapit->exists(iter); pl_sd = (struct map_session_data *)mapit->next(iter)) {
			if (!(pl_sd->bg_id || map->list[pl_sd->bl.m].flag.battleground))
				continue;
			if (sockt->session[sd->fd]->client_addr == sockt->session[pl_sd->fd]->client_addr)
				c++;
		}
		mapit->free(iter);
	}
	return c;
}

/**
 * Returns MapID Based of MapName.
 * @param name MapName
 * @return MapIndex(0 if none found)
 **/
unsigned short mapname2id(const char* name)
{
	int i;
	char map_name[MAP_NAME_LENGTH];

	mapindex->getmapname(name, map_name);

	if ((i = strdb_iget(mapindex->db, map_name)))
		return i;

	return 0;
}

/**
 * Changes Team of a Member
 * @param sd Map_Session_Data
 * @param team1 TeamID to Join
 * @param guild_id GuildID to Join
 **/
void bg_e_change_team(struct map_session_data *sd, int team1,int guild_id)
{
	int i, bg_id;
	struct battleground_data *bgd = NULL;
	struct sd_p_data* sd_data;

	if (sd == NULL)
		return;
	sd_data = pdb_search(sd, false);
	if (sd_data == NULL || !sd->bg_id)
		return;
	bg->send_dot_remove(sd);
	bg_id = sd->bg_id;

	if ((bgd = bg->team_search(bg_id)) == NULL)
		return;

	ARR_FIND(0, MAX_BG_MEMBERS, i, bgd->members[i].sd == sd);
	if (i < MAX_BG_MEMBERS) { // Removes member from BG
		if (sd->bg_queue.arena) {
			bg->queue_pc_cleanup(sd);
			pc->setpos(sd,bgd->members[i].source.map, bgd->members[i].source.x, bgd->members[i].source.y, CLR_OUTSIGHT);
		}
		memset(&bgd->members[i], 0, sizeof(bgd->members[0]));
	}

	clear_bg_guild_data(sd, bgd);

	if (sd->bg_queue.arena) {
		bg->queue_pc_cleanup(sd);
	}
	bg_e_team_join(team1, sd, guild_id);
	return;
}

#ifndef VIRT_GUILD
/**
 * Initializes Fake Guild Data.
 **/
void guild_data_load_eBG(void)
{
	struct guild *g;
	char* data;
	size_t len;
	char* p;
	int i;
	int guild_id;
	
	for (guild_id = GET_EBG_GUILD_ID(0); guild_id <= GET_EBG_GUILD_ID(TOTAL_GUILD); guild_id++) {
		g = NULL;

		if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT g.`name`,c.`name`,g.`guild_lv`,g.`connect_member`,g.`max_member`,g.`average_lv`,g.`exp`,g.`next_exp`,g.`skill_point`,g.`mes1`,g.`mes2`,g.`emblem_len`,g.`emblem_id`,g.`emblem_data` "
			"FROM `%s` g LEFT JOIN `%s` c ON c.`char_id` = g.`char_id` WHERE g.`guild_id`='%d'", "guild", "char", guild_id))
		{
			Sql_ShowDebug(map->mysql_handle);
			continue;
		}

		if (SQL_SUCCESS != SQL->NextRow(map->mysql_handle))
			continue;// Guild does not exists.

		CREATE(g, struct guild, 1);

		g->guild_id = guild_id;
		SQL->GetData(map->mysql_handle,  0, &data, &len); memcpy(g->name, data, min(len, NAME_LENGTH));
		SQL->GetData(map->mysql_handle,  1, &data, &len); memcpy(g->master, data, min(len, NAME_LENGTH));
		SQL->GetData(map->mysql_handle,  2, &data, NULL); g->guild_lv = atoi(data);
		SQL->GetData(map->mysql_handle,  3, &data, NULL); g->connect_member = atoi(data);
		SQL->GetData(map->mysql_handle,  4, &data, NULL); g->max_member = atoi(data);
		if (g->max_member > MAX_GUILD) {
			// Fix reduction of MAX_GUILD [PoW]
			ShowWarning("Guild %d:%s specifies higher capacity (%d) than MAX_GUILD (%d)\n", guild_id, g->name, g->max_member, MAX_GUILD);
			g->max_member = MAX_GUILD;
		}
		SQL->GetData(map->mysql_handle,  5, &data, NULL); g->average_lv = atoi(data);
		SQL->GetData(map->mysql_handle,  6, &data, NULL); g->exp = strtoull(data, NULL, 10);
		SQL->GetData(map->mysql_handle,  7, &data, NULL); g->next_exp = (unsigned int)strtoul(data, NULL, 10);
		SQL->GetData(map->mysql_handle,  8, &data, NULL); g->skill_point = atoi(data);
		SQL->GetData(map->mysql_handle,  9, &data, &len); memcpy(g->mes1, data, min(len, sizeof(g->mes1)));
		SQL->GetData(map->mysql_handle, 10, &data, &len); memcpy(g->mes2, data, min(len, sizeof(g->mes2)));
		SQL->GetData(map->mysql_handle, 11, &data, &len); g->emblem_len = atoi(data);
		SQL->GetData(map->mysql_handle, 12, &data, &len); g->emblem_id = atoi(data);
		SQL->GetData(map->mysql_handle, 13, &data, &len);
		// convert emblem data from hexadecimal to binary
		//TODO: why not store it in the db as binary directly? [ultramage]
		for (i = 0, p = g->emblem_data; i < g->emblem_len; ++i, ++p) {
			if (*data >= '0' && *data <= '9')
				*p = *data - '0';
			else if (*data >= 'a' && *data <= 'f')
				*p = *data - 'a' + 10;
			else if (*data >= 'A' && *data <= 'F')
				*p = *data - 'A' + 10;
			*p <<= 4;
			++data;

			if (*data >= '0' && *data <= '9')
				*p |= *data - '0';
			else if (*data >= 'a' && *data <= 'f')
				*p |= *data - 'a' + 10;
			else if (*data >= 'A' && *data <= 'F')
				*p |= *data - 'A' + 10;
			++data;
		}

		// load guild member info
		if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name` "
			"FROM `%s` WHERE `guild_id`='%d' ORDER BY `position`", "eBG_member", guild_id)) {
			Sql_ShowDebug(map->mysql_handle);
			aFree(g);
			continue;
		}
		for (i = 0; i < g->max_member && SQL_SUCCESS == SQL->NextRow(map->mysql_handle); ++i) {
			struct guild_member* m = &g->member[i];

			SQL->GetData(map->mysql_handle,  0, &data, NULL); m->account_id = atoi(data);
			SQL->GetData(map->mysql_handle,  1, &data, NULL); m->char_id = atoi(data);
			SQL->GetData(map->mysql_handle,  2, &data, NULL); m->hair = atoi(data);
			SQL->GetData(map->mysql_handle,  3, &data, NULL); m->hair_color = atoi(data);
			SQL->GetData(map->mysql_handle,  4, &data, NULL); m->gender = atoi(data);
			SQL->GetData(map->mysql_handle,  5, &data, NULL); m->class = atoi(data);
			SQL->GetData(map->mysql_handle,  6, &data, NULL); m->lv = atoi(data);
			SQL->GetData(map->mysql_handle,  7, &data, NULL); m->exp = strtoull(data, NULL, 10);
			SQL->GetData(map->mysql_handle,  8, &data, NULL); m->exp_payper = (unsigned int)atoi(data);
			SQL->GetData(map->mysql_handle,  9, &data, NULL); m->online = atoi(data);
			SQL->GetData(map->mysql_handle, 10, &data, NULL); m->position = atoi(data);
			if (m->position >= MAX_GUILDPOSITION) // Fix reduction of MAX_GUILDPOSITION [PoW]
				m->position = MAX_GUILDPOSITION - 1;
			SQL->GetData(map->mysql_handle, 11, &data, &len); memcpy(m->name, data, min(len, NAME_LENGTH));
			m->modified = 0x00;
		}

		//printf("- Read guild_position %d from sql \n",guild_id);
		if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `position`,`name`,`mode`,`exp_mode` FROM `%s` WHERE `guild_id`='%d'", "guild_position", guild_id)) {
			Sql_ShowDebug(map->mysql_handle);
			aFree(g);
			continue;
		}
		while( SQL_SUCCESS == SQL->NextRow(map->mysql_handle)) {
			int position;
			struct guild_position *pos;

			SQL->GetData(map->mysql_handle, 0, &data, NULL); position = atoi(data);
			if (position < 0 || position >= MAX_GUILDPOSITION)
				continue;// invalid position
			pos = &g->position[position];
			SQL->GetData(map->mysql_handle, 1, &data, &len); memcpy(pos->name, data, min(len, NAME_LENGTH));
			SQL->GetData(map->mysql_handle, 2, &data, NULL); pos->mode = atoi(data);
			SQL->GetData(map->mysql_handle, 3, &data, NULL); pos->exp_mode = atoi(data);
			pos->modified = 0x00;
		}

		//printf("- Read guild_alliance %d from sql \n",guild_id);
		if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `opposition`,`alliance_id`,`name` FROM `%s` WHERE `guild_id`='%d'", "guild_alliance", guild_id)) {
			Sql_ShowDebug(map->mysql_handle);
			aFree(g);
			continue;
		}
		for (i = 0; i < MAX_GUILDALLIANCE && SQL_SUCCESS == SQL->NextRow(map->mysql_handle); ++i) {
			struct guild_alliance* a = &g->alliance[i];

			SQL->GetData(map->mysql_handle, 0, &data, NULL); a->opposition = atoi(data);
			SQL->GetData(map->mysql_handle, 1, &data, NULL); a->guild_id = atoi(data);
			SQL->GetData(map->mysql_handle, 2, &data, &len); memcpy(a->name, data, min(len, NAME_LENGTH));
		}

		//printf("- Read guild_expulsion %d from sql \n",guild_id);
		if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `account_id`,`name`,`mes` FROM `%s` WHERE `guild_id`='%d'", "guild_expulsion", guild_id)) {
			Sql_ShowDebug(map->mysql_handle);
			aFree(g);
			continue;
		}
		for (i = 0; i < MAX_GUILDEXPULSION && SQL_SUCCESS == SQL->NextRow(map->mysql_handle); ++i) {
			struct guild_expulsion *e = &g->expulsion[i];

			SQL->GetData(map->mysql_handle, 0, &data, NULL); e->account_id = atoi(data);
			SQL->GetData(map->mysql_handle, 1, &data, &len); memcpy(e->name, data, min(len, NAME_LENGTH));
			SQL->GetData(map->mysql_handle, 2, &data, &len); memcpy(e->mes, data, min(len, sizeof(e->mes)));
		}

		//printf("- Read guild_skill %d from sql \n",guild_id);
		if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `id`,`lv` FROM `%s` WHERE `guild_id`='%d' ORDER BY `id`", "guild_skill", guild_id)) {
			Sql_ShowDebug(map->mysql_handle);
			aFree(g);
			continue;
		}

		for (i = 0; i < MAX_GUILDSKILL; i++) {
			//Skill IDs must always be initialized. [Skotlex]
			g->skill[i].id = i + GD_SKILLBASE;
		}

		while( SQL_SUCCESS == SQL->NextRow(map->mysql_handle)) {
			int id;
			SQL->GetData(map->mysql_handle, 0, &data, NULL); id = atoi(data) - GD_SKILLBASE;
			if (id < 0 || id >= MAX_GUILDSKILL)
				continue;// invalid guild skill
			SQL->GetData(map->mysql_handle, 1, &data, NULL); g->skill[id].lv = atoi(data);
		}
		SQL->FreeResult(map->mysql_handle);

		idb_put(guild->db, guild_id, g); //Add to cache
		g->save_flag |= 0x8000; //But set it to be removed, in case it is not needed for long.
		g->channel = channel->create(HCS_TYPE_ALLY, channel->config->ally_name, channel->config->ally_color);
		
		ShowInfo("Guild loaded (%d - %s)\n", guild_id, g->name);
	}
}
#endif

/**
 * Makes a Player Join a BG Team
 * @param bg_id BG_ID in which a player joined
 * @param sd Map_Session_Data
 * @param guild_id GuildID to Join..
 * @return 0 if failed, else 1
 **/
int bg_e_team_join(int bg_id, struct map_session_data *sd, int guild_id)
{
	int i;
	struct battleground_data *bgd = bg->team_search(bg_id);
	struct sd_p_data *data;
	struct bg_extra_info *bg_data_t;

	if (sd == NULL)
		return 0;
	if (bgd == NULL) {
		clif->message(sd->fd, "Battleground Data is NULL.");
		return 0;
	}
	if (sd->bg_id) {
		clif->message(sd->fd, "You are already on Battleground.");
		return 0;
	}
	
	if (bg_queue_townonly && !map->list[sd->bl.m].flag.town) {
		clif->message(sd->fd, "You can only join when in town.");
		return 0;
	}

	ARR_FIND(0, MAX_BG_MEMBERS, i, bgd->members[i].sd == NULL);
	if (i == MAX_BG_MEMBERS) {
		clif->message(sd->fd, "Team is Full.");
		return 0; // No free slots
	}
	
	if (sd->bg_id) {
		clif->message(sd->fd, "You are already on eBattleground.");
		return 0;
	}
	data = pdb_search(sd, true);
	
	bg_data_t = bg_extra_create(bgd, true);
	
	if (bg_data_t->init==false) {
		bg_data_t->creation_time = timer->gettick();
		bg_data_t->init = true;
	}
#ifndef LEADER_INT
	if (!bg_data_t->leader->char_id) {
		bg_data_t->leader->char_id = sd->status.char_id;
#else
	if (!bg_data_t->leader) {
		bg_data_t->leader = sd->status.char_id;
#endif
		data->leader = true;
		clif->message(sd->fd,"You are Leader of your Team");
	} else {
		data->leader = false;
	}
	
	sd->bg_id = bg_id;
	bgd->members[i].sd = sd;
	bgd->members[i].x = sd->bl.x;
	bgd->members[i].y = sd->bl.y;
	
	data->eBG = true;
	data->flag.ebg_afk = 0;
	sd->idletime = sockt->last_tick;	// Set Idle tick to current tick
	data->ranked.match = false;
	
	/* Guild Join [Dastgir/Hercules] **/
	if (guild_id) {
		struct guild* g;
#ifdef VIRT_GUILD
		bg_data_t->g = &bg_guild[guild_id-1];
		g = bg_data_t->g;
#else
		if (data->g) {
			eBG_Guildremove(sd,sd->guild);
			sd->guild = data->g;
			sd->status.guild_id = data->g->guild_id;
			data->g = NULL;
		}
		data->g = sd->guild;
		sd->guild = guild->search(GET_EBG_GUILD_ID(guild_id) - 1);	//bg_guild[guild_id]
		eShowDebug("Guild_ID: %d(%d):%s\n", GET_EBG_GUILD_ID(guild_id) - 1, sd->guild->guild_id, sd->guild->name);
		sd->status.guild_id = sd->guild->guild_id;
		intif->guild_emblem(sd->guild->guild_id, sd->guild->emblem_len, sd->guild->emblem_data);
		eShowDebug("Guild Emblem_Len:ID: %d %d\n", sd->guild->emblem_len, sd->guild->emblem_id);
		eBG_Guildadd(sd, sd->guild);
		g = sd->guild;
#endif
		guild->send_memberinfoshort(sd,1);
		clif->guild_belonginfo(sd,g);
		clif->charnameupdate(sd);
		g->connect_member++;
	}
	/* Guild Join [Dastgir/Hercules] **/
	
	/* populate 'where i came from' **/
	if (map->list[sd->bl.m].flag.nosave || map->list[sd->bl.m].instance_id >= 0) {
		struct map_data *m=&map->list[sd->bl.m];
		if (m->save.map)
			memcpy(&bgd->members[i].source,&m->save,sizeof(struct point));
		else
			memcpy(&bgd->members[i].source,&sd->status.save_point,sizeof(struct point));
	} else
		memcpy(&bgd->members[i].source,&sd->status.last_point,sizeof(struct point));
	bgd->count++;

	if (mapreg->readreg(script->add_str("$BGRanked")) && bg_ranked_mode && data->esd->bg.ranked_games < bg_max_rank_game  &&  (int)DIFF_TICK(timer->gettick(), bg_data_t->creation_time) < 60) {	//60 Seconds Time Limit for Getting Ranked.
		int *total_ranked_game;
		char output[128];
		time_t clock;
		struct tm *t;
		
		time(&clock);
		t = localtime(&clock);
		
		data->ranked.match = true;
		data->esd->bg.ranked_games++;
		SET_VARIABLE_ADD(sd, total_ranked_game, BG_TOTAL_RANKED_GAMES, 1, int);
		
		pc_setglobalreg(sd,script->add_str("bg_ranked"),t->tm_mday);
		sprintf(output,"-- Ranked Battleground Match %d Of %d --", data->esd->bg.ranked_games, bg_max_rank_game);
		clif->message(sd->fd,output);
	}
	
	guild->send_dot_remove(sd);
	bg_send_char_info(sd);
	clif->charnameupdate(sd);
	
	for (i = 0; i < MAX_BG_MEMBERS; i++) {
		struct map_session_data *pl_sd = bgd->members[i].sd;
		if (pl_sd == NULL)
			continue;
		/// Send Guild Info First 
		clif->guild_basicinfo(pl_sd);
#ifndef VIRT_GUILD
		clif->guild_emblem(pl_sd, pl_sd->guild);
#else
		clif->guild_emblem(pl_sd, bg_data_t->g);
#endif
		send_bg_memberlist(pl_sd);
		clif->guild_positionnamelist(pl_sd);
		clif->guild_memberlist(pl_sd);
		clif->guild_positioninfolist(pl_sd);
		if (pl_sd != sd)
			clif->hpmeter_single(sd->fd, pl_sd->bl.id, pl_sd->battle_status.hp, pl_sd->battle_status.max_hp);
	}

	clif->bg_hp(sd);
	clif->bg_xy(sd);
	if (guild_id >= 0)
		clif->guild_emblem_area(&sd->bl);
	return 1;
}

/**
 * Builds Fake Guild Data
 **/
void bg_guild_build_data(void) {
	int i = 1, k, skill_id;
	memset(&bg_guild, 0, sizeof(bg_guild));
	for (; i <= TOTAL_GUILD; i++) { // Emblem Data - Guild ID's
		FILE* fp = NULL;
		char path_db[256];
		struct guild* g = NULL;
		int j = i - 1;
#ifdef VIRT_GUILD
		g = &bg_guild[j];
		if (g == NULL) {
			ShowError("Guild %d:%d Doesnt Exist\n", j, GET_EBG_GUILD_ID(j));
			continue;
		}
		g->guild_id = GET_EBG_GUILD_ID(j); // Last 13 ID's
#else
		g = guild->search(GET_EBG_GUILD_ID(i));
		if (g == NULL) {
			ShowError("Guild %d Doesnt Exist\n", GET_EBG_GUILD_ID(i);
			guild->request_info(GET_EBG_GUILD_ID(i));
			continue;
		}
#endif
		g->emblem_id = 1; // Emblem Index
		g->guild_lv = 1;
		g->max_member = MAX_BG_MEMBERS;

		// Skills
		if (j < 3) {
			for (k = 0; k < MAX_GUILDSKILL; k++) {
				skill_id = k + GD_SKILLBASE;
				g->skill[k].id = skill_id;
				switch( skill_id) {
				case GD_GLORYGUILD:
					g->skill[k].lv = 0;
					break;
				case GD_APPROVAL:
				case GD_KAFRACONTRACT:
				case GD_GUARDRESEARCH:
				case GD_BATTLEORDER:
				case GD_RESTORE:
				case GD_EMERGENCYCALL:
				case GD_DEVELOPMENT:
					g->skill[k].lv = 1;
					break;
				case GD_GUARDUP:
				case GD_REGENERATION:
					g->skill[k].lv = 3;
					break;
				case GD_LEADERSHIP:
				case GD_GLORYWOUNDS:
				case GD_SOULCOLD:
				case GD_HAWKEYES:
					g->skill[k].lv = 5;
					break;
				case GD_EXTENSION:
					g->skill[k].lv = 10;
					break;
				}
			}
		}
		else { // Other Data
			snprintf(g->name, NAME_LENGTH, "Team %d", i - 3); // Team 1, Team 2 ... Team 10
			strncpy(g->master, g->name, NAME_LENGTH);
			snprintf(g->position[0].name, NAME_LENGTH, "%s Leader", g->name);
			strncpy(g->position[1].name, g->name, NAME_LENGTH);
		}

		sprintf(path_db, "db/emblems/bg_%d.ebm", i);
		if ((fp = fopen(path_db, "rb")) != NULL) {
			fseek(fp, 0, SEEK_END);
			g->emblem_len = (int)ftell(fp);
			fseek(fp, 0, SEEK_SET);
			if (fread(g->emblem_data, 1,g->emblem_len, fp) != g->emblem_len) {
				ShowWarning("bg_guild_build_data: db/emblems/bg_%d.ebm Cannot be read properly.\n", i);
			} else {
				eShowDebug("MaxMember/GuildId:%d/%d\n", g->max_member, g->guild_id);
				intif->guild_emblem(g->guild_id, g->emblem_len, g->emblem_data);
				ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"' emblem data file.\n", path_db);
			}
			fclose(fp);
		}
		switch(i) {
			case 1:
				strncpy(g->name, "Blue Team", NAME_LENGTH);
				strncpy(g->master, "General Guillaume", NAME_LENGTH);
				strncpy(g->position[0].name, "Blue Team Leader", NAME_LENGTH);
				strncpy(g->position[1].name, "Blue Team", NAME_LENGTH);
				break;
			case 2:
				strncpy(g->name, "Red Team", NAME_LENGTH);
				strncpy(g->master, "Prince Croix", NAME_LENGTH);
				strncpy(g->position[0].name, "Red Team Leader", NAME_LENGTH);
				strncpy(g->position[1].name, "Red Team", NAME_LENGTH);
				break;
			case 3:
				strncpy(g->name, "Green Team", NAME_LENGTH);
				strncpy(g->master, "Mercenary", NAME_LENGTH);
				strncpy(g->position[0].name, "Green Team Leader", NAME_LENGTH);
				strncpy(g->position[1].name, "Green Team", NAME_LENGTH);
				break;
		}
	}
}

/**
 * Clears all Data that were generated upon TeamJoin
 * bg->team_leave PreHooked.
 * @see bg_team_leave
 **/
int bg_e_team_leave(struct map_session_data **sd_, enum bg_team_leave_type* flag) {
	struct sd_p_data* data;
	struct map_session_data *sd = *sd_;
	struct battleground_data *bgd = NULL;
#ifdef VIRT_GUILD
	struct bg_extra_info *bg_data_t;
	bgd = bg->team_search(sd->bg_id);
	bg_data_t = bg_extra_create(bgd, false);
#endif
	data = pdb_search(sd, false);
	if (data == NULL || !data->eBG || !sd->bg_id)
		return 0;
	eShowDebug("BG_LEAVE: %u ID \n", sd->bg_id);
	
	if ( data &&
#ifdef VIRT_GUILD
		bg_data_t && bg_data_t->g
#else
		data->g
#endif
	) {
#ifndef VIRT_GUILD
		eShowDebug("BG Leave: Guild Found\n");
		sd->guild->connect_member--;
		eBG_Guildremove(sd,sd->guild);
		eShowDebug("BG Leave: Guild Removed\n");
		if (data->g) {
			sd->status.guild_id = data->g->guild_id;
			sd->guild = data->g;
			eShowDebug("BG Leave: sd->g = data->g\n");
		}else {
			sd->status.guild_id = 0;
			sd->guild = NULL;
			eShowDebug("BG Leave: sd->g = 0\n");
		}
			
		data->g = NULL;
#endif
	} else {
		eShowDebug("team_leave: BG data not parsed.\n");
	}
	if (data && data->leader) {
		eBG_ChangeLeader(data, sd->bg_id);
	}
	npc->event(sd, "BG_Settings::OnLeaveBGSource", 0);
	// Sets bg_id to 0
	clear_bg_guild_data(sd, bgd);
	
	if (bgd != NULL) {
		--bgd->count;
		return bgd->count;
	} else {
		return 0;
	}
}


/**
 * Generates BGFame for Regular and Ranked Modes.
 * @return 1
 **/
int bg_fame_receive(void) {
#ifdef CHAR_SERVER_SAVE
	if (!chrif->isconnected()) {
		return 0;
	}
	WFIFOHEAD(chrif->fd, 4);
	WFIFOW(chrif->fd, 0) = PACKET_MC_REQ_FAME_DATA;
	WFIFOW(chrif->fd, 2) = 3; // Rank + Regular
	WFIFOSET(chrif->fd, 4);
#else
#define min_num(a, b) ((a) < (b) ? (a) : (b))
	int i;
	char* data;
	size_t len;
	memset(bgrank_fame_list, 0, sizeof(bgrank_fame_list));
	memset(bg_fame_list, 0, sizeof(bg_fame_list));
	// BG Ranked.
	if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `eBG_main`.`char_id`, `eBG_main`.`rank_points`, `char`.`name` FROM `eBG_main` LEFT JOIN `char` ON `char`.`char_id` = `eBG_main`.`char_id` WHERE `eBG_main`.`rank_points` > '0' ORDER BY `eBG_main`.`rank_points` DESC LIMIT 0,%d", fame_list_size_bgrank))
		Sql_ShowDebug(map->mysql_handle);
	for (i = 0; i < fame_list_size_bgrank && SQL_SUCCESS == SQL->NextRow(map->mysql_handle); ++i) {
		SQL->GetData(map->mysql_handle, 0, &data, NULL); bgrank_fame_list[i].id = atoi(data);
		SQL->GetData(map->mysql_handle, 1, &data, NULL); bgrank_fame_list[i].fame = atoi(data);
		SQL->GetData(map->mysql_handle, 2, &data, &len); 
		if (data == NULL)
			data = "Name-NULL";
		memcpy(bgrank_fame_list[i].name, data, min_num(len, NAME_LENGTH));
	}
	SQL->FreeResult(map->mysql_handle);
	// BG Normal
	if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `eBG_main`.`char_id`, `eBG_main`.`points`, `char`.`name` FROM `eBG_main` LEFT JOIN `char` ON `char`.`char_id` = `eBG_main`.`char_id` WHERE `eBG_main`.`points` > '0' ORDER BY `eBG_main`.`points` DESC LIMIT 0,%d",  fame_list_size_bg))
		Sql_ShowDebug(map->mysql_handle);
	for (i = 0; i < fame_list_size_bg && SQL_SUCCESS == SQL->NextRow(map->mysql_handle); ++i) {
		SQL->GetData(map->mysql_handle, 0, &data, NULL); bg_fame_list[i].id = atoi(data);
		SQL->GetData(map->mysql_handle, 1, &data, NULL); bg_fame_list[i].fame = atoi(data);
		SQL->GetData(map->mysql_handle, 2, &data, &len);
		if (data == NULL)
			data = "Name-NULL";
		memcpy(bg_fame_list[i].name, data, min_num(len, NAME_LENGTH));
	}
	SQL->FreeResult(map->mysql_handle);
#undef min_num
#endif
	return 1;
}

void ebg_request_save_data(int64 save_type, struct sd_p_data *sd_data, struct map_session_data *sd)
{
#ifdef CHAR_SERVER_SAVE
	int offset = 0;
	unsigned char buf[2048];

	nullpo_retv(sd);
	nullpo_retv(sd_data);

	set_ebg_buffer(buf, &offset, &save_type, sd_data->esd);

	if (!chrif->isconnected())
		return;

	WFIFOHEAD(chrif->fd, offset + 18);
	WFIFOW(chrif->fd, 0) = PACKET_MC_SYNC_DATA;
	WFIFOL(chrif->fd, 2) = offset + 18;
	WFIFOL(chrif->fd, 6) = sd->status.char_id;
	WFIFOQ(chrif->fd, 10) = save_type;
	memcpy(WFIFOP(chrif->fd, 18), buf, offset);	
	WFIFOSET(chrif->fd, offset + 18);
#else
#define GET_SAVE_TYPE(saveType, saveFunc) \
				if (save_type&saveType) { \
					if (saveType == EBG_SAVE_LEADER) { \
						BG_CREATE_DATA(sd_data, leader, bg_status_data, 3); \
					} \
					saveFunc(sd, map->mysql_handle, sd->status.char_id, sd_data, save_type);
				}
	GET_SAVE_TYPE(EBG_SAVE_ALL_COMMON, bg_save_common);
	GET_SAVE_TYPE(EBG_SAVE_BOSS, bg_save_boss);
	GET_SAVE_TYPE(EBG_SAVE_CONQ, bg_save_conq);
	GET_SAVE_TYPE(EBG_SAVE_CTF, bg_save_ctf);
	GET_SAVE_TYPE(EBG_SAVE_DOM, bg_save_dom);
	GET_SAVE_TYPE(EBG_SAVE_EOS, bg_save_eos);
	GET_SAVE_TYPE(EBG_SAVE_LEADER, bg_save_leader);
	GET_SAVE_TYPE(EBG_SAVE_BG_RANK_ALL, bg_save_ranking);
	GET_SAVE_TYPE(EBG_SAVE_WOE_RANK_ALL, bg_save_ranking);
	GET_SAVE_TYPE(EBG_SAVE_RUSH, bg_save_rush);
	GET_SAVE_TYPE(EBG_SAVE_SC, bg_save_sc);
	GET_SAVE_TYPE(EBG_SAVE_TDM, bg_save_tdm);
	GET_SAVE_TYPE(EBG_SAVE_TI, bg_save_ti);
#undef GET_SAVE_TYPE
#endif
}

#define EBG_SAVE_DATA_INIT(high_const, low_const, data, data_type, map_type) \
		int i; \
		int sub_with = low_const; \
		bool save = false; /* Save the Data? **/ \
		data_type values[(high_const)-(low_const)+1]; \
		data_type *value_ptr; \
		if (data == NULL)	\
			return;	\
		for (i=(low_const); i <= (high_const); i++) { \
			if (map_type == MAP_IS_NONE) { \
				value_ptr = GET_VARIABLE_SIZE(sd, i, false, data_type); \
			} else if (map_type == MAP_IS_BG) { \
				value_ptr = GET_VARIABLE_SIZE(sd, MakeDWord(i, RANKING_BG_FORCE), false, data_type); \
			} else if (map_type == MAP_IS_WOE) { \
				value_ptr = GET_VARIABLE_SIZE(sd, MakeDWord(i, RANKING_WOE_FORCE), false, data_type); \
			} \
			if (value_ptr != NULL) { \
				save = true; \
				values[i-(low_const)] = *value_ptr; \
			} else { \
				values[i-(low_const)] = 0; \
			} \
		}

/**
 * map->quit PreHooked
 * Executes pc_logout_action
 * @see map_quit
 * @see pc_logout_action
 * @return 0
 **/
int bg_clear_char_data_hook(struct map_session_data **sd) {
	pc_logout_action(*sd);
	return 0;
}

/**
 * Adds a Player to the Guild
 * @param sd Map_Session_Data
 * @param g guild Struct
 * @return 1
 **/
int eBG_Guildadd(struct map_session_data *sd, struct guild* g) {
	struct guild_member m;
	int i;
	struct sd_p_data* data;
	ARR_FIND( 0, g->max_member, i, g->member[i].account_id == 0);
	if (i == g->max_member)
		return 0;
	guild->makemember(&m,sd);
	sd->guild_invite = 1;
	data = pdb_search(sd, false);
	
	if (data == NULL)
		return 0;
	
	for (i=0;i<g->max_member;i++) {
		if (g->member[i].account_id==sd->status.account_id)
			break;
		else if (g->member[i].account_id==0) {
			memcpy(&g->member[i],&m,sizeof(struct guild_member));
			g->member[i].modified = (0x02 | 0x01);
			guild->member_added(g->guild_id,m.account_id,m.char_id, 0);
			g->save_flag |= 0x0100;
			g->member[i].position = (data->leader?0:1);
			guild->recv_info(g);

			g->save_flag |= 0x0002;
			if (g->save_flag&0x8000)
				g->save_flag&=~0x8000;
			return 1;    //Yes, intended return;
		}
		
	}
	return 1;
}

/**
 * Removes a Player from Guild
 * @param sd Map_Session_Data
 * @param g guild Struct
 * @return 1
 **/
int eBG_Guildremove(struct map_session_data *sd, struct guild* g) {
	int i;
	i = guild->getindex(g, sd->status.account_id, sd->status.char_id);
	if (i != -1 && strcmp(g->member[i].name,g->master) != 0) //Can't expel the GL!
		intif->guild_leave(g->guild_id,sd->status.account_id,sd->status.char_id,1,"Removed-ExtendedBG");
	return 1;
}

/**
 * clif->guild_memberlist PreHooked.
 * Sends BGMemberData instead of GuildMember to Guild Window
 * @see clif_guild_memberlist
 **/
void send_bg_memberlist_(struct map_session_data **sd_)
{
	struct map_session_data *sd = *sd_;
	struct battleground_data *bgd = bg->team_search(sd->bg_id);
#ifdef VIRT_GUILD
	struct bg_extra_info *bg_data_t;
	if (bgd != NULL && (bg_data_t = bg_extra_create(bgd, false)) != NULL && bg_data_t->g != NULL)
#else
	if (sd->status.guild_id >= GET_EBG_GUILD_ID(0) && sd->status.guild_id <= GET_EBG_GUILD_ID(TOTAL_GUILD))
#endif
	{
		send_bg_memberlist(sd);
		hookStop();
	}
}

/**
 * Sends BGMemberData instead of GuildMember to Guild Window
 * @param sd Map_Session_Data
 **/
void send_bg_memberlist(struct map_session_data *sd)
{
	int fd;
	int i, c;
	struct battleground_data *bgd;
	struct map_session_data *psd;
	struct sd_p_data *data;
#if PACKETVER < 20161026
	const int cmd = 0x154;
	const int size = 104;
#else
	const int cmd = 0xaa5;
	const int size = 34;
#endif
	nullpo_retv(sd);

	if ((fd = sd->fd) == 0)
		return;
	
	data = pdb_search(sd, false);
	
	if (data == NULL || !sd->bg_id || (bgd = bg->team_search(sd->bg_id)) == NULL)
		return;
	
	WFIFOHEAD(fd, bgd->count * size + 4);
	WFIFOW(fd, 0) = cmd;
	eShowDebug("%d - Bg_Count \n", bgd->count);
	for (i = 0, c = 0; i < bgd->count; i++) {
		if ((psd = bgd->members[i].sd) == NULL)
			continue;
		data = pdb_search(psd, false);
		if (data == NULL)
			continue;
		eShowDebug("Accounts %d-%d-%d-%s\n",i,psd->status.account_id,psd->status.char_id,psd->status.name);
		WFIFOL(fd, c * size + 4) = psd->status.account_id;
		WFIFOL(fd, c * size + 8) = psd->status.char_id;
		WFIFOW(fd, c * size + 12) = psd->status.hair;
		WFIFOW(fd, c * size + 14) = psd->status.hair_color;
		WFIFOW(fd, c * size + 16) = psd->status.sex;
		WFIFOW(fd, c * size + 18) = psd->status.class;
		WFIFOW(fd, c * size + 20) = psd->status.base_level;
		WFIFOL(fd, c * size + 22) = data->kills;
		WFIFOL(fd, c * size + 26) = 1;
		WFIFOL(fd, c * size + 30) = data->leader ? 0 : 1;
#if PACKETVER < 20161026
		if (data->flag.ebg_afk) {
			safestrncpy(WFIFOP(fd, c * size + 34), "AFK", 50);
		} else {
			memset(WFIFOP(fd, c * size + 34), 0, 50);
		}
		safestrncpy(WFIFOP(fd, c * size + 84), psd->status.name, NAME_LENGTH);
#else
		WFIFOL(fd, c * size + 34) = (int)psd->status.last_login;
#endif

		c++;
	}
	WFIFOW(fd, 2) = c * size + 4;
	WFIFOSET(fd, c * size + 4);
}

/**
 * clif->guild_basicinfo PreHooked.
 * Sends BGData instead of GuildData to Guild Window
 * @see clif_guild_basicinfo
 **/
void send_bg_basicinfo(struct map_session_data **sd_)
{	//Guild Info List
	struct map_session_data *sd = *sd_;
	int fd;
	struct guild *g;
	struct battleground_data *bgd;
	struct sd_p_data *sd_data;
	struct bg_extra_info *bg_data_t;
#if PACKETVER < 20160622
	const int cmd = 0x1b6;  //0x150; [4144] this is packet for older versions?
#else
	const int cmd = 0xa84;
#endif

	nullpo_retv(sd);

	fd = sd->fd;
	sd_data = pdb_search(sd, false);

	if (!sd->bg_id || (bgd = bg->team_search(sd->bg_id)) == NULL)
		return;

	if (sd_data) {
		bg_data_t = bg_extra_create(bgd, false);
		if (bg_data_t == NULL)
			return;
	} else {
		return;
	}

#ifdef VIRT_GUILD
	g = bg_data_t->g;
#else
	g = sd->guild;
#endif

	if (g == NULL)
		return;
	
	WFIFOHEAD(fd, (clif->packet(cmd))->len);
	WFIFOW(fd, 0) = cmd;
	WFIFOL(fd, 2) = g->guild_id;
	WFIFOL(fd, 6) = g->guild_lv;
	WFIFOL(fd,10) = bgd->count;
	WFIFOL(fd,14) = g->max_member;
	WFIFOL(fd,18) = g->average_lv;
	WFIFOL(fd,22) = (uint32) cap_value(g->exp,0,INT32_MAX);
	WFIFOL(fd,26) = g->next_exp;
	WFIFOL(fd,30) = 0;    // Tax Points
	WFIFOL(fd,34) = 0;    // Honor: (left) Vulgar [-100,100] Famed (right)
	WFIFOL(fd,38) = 0;    // Virtue: (down) Wicked [-100,100] Righteous (up)
	WFIFOL(fd,42) = g->emblem_id;
	memcpy(WFIFOP(fd, 46), g->name, NAME_LENGTH);
#if PACKETVER < 20160622
	memcpy(WFIFOP(fd, 70), g->master, NAME_LENGTH);
	safestrncpy(WFIFOP(fd, 94), msg_sd(sd, 300 + guild->checkcastles(g)), 16);  // "'N' castles"
	WFIFOL(fd, 110) = 0;  // zeny
#else
	safestrncpy(WFIFOP(fd, 70), msg_sd(sd, 300 + guild->checkcastles(g)), 16);  // "'N' castles"
	WFIFOL(fd, 86) = 0;  // zeny
#ifdef LEADER_INT
	WFIFOL(fd, 90) = bg_data_t->leader;
	bg_data_t->leader = bg_sd->status.char_id;
#else
	if (bg_data_t->leader != NULL) {
		WFIFOL(fd, 90) = bg_data_t->leader->char_id;
	} else {
		WFIFOL(fd, 90) = 0;
	}
#endif
#endif

	WFIFOSET(fd, (clif->packet(cmd))->len);
	hookStop();
}

/**
 * Sends Guild Data of a Character for Display
 * @param sd Map_Session_Data 
 **/
void bg_send_char_info(struct map_session_data *sd)
{
	int fd;
	struct guild *g = NULL;
	struct sd_p_data *sd_data;
#ifdef VIRT_GUILD
	struct bg_extra_info *bg_data_t;
#endif
	nullpo_retv(sd);
	
	sd_data = pdb_search(sd, false);
	if (sd_data == NULL || !sd->bg_id)
		 return;
#ifdef VIRT_GUILD
	bg_data_t = bg_extra_create(bg->team_search(sd->bg_id), false);
	if (bg_data_t != NULL && bg_data_t->g != NULL)
		g = bg_data_t->g;
#else
	g = sd->guild;
#endif
	if (g == NULL) {
		eShowDebug("bg_send_char_info: Guild Not Found.\n");
		return;
	}

	fd = sd->fd;
	WFIFOHEAD(fd,43);
	memset(WFIFOP(fd,0),0,43);
	WFIFOW(fd,0) = 0x16c;
	WFIFOL(fd,2) = g->guild_id;
	WFIFOL(fd,6) = g->emblem_id;
	WFIFOL(fd,10) = 0;
	WFIFOB(fd,14) = 0;
	WFIFOL(fd,15) = 0;
	memcpy(WFIFOP(fd,19), g->name, NAME_LENGTH);
	WFIFOSET(fd,43);
}

/**
 * clif->sendbgemblem_area PreHooked.
 * Sends GuildEmblem to Area.
 * @see clif_sendbgemblem_area
 **/
void send_bg_emblem_area(struct map_session_data **sd_) {
	struct sd_p_data *data;
	struct map_session_data *sd = *sd_;
	struct block_list* bl = &sd->bl;

	nullpo_retv(bl);

	data = pdb_search(sd, false);
	if (data && data->eBG) {	
		eShowDebug("SDEmblem:%d,bl:%d\n",sd->guild->emblem_id,status->get_emblem_id(bl));
		eShowDebug("SDGuild:%d,bl:%d\n",sd->guild->guild_id,status->get_guild_id(bl));
		clif->guild_emblem_area(bl);
		hookStop();
	}
}

/**
 * clif->sendbgemblem_single PreHooked.
 * Sends GuildEmblem to Requested Client.
 * @see clif_sendbgemblem_single
 **/
void send_bg_emblem_single(int *fd, struct map_session_data **sd_) {
	struct sd_p_data *data;
	struct guild *g = NULL;
	struct map_session_data *sd = *sd_;
#ifdef VIRT_GUILD
	struct bg_extra_info *bg_data_t;
	struct sd_p_data *sd_data = pdb_search(sd, false);

	if (sd_data && sd->bg_id) {
		bg_data_t = bg_extra_create(bg->team_search(sd->bg_id), false);
		if (bg_data_t == NULL || !bg_data_t->g)
			g = NULL;
		else 
			g = bg_data_t->g;
	}
#else
	g = sd->guild;
#endif

	nullpo_retv(sd);

	data = pdb_search(sd, false);
	if (data && data->eBG) {	
		clif->guild_emblem(sd,g);
		hookStop();
	}
}

/**
 * clif->pGuildRequestEmblem PreHooked.
 * Sends GuildEmblem to Requested Client.
 * @see clif_parse_GuildRequestEmblem
 **/
void send_bg_emblem_single_(int *fd, struct map_session_data **sd_) {
	struct sd_p_data *data;
	struct guild *g = NULL;
	struct map_session_data *sd = *sd_;
#ifdef VIRT_GUILD
	struct bg_extra_info *bg_data_t;
	struct sd_p_data *sd_data;
#endif
	nullpo_retv(sd);

#ifdef VIRT_GUILD
	sd_data = pdb_search(sd, false);
	if (sd_data && sd->bg_id) {
		bg_data_t = bg_extra_create(bg->team_search(sd->bg_id), false);
		if (bg_data_t == NULL || bg_data_t->g == NULL)
			g = NULL;
		else
			g = bg_data_t->g;
	}
#else
	g = sd->guild;
#endif

	data = pdb_search(sd, false);
	if (data && data->eBG) {
		if (g != NULL)
			clif->guild_emblem(sd, g);
		hookStop();
	}
}

/**
 * clif->maptypeproperty2 PreHooked.
 * Makes client believe its Guild.
 * @see clif_maptypeproperty2
 **/
void send_map_property(struct block_list **bl, enum send_target *_t) {
#if PACKETVER >= 20121010
	enum send_target t = *_t;
	struct sd_p_data *data;
	struct packet_maptypeproperty2 p;
	struct map_session_data *sd = NULL;
	nullpo_retv(*bl);

	sd = BL_CAST(BL_PC, *bl);
	if (sd) {
		data = pdb_search(sd, false);
		if (data && data->eBG) {	
			p.PacketType = maptypeproperty2Type;
			p.type = 0x28;
			p.flag.party = 0;             // 1 in PvP
			p.flag.guild = 1;             // 1 in BG
			p.flag.siege = 1;             // 1 in BG
			p.flag.mineffect = 0;         // 1 in WoE
			p.flag.nolockon = 0;          // TODO
			p.flag.countpk = 0;           // 1 in PvP
			p.flag.nopartyformation = 0;  // 1 When PartyLock
			p.flag.bg = 1;                // 1 in BG
			p.flag.nocostume = 0;         // TODO
			p.flag.summonstarmiracle = 0; // TODO
			p.flag.usecart = 1;           // TODO
			p.flag.SpareBits = 0;
			clif->send(&p, sizeof(p), *bl, t);
			hookStop();
		}
		eShowDebug("Map_Type_Property: %d:%u:%d:%d:%d",p.flag.siege,t,sd->bl.m,ALL_SAMEMAP,maptypeproperty2Type);
	}
#endif
}

/**
 * clif->map_type PreHooked.
 * Avoids BG MapType.
 * @see clif_map_type
 **/
void Stop_This_Shitty_Battleground_Check(struct map_session_data **sd, enum map_type *type) {
	if (*sd != NULL) {
		struct sd_p_data *data = pdb_search(*sd, false);
		if (data && data->eBG) {
			clif->map_property(*sd, MAPPROPERTY_AGITZONE);
			hookStop();
		}
	}
}

/**
 * skill->check_condition_castbegin PreHooked. to allow execution of GuildSkills by Leader on eBG.
 * @see skill_check_condition_castbegin
 **/
int gmaster_skill_cast(struct map_session_data **sd_, uint16 *skill_id, uint16 *skill_lv)
{
	struct map_session_data *sd = *sd_;
	struct sd_p_data *data = pdb_search(sd, false);
	if (data == NULL)
		return 0;
	
	switch(*skill_id) {
		case GD_BATTLEORDER:
		case GD_REGENERATION:
		case GD_RESTORE:
			if (map->list[sd->bl.m].flag.battleground && data->eBG==true) {
				eShowDebug("GD_xxx: HookStopped.1");
				hookStop();
				return 1;
			}
			break;
		case GD_EMERGENCYCALL:
			// other checks were already done in skillnotok()
			// Todo: Add bg_data_t->leader check?
			if (map->list[sd->bl.m].flag.battleground && data->eBG == true && data->leader == true) {
				hookStop();
				return 1;
			}
			break;
	}
	return 0;
}

/**
 * clif->pUseSkillToId PreHooked. To Allow Execution of Guild Skills.
 * @see clif_parse_UseSkillToID
 **/
void unit_guild_skill(int *fd_, struct map_session_data **sd_)
{
	struct map_session_data *sd = *sd_;
	int fd = *fd_;
	uint16 skill_id, skill_lv;
	int tmp, target_id;
	int64 tick = timer->gettick();
	bool stopHook = false;
	
	skill_lv = RFIFOW(fd,2);
	skill_id = RFIFOW(fd,4);
	target_id = RFIFOL(fd,6);
	eShowDebug("%d - %d - %d\n",skill_lv,skill_id,target_id);
	if (skill_lv < 1) skill_lv = 1; //No clue, I have seen the client do this with guild skills :/ [Skotlex]
	if (skill_id < GD_SKILLBASE || sd->skillitem)
		return;

	tmp = skill->get_inf(skill_id);
	if (tmp&INF_GROUND_SKILL || !tmp)
		return; //Using a ground/passive skill on a target? WRONG.
	if (sd->npc_id || sd->state.workinprogress&1) {
#if PACKETVER >= 20110308
		clif->msgtable(sd, MSG_BUSY);
#else
		clif->messagecolor_self(fd, COLOR_WHITE, msg_fd(fd, 48));
#endif
		return;
	}
	if (pc_cant_act(sd)
	&& skill_id != RK_REFRESH
	&& !(skill_id == SR_GENTLETOUCH_CURE && (sd->sc.opt1 == OPT1_STONE || sd->sc.opt1 == OPT1_FREEZE || sd->sc.opt1 == OPT1_STUN))
	&& ( sd->state.storage_flag && !(tmp&INF_SELF_SKILL)) // SELF skills can be used with the storage open, issue: 8027
	)
		return;

	if (pc_issit(sd))
		return;

	if (skill->not_ok(skill_id, sd))
		return;

	if (sd->bl.id != target_id && tmp&INF_SELF_SKILL)
		target_id = sd->bl.id; // never trust the client

	if (target_id < 0 && -target_id == sd->bl.id) // for disguises [Valaris]
		target_id = sd->bl.id;

	if (sd->ud.skilltimer != INVALID_TIMER) {
		if (skill_id != SA_CASTCANCEL && skill_id != SO_SPELLFIST)
			return;
	} else if (DIFF_TICK(tick, sd->ud.canact_tick) < 0) {
		if (sd->skillitem != skill_id) {
			clif->skill_fail(sd, skill_id, USESKILL_FAIL_SKILLINTERVAL, 0, 0);
			return;
		}
	}
	
	if (sd->sc.option&OPTION_COSTUME)
		return;

	eShowDebug("UGS: 8\n");
	if (sd->sc.data[SC_BASILICA])
		return; // On basilica only caster can use Basilica again to stop it.

	sd->skillitem = sd->skillitemlv = 0;

	if (skill_id >= GD_SKILLBASE) {
		struct sd_p_data *data = pdb_search(sd, false);
		if (data && data->eBG && data->leader && map->list[sd->bl.m].flag.battleground) {
#ifdef VIRT_GUILD
			struct bg_extra_info *bg_data_t = bg_extra_create(bg->team_search(sd->bg_id), false);
			if (bg_data_t == NULL || bg_data_t->g == NULL) // How is guild not found?
				return;
			skill_lv = guild->checkskill(bg_data_t->g, skill_id);
#else
			skill_lv = guild->checkskill(sd->guild, skill_id);
#endif
			eShowDebug("UGS: Hooked\n");
			stopHook = true;
		}
	}

	eShowDebug("UGS: %d:%d:%d:%d\n",GD_EMERGENCYCALL,skill_id,skill_lv,sd->status.char_id);
	pc->delinvincibletimer(sd);
	unit->skilluse_id(&sd->bl, target_id, skill_id, skill_lv);
	if (stopHook)
		hookStop();
}

/**
 * skill->castend_nodamage_id PreHooked. To Allow Execution of Guild Skills.
 * @see skill_castend_nodamage_id
 **/
int skill_castend_guild(struct block_list **src_, struct block_list **bl_, uint16 *skill_id, uint16 *skill_lv, int64 *tick, int *flag)
{
	struct block_list *src = *src_, *bl = *bl_;
	if (src->type == BL_PC) {
		struct map_session_data *sd = BL_CAST(BL_PC, src);
		struct sd_p_data *data = pdb_search(sd, false);
		if (sd == NULL)
			return 0;
		if (data == NULL || !data->eBG || !data->leader || !map->list[sd->bl.m].flag.battleground)
			return 0;
		
		if (*skill_id == GD_EMERGENCYCALL) {
			int dx[9]={-1, 1, 0, 0,-1, 1,-1, 1, 0};
			int dy[9]={ 0, 0, 1,-1, 1,-1,-1, 1, 0};
			int i, j = 0, count = 0;
	
#ifdef VIRT_GUILD
			struct battleground_data *bgd = bg->team_search(sd->bg_id);
			struct bg_extra_info *bg_data_t = bg_extra_create(bgd, false);
			if (bg_data_t == NULL || bg_data_t->g == NULL) // No BG? Weird
				return 0;
			count = bgd->count;
#else
			struct guild *g = sd->guild;
			if (g == NULL)
				return 0;
			count = g->max_member;
#endif

			eShowDebug("Guild Emergency Called - BG");

			clif->skill_nodamage(src, bl, *skill_id, *skill_lv, 1);
			for (i = 0; i < count; i++, j++) {
				struct map_session_data *dstsd;
				if (j > 8)
					j = 0;
#ifdef VIRT_GUILD
				dstsd = bgd->members[i].sd;
#else
				dstsd = g->member[i].sd;
#endif
				if (dstsd != NULL && sd != dstsd && !dstsd->state.autotrade && !pc_isdead(dstsd)) {
					if (map->getcell(src->m,src,src->x+dx[j],src->y + dy[j],CELL_CHKNOREACH))
						dx[j] = dy[j] = 0;
					pc->setpos(dstsd, map_id2index(src->m), src->x + dx[j], src->y + dy[j], CLR_RESPAWN);
				}
			}

			guild->block_skill(sd, skill->get_time2(*skill_id,*skill_lv));
			hookStop();
		}
	}
	return 0;

}

/**
 * Warps a Player to SavePoint (used by bg_mapwarp ScriptCommand)
 * @param bl BlockList of the Player
 * @param ap va_list
 * @return 0
 **/
int bg_mapwarp_sub(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd = (struct map_session_data *)bl;
	pc->setpos(sd, sd->status.save_point.map,sd->status.save_point.x, sd->status.save_point.y, CLR_TELEPORT);
	return 0;
}

/**
 * PreHooks status_damage for mob_immunity.
 * @see status_damage
 **/
int mob_immunity(struct block_list **src_, struct block_list **target_,int64 *in_hp, int64 *in_sp, int *walkdelay, int *flag)
{
	struct block_list *target = *target_;
	if (target && target->type==BL_MOB) {
		struct mob_extra_data *data;
		data = mob_e_search((TBL_MOB*)target, false);
		if (data && data->immunity) {
			if (*in_hp > 1) {
				*in_hp = 1;
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Reveals the Position(Also Clears)
 * @return 0
 **/
int bg_reveal_pos(struct block_list *bl, va_list ap) {
	struct map_session_data *pl_sd = BL_CAST(BL_PC, bl);
	struct map_session_data *sd = va_arg(ap, struct map_session_data *); /// Player who invoked it.
	int flag = va_arg(ap, int);
	int color = va_arg(ap, int);

	if (sd == NULL || pl_sd == NULL)
		return 0;
	if (pl_sd->bg_id == sd->bg_id)
		return 0; /// Same Team

	clif->viewpoint(pl_sd, sd->bl.id, flag, sd->bl.x, sd->bl.y, sd->bl.id, color);
	return 0;
}

/**
 * Hooks bg_send_dot_remove
 * Removes the view from minimap of the team.
 * @param sd Player map_session_data
 **/
void remove_teampos(struct map_session_data *sd) {
	struct sd_p_data *sd_data = NULL;
	if (sd != NULL) {
		sd_data = pdb_search(sd, false);
		if (sd_data != NULL && sd->bg_id > 0) {
			struct battleground_data *bgd;
			if ((bgd = bg->team_search(sd->bg_id)) != NULL) {
				struct bg_extra_info *bg_data_t = bg_extra_create(bgd, false);
				int m;
				if (bg_data_t != NULL && bg_data_t->reveal_pos && (m = map->mapindex2mapid(bgd->mapindex)) == sd->bl.m) {
					map->foreachinmap(bg_reveal_pos, m, BL_PC, sd, 2, 0xFFFFFF);
				}
			}
		}
	}
}

/**
 * @see DBApply
 **/
int bg_send_pos(union DBKey key, struct DBData *data, va_list ap) {
	struct battleground_data *bgd = DB->data2ptr(data);
	struct bg_extra_info *bg_data_t;
	nullpo_ret(bgd);
	
	bg_data_t = bg_extra_create(bgd, false);
	if (bg_data_t != NULL && bg_data_t->reveal_pos) {
		int i;
		int m = map->mapindex2mapid(bgd->mapindex);
		bg_data_t->reveal_flag = ((bg_data_t->reveal_flag) ? false : true);
		if (!bg_data_t->reveal_flag)
			return 0;
#ifdef VIRT_GUILD
		if (bg_data_t->g == NULL)
			return 0;
#endif
		for (i = 0; i < MAX_BG_MEMBERS; i++) {
			struct map_session_data *sd;
#ifndef VIRT_GUILD
			struct sd_p_data* sd_data;
#endif
			if ((sd = bgd->members[i].sd) == NULL) {
				continue;
			}
#ifndef VIRT_GUILD
			sd_data = pdb_search(sd, false);
			if (sd_data == NULL || sd_data->g == NULL) {
				continue;
			}
#endif
			if (bg_data_t->reveal_flag && sd->bl.m == m) {
#ifdef VIRT_GUILD
				map->foreachinmap(bg_reveal_pos, m, BL_PC, sd, 1, bg_colors[GET_ORIG_GUILD_ID(bg_data_t->g->guild_id)]);
#else
				map->foreachinmap(bg_reveal_pos, m, BL_PC, sd, 1, bg_colors[GET_ORIG_GUILD_ID(sd_data->g->guild_id)]);
#endif
			}
		}
	}
	return 0;
}

/**
 * PostHook of bg->send_xy_timer
 * @see bg_send_xy_timer
 **/
int send_pos_location(int retVal, int tid, int64 tick, int id, intptr_t data) {
	if (bg->team_db)
		bg->team_db->foreach(bg->team_db, bg_send_pos, tick);
	return 0;
}

/**
 * PreHook of npc->parse_unknown_mapflag
 * @see npc_parse_unknown_mapflag
 **/
void parse_mapflags(const char **name, const char **w3, const char **w4, const char **start, const char **buffer, const char **filepath, int **retval)
{
	int16 m = map->mapname2mapid(*name);
	struct ebg_mapflags *mf_data;
	bool init = false;
	if (!strcmpi(*w3, "bg_topscore")) {
		CREATE_MAPD(bg_topscore, atoi(*w4));
	} else if (!strcmpi(*w3, "bg_3")) {
		CREATE_MAPD(bg_3, 1);
	} else if (!strcmpi(*w3,"noemergencycall")) {
		CREATE_MAPD(no_ec, 1);
	}
	if (init)
		hookStop();
	
	return;
}

/**
 * PreHook of clif->bg_updatescore_single
 * @see clif_bg_updatescore_single
 **/
void bg_updatescore_mf(struct map_session_data **sd_)
{
	struct map_session_data *sd = *sd_;
	struct bg_extra_info* bg_extra;
	struct ebg_mapflags *mf_data;
	int fd;
	struct sd_p_data *sd_data;
	
	nullpo_retv(sd);

	fd = sd->fd;
	
	sd_data = pdb_search(sd, false);
	if (sd_data == NULL || !sd->bg_id)
		return;
		
	bg_extra = bg_extra_create(bg->team_search(sd->bg_id), false);
	mf_data = getFromMAPD(&map->list[sd->bl.m], 0);
	if (mf_data != NULL && mf_data->bg_3) {
		WFIFOHEAD(fd, (clif->packet(0x2de))->len);
		WFIFOW(fd, 0) = 0x2de;
		if (bg_extra != NULL) {
			WFIFOW(fd, 2) = bg_extra->points;
		} else {
			WFIFOW(fd, 2) = 0;
		}
		WFIFOL(fd, 4) = mf_data->bg_topscore;
		WFIFOSET(fd, (clif->packet(0x2de))->len);
		hookStop();
		return;
	}
}

/**
 * Updates TeamScore
 **/
void bg_updatescore_team(struct battleground_data *bgd) {
	int len = (clif->packet(0x2de))->len;
	char *buf;
	struct bg_extra_info* bg_extra;
	struct ebg_mapflags *mf_data;
	struct map_session_data *sd;
	struct sd_p_data *sd_data;
	int i, m;
	bool init = false;

	nullpo_retv(bgd);

	if (!(m = map->mapindex2mapid(bgd->mapindex)))
		return;

	buf = (char*) malloc(len);
	
	for (i = 0; i < MAX_BG_MEMBERS; i++) {
		if ((sd = bgd->members[i].sd) == NULL || sd->bl.m != m)
			continue;
		sd_data = pdb_search(sd, false);
		if (sd_data == NULL || sd->bg_id == 0)
			continue;
		
		if (!init) {	// If Already init, buf might have the values.
			bg_extra = bg_extra_create(bg->team_search(sd->bg_id), false);
			mf_data = getFromMAPD(&map->list[sd->bl.m], 0);
		
			if (mf_data != NULL && mf_data->bg_topscore > 0) {
				WBUFW(buf, 0) = 0x2de;
				if (bg_extra != NULL) {
					WBUFW(buf, 2) = bg_extra->points;
				} else {
					WBUFW(buf, 2) = 0;
				}
				WBUFW(buf, 4) = mf_data->bg_topscore;
				init = true;
			}
		} else {
			continue;
		}

		clif->send(buf, (clif->packet(0x2de))->len, &sd->bl, SELF);
	}
	free(buf);
	return;
}

#ifdef VIRT_GUILD

/**
 * Checks if sd_p_data of player and Battleground ID exists.
 * if it exists, it saves eBG Data into variable bg_data_t(to be defined on function)
 * else it return
 * @param sd map_session_data
 * @param data sd_p_data
 * @return 
 **/
#define CHECK_DATA_BG(sd, data) \
	eShowDebug("Entering CHECK_DATA_BG: %u\n", (sd)->bg_id);	\
	if ((sd) && (sd)->bg_id) {	\
		eShowDebug("BG_Check 2: %u\n", (sd)->bg_id);	\
		bg_data_t = bg_extra_create(bg->team_search((sd)->bg_id), false);	\
		if (bg_data_t == NULL) {	\
			eShowDebug("BG Data Null.\n");	\
			return;	\
		} \
		if (bg_data_t->g == NULL) { \
			eShowDebug("BG Guild Data Null. \n"); \
			return;	\
		} \
		eShowDebug("BG Data Found %s. \n",bg_data_t->g->name);	\
	} else {	\
		eShowDebug("No BG ID Found.\n");	\
		return;	\
	}
/**
 * PreHook of status->get_guild_id
 * Returns Virtual GuildID
 * @see status_get_guild_id
 **/
int status_virt_guild_id(const struct block_list **bl_) {
	const struct block_list *bl = *bl_;
	nullpo_ret(bl);
	if (bl->type == BL_PC) {
		struct bg_extra_info* bg_data_t = NULL;
		struct sd_p_data *sd_data;
		const struct map_session_data *sd = BL_CCAST(BL_PC, bl);
		sd_data = getFromMSD(sd, 0);
		eShowDebug("virt_guild_id: %d:%u\n", (sd_data && sd_data->eBG)? 1: 0, sd->bg_id);
		if (sd_data && sd_data->eBG && sd->bg_id)
			 bg_data_t = bg_extra_create(bg->team_search(sd->bg_id), false);
		else
			return 0;
		if (bg_data_t && bg_data_t->g) {
			hookStop();
			return bg_data_t->g->guild_id;
		}
	}
	return 0;
}

/**
 * PreHook of status->get_emblem_id
 * Returns Virtual EmblemID
 * @see status_get_emblem_id
 **/
int status_virt_emblem_id(const struct block_list **bl_) {
	const struct block_list *bl = *bl_;
	nullpo_ret(bl);
	if (bl->type == BL_PC) {
		struct bg_extra_info* bg_data_t;
		struct sd_p_data *sd_data;
		const struct map_session_data *sd = BL_CCAST(BL_PC, bl);
		sd_data = getFromMSD(sd, 0);
		eShowDebug("virt_guild_id: %d:%u\n", (sd_data && sd_data->eBG)? 1: 0, sd->bg_id);
		if (sd_data && sd_data->eBG && sd->bg_id)
			 bg_data_t = bg_extra_create(bg->team_search(sd->bg_id), false);
		else
			return 0;
		if (bg_data_t && bg_data_t->g) {
			hookStop();
			return bg_data_t->g->emblem_id;
		}
	}
	return 0;
}

/**
 * PreHook of clif->charnameupdate
 * Resolves the Name
 * @see clif_charnameupdate
 **/
void clif_charname_virt(struct map_session_data **ssd_) {
	int cmd = 0x195, ps = -1;
	int len = (clif->packet(cmd))->len + 1;
	unsigned char *buf;
	struct party_data *p = NULL;
	struct guild *g = NULL;
	struct bg_extra_info *bg_data_t;
	struct sd_p_data* data;
	struct map_session_data *ssd = *ssd_;

	nullpo_retv(ssd);
	data = pdb_search(ssd, false);
	checkNull_void(data);
	CHECK_DATA_BG(ssd, data)
	if (bg_data_t->g == NULL)
		return;

	if (ssd->fakename[0]) {
		hookStop();
		return; //No need to update as the party/guild was not displayed anyway.
	}

	buf = (unsigned char*)malloc(len);

	WBUFW(buf,0) = cmd;
	WBUFL(buf,2) = ssd->bl.id;

	memcpy(WBUFP(buf,6), ssd->status.name, NAME_LENGTH);

	if (!battle->bc->display_party_name) {
		if (ssd->status.party_id > 0 && ssd->status.guild_id > 0 && (g = ssd->guild) != NULL)
			p = party->search(ssd->status.party_id);
	} else {
		if (ssd->status.party_id > 0)
			p = party->search(ssd->status.party_id);
	}

	if (bg_data_t->g) {
		g = bg_data_t->g;
		ps = 1;
		if (data && data->leader)
			ps = 0;
	}

	if (p)
		memcpy(WBUFP(buf,30), p->party.name, NAME_LENGTH);
	else
		WBUFB(buf,30) = 0;

	if (g && ps >= 0 && ps < MAX_GUILDPOSITION) {
		eShowDebug("Name/Position: %s,%s\n",g->name, g->position[ps].name);
		memcpy(WBUFP(buf,54), g->name,NAME_LENGTH);
		memcpy(WBUFP(buf,78), g->position[ps].name, NAME_LENGTH);
	} else {
		eShowDebug("Guild Name Not Send(%d:%d)..\n",ps,MAX_GUILDPOSITION);
		WBUFB(buf,54) = 0;
		WBUFB(buf,78) = 0;
	}

	// Update nearby clients
	clif->send(buf, (clif->packet(cmd))->len, &ssd->bl, AREA);
	free(buf);
	hookStop();
	return;
}

/**
 * PreHook of clif->charnameack
 * Resolves the Name
 * @see clif_charnameack
 **/
void clif_charname_virt2(int *fd, struct block_list **bl_)
{
	unsigned char buf[103];
	int cmd = 0x95;
	struct block_list *bl = *bl_;

	nullpo_retv(bl);

	WBUFW(buf,0) = cmd;
	WBUFL(buf,2) = bl->id;

	if (bl->type != BL_PC)
		return;
	else {
		struct map_session_data *ssd = (struct map_session_data *)bl;
		struct party_data *p = NULL;
		struct guild *g = NULL;
		struct sd_p_data *data = pdb_search(ssd, false);
		struct bg_extra_info *bg_data_t;
		int ps = 1;
		
		
		/// Requesting your own "shadow" name. [Skotlex]
		if (ssd->fd == *fd && ssd->disguise != -1)
			WBUFL(buf,2) = -bl->id;

		if (ssd->fakename[0])
			return;
		
		memcpy(WBUFP(buf,6), ssd->status.name, NAME_LENGTH);
		
		data = pdb_search(ssd, false);
		checkNull_void(data);
		CHECK_DATA_BG(ssd, data)
		if (bg_data_t->g == NULL)
			return;

		if (ssd->status.party_id)
			p = party->search(ssd->status.party_id);

		g = bg_data_t->g;
		ps = 1; // Position
		if (data && data->leader)
			ps = 0;
			

		if (!battle->bc->display_party_name && g == NULL) {// do not display party unless the player is also in a guild
			p = NULL;
		}

		if (p == NULL && g == NULL) {
			hookStop();
			return;
		}

		WBUFW(buf, 0) = cmd = 0x195;
		if (p)
			memcpy(WBUFP(buf,30), p->party.name, NAME_LENGTH);
		else
			WBUFB(buf,30) = 0;

		if (g && ps >= 0 && ps < MAX_GUILDPOSITION) {
			memcpy(WBUFP(buf,54), g->name,NAME_LENGTH);
			memcpy(WBUFP(buf,78), g->position[ps].name, NAME_LENGTH);
		} else { //Assume no guild.
			WBUFB(buf,54) = 0;
			WBUFB(buf,78) = 0;
		}
	}
	// if no recipient specified just update nearby clients
	if (*fd == 0)
		clif->send(buf, (clif->packet(cmd))->len, bl, AREA);
	else {
		WFIFOHEAD(*fd, (clif->packet(cmd))->len);
		memcpy(WFIFOP(*fd, 0), buf, (clif->packet(cmd))->len);
		WFIFOSET(*fd, (clif->packet(cmd))->len);
	}
	hookStop();
}

/**
 * PreHook of guild->getposition
 * Used by clif_belonginfo_virt
 * @see guild_getposition
 **/
int guild_getposition_pre(struct guild **g, struct map_session_data **sd_)
{
	struct sd_p_data *sd_data;
	struct map_session_data *sd = *sd_;
	
	if (sd == NULL || *g == NULL)
		return -1;
	
	if ((*g)->guild_id >= GET_EBG_GUILD_ID(0))
		hookStop();
	sd_data = pdb_search(sd, false);
	if (sd_data && sd_data->leader)
		return 0;
	return 1;
}

/**
 * PreHook of clif->guild_belonginfo
 * Sends BG Guild Information
 * @see clif_guild_belonginfo
 **/
void clif_belonginfo_virt(struct map_session_data **sd_, struct guild **g_) {
	int ps,fd;
	struct bg_extra_info *bg_data_t = NULL;
	struct sd_p_data *data = NULL;
	struct map_session_data *sd = *sd_;
	struct guild *g = *g_;

	nullpo_retv(sd);
	nullpo_retv(g);
	
	if ((data = pdb_search(sd, false)) == NULL)
		return;
	
	CHECK_DATA_BG(sd, data)
	if (bg_data_t->g == NULL)
		return;

	g = bg_data_t->g;
	fd = sd->fd;
	ps = ((data->leader) ? 0 : 1);
	WFIFOHEAD(fd,(clif->packet(0x16c))->len);
	WFIFOW(fd,0)=0x16c;
	WFIFOL(fd,2)= g->guild_id;
	WFIFOL(fd,6)= g->emblem_id;
	WFIFOL(fd,10)= g->position[ps].mode;
	WFIFOB(fd,14)= false; // Not a Guild Master
	WFIFOL(fd,15)= 0;
	memcpy(WFIFOP(fd,19), g->name, NAME_LENGTH);
	WFIFOSET(fd,(clif->packet(0x16c))->len);
	hookStop();
}

/**
 * PreHook of clif->guild_emblem
 * Sends BG Guild Emblem
 * @see clif_guild_emblem
 **/
void clif_virt_guild_emblem(struct map_session_data **sd_, struct guild **g_)
{
	int fd;
	struct bg_extra_info *bg_data_t = NULL;
	struct sd_p_data *sd_data = NULL;
	struct map_session_data *sd = *sd_;
//	struct guild *g = *g_;
	nullpo_retv(sd);

	sd_data = pdb_search(sd, false);

	checkNull_void(sd_data);
	CHECK_DATA_BG(sd, sd_data)

	fd = sd->fd;
	if (bg_data_t->g->emblem_len <= 0) {
		hookStop();
		return;
	}

	WFIFOHEAD(fd, bg_data_t->g->emblem_len+12);
	WFIFOW(fd,0)= 0x152;
	WFIFOW(fd,2)= bg_data_t->g->emblem_len+12;
	WFIFOL(fd,4)= bg_data_t->g->guild_id;
	WFIFOL(fd,8)= bg_data_t->g->emblem_id;
	memcpy(WFIFOP(fd,12), bg_data_t->g->emblem_data, bg_data_t->g->emblem_len);
	WFIFOSET(fd, WFIFOW(fd, 2));
	hookStop();
}

/**
 * PreHook of clif->guild_positionnamelist
 * Sends BG Guild Position Name
 * @see clif_guild_positionnamelist
 **/
void clif_virt_positionname(struct map_session_data **sd) {
	int i,fd;
	struct guild *g;
	struct bg_extra_info *bg_data_t = NULL;
	struct sd_p_data *sd_data = NULL;
	
	nullpo_retv(*sd);
	sd_data = pdb_search(*sd, false);
	checkNull_void(sd_data);
	CHECK_DATA_BG(*sd, sd_data)
	g = bg_data_t->g;

	fd = (*sd)->fd;
	WFIFOHEAD(fd, MAX_GUILDPOSITION * 28 + 4);
	WFIFOW(fd, 0)=0x166;
	for (i=0;i<MAX_GUILDPOSITION;i++) {
		WFIFOL(fd,i*28+4)=i;
		memcpy(WFIFOP(fd,i*28+8),g->position[i].name,NAME_LENGTH);
	}
	WFIFOW(fd,2)=i*28+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	hookStop();
}

/**
 * PreHook of clif->guild_positioninfolist
 * Sends BG Guild Position Info
 * @see clif_guild_positioninfolist
 **/
void clif_virt_positioninfo(struct map_session_data **sd) {
	int i,fd;
	struct guild *g;
	struct bg_extra_info *bg_data_t = NULL;
	struct sd_p_data *sd_data = NULL;

	nullpo_retv(*sd);
	sd_data = pdb_search(*sd, false);
	checkNull_void(sd_data);
	CHECK_DATA_BG(*sd, sd_data)
	g = bg_data_t->g;

	fd = (*sd)->fd;
	WFIFOHEAD(fd, MAX_GUILDPOSITION * 16 + 4);
	WFIFOW(fd, 0)=0x160;
	for (i=0;i<MAX_GUILDPOSITION;i++) {
		struct guild_position *p=&g->position[i];
		WFIFOL(fd,i*16+ 4)=i;
		WFIFOL(fd,i*16+ 8)=p->mode;
		WFIFOL(fd,i*16+12)=i;
		WFIFOL(fd,i*16+16)=p->exp_mode;
	}
	WFIFOW(fd, 2)=i*16+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	hookStop();
}

/**
 * PreHook of clif->guild_allianceinfo
 * Doesn't send any alliance of BG
 * @see clif_guild_allianceinfo
 **/
void clif_virt_alliance(struct map_session_data **sd)
{
	struct sd_p_data *sd_data;
	nullpo_retv(*sd);
	sd_data = pdb_search(*sd, false);
	if (sd_data && (*sd)->bg_id) {
		struct bg_extra_info *bg_data_t = bg_extra_create(bg->team_search((*sd)->bg_id), false);
		if (bg_data_t && bg_data_t->g) {
			hookStop();
			return;
		}
	}
}

/**
 * PreHook of clif->guild_skillinfo
 * Sends the Skill info of Virtual BG Guild
 * @see clif_guild_skillinfo
 **/
void clif_virt_skill(struct map_session_data **sd)
{
	int fd;
	struct guild* g;
	int i,c;
	struct bg_extra_info *bg_data_t = NULL;
	struct sd_p_data *sd_data = NULL;

	nullpo_retv(*sd);
	
	sd_data = pdb_search(*sd, false);
	checkNull_void(sd_data);
	CHECK_DATA_BG(*sd, sd_data)
	g = bg_data_t->g;

	fd = (*sd)->fd;
	WFIFOHEAD(fd, 6 + MAX_GUILDSKILL*37);
	WFIFOW(fd,0) = 0x0162;
	WFIFOW(fd,4) = g->skill_point;
	for (i = 0, c = 0; i < MAX_GUILDSKILL; i++) {
		if (g->skill[i].id > 0 && guild->check_skill_require(g, g->skill[i].id)) {
			int id = g->skill[i].id;
			int p = 6 + c*37;
			WFIFOW(fd,p+0) = id;
			WFIFOL(fd,p+2) = skill->get_inf(id);
			WFIFOW(fd,p+6) = g->skill[i].lv;
			if (g->skill[i].lv) {
				WFIFOW(fd, p + 8) = skill->get_sp(id, g->skill[i].lv);
				WFIFOW(fd, p + 10) = skill->get_range(id, g->skill[i].lv);
			} else {
				WFIFOW(fd, p + 8) = 0;
				WFIFOW(fd, p + 10) = 0;
			}
			safestrncpy((char*)WFIFOP(fd,p+12), skill->get_name(id), NAME_LENGTH);
			WFIFOB(fd,p+36)= (g->skill[i].lv < guild->skill_get_max(id) && *sd == g->member[0].sd) ? 1 : 0;
			c++;
		}
	}
	WFIFOW(fd,2) = 6 + c*37;
	WFIFOSET(fd,WFIFOW(fd,2));
	hookStop();
}

/**
 * PreHook of guild->isallied
 * Alliance check, virt guild cannot be alliance
 * @see guild_isallied
 **/
bool guild_virt_allied(int *guild_id, int *guild_id2)
{
	int i = GET_EBG_GUILD_ID(0);
	eShowDebug("i:%d,guild_id:%d,guild_id2:%d\n", i, *guild_id, *guild_id2);
	if (*guild_id >= i || *guild_id2 >= i) {    // Virtual Guild
		hookStop();
		return false;
	}
	return true;
}

/**
 * @param guild_id GuildID
 * Returns Structure of Virtual Guild
 **/
struct guild *virt_search(int *guild_id) {
	if ((*guild_id) >= INT_MAX - TOTAL_GUILD - 1) {
		hookStop();
		return &bg_guild[*guild_id - (INT_MAX - TOTAL_GUILD - 1)];
	}
	return NULL;
}

#endif

/**
 * Similar to clif send except the target
 * @param type Our list of targets
 * @see ebg_target
 * @see clif_send
 **/
bool ebg_clif_send(const void* buf, int len, struct block_list* bl, enum ebg_target type) {
	struct map_session_data *tsd;
	struct s_mapiterator* iter;
	struct sd_p_data *data;
	
	switch(type) {
		case ANNOUNCE_EBG:
			iter = mapit_getallusers();
			while( (tsd = (struct map_session_data *)mapit->next(iter)) != NULL) {
				data = pdb_search(tsd, false);
				if (data == NULL || !data->ignore) {
					WFIFOHEAD(tsd->fd, len);
					memcpy(WFIFOP(tsd->fd,0), buf, len);
					WFIFOSET(tsd->fd,len);
				}
			}
			mapit->free(iter);
			break;
		case CLIENT_EBG: {
			struct map_session_data *sd = BL_CAST(BL_PC, bl);
			int bg_id;
			
			nullpo_ret(sd);
			data = pdb_search(sd, false);
			if (data == NULL)
				break;
			bg_id = sd->bg_id;
			iter = mapit_getallusers();

			while( (tsd = (struct map_session_data *)mapit->next(iter)) != NULL) {
				if ((data = pdb_search(tsd, false)) != NULL && data->eBG && bg_id == tsd->bg_id) {
					WFIFOHEAD(tsd->fd, len);
					memcpy(WFIFOP(tsd->fd,0), buf, len);
					WFIFOSET(tsd->fd,len);
				}
			}
			mapit->free(iter);
		}
			break;
	}
	return true;
}

/// Send broadcast message with font formatting (ZC_BROADCAST2).
/// Format: 01c3 <packet len>.W <fontColor>.L <fontType>.W <fontSize>.W <fontAlign>.W <fontY>.W <message>.?B
void ebg_broadcast2(struct block_list* bl, const char* mes, int len, unsigned int fontColor, short fontType, short fontSize, short fontAlign, short fontY, enum ebg_target target)
{
	unsigned char *buf;

	nullpo_retv(mes);

	buf = (unsigned char*)aMalloc((16 + len)*sizeof(unsigned char));
	WBUFW(buf,0)  = 0x1c3;
	WBUFW(buf,2)  = len + 16;
	WBUFL(buf,4)  = fontColor;
	WBUFW(buf,8)  = fontType;
	WBUFW(buf,10) = fontSize;
	WBUFW(buf,12) = fontAlign;
	WBUFW(buf,14) = fontY;
	memcpy(WBUFP(buf,16), mes, len);
	ebg_clif_send(buf, WBUFW(buf,2), bl, target);

	aFree(buf);
}

/**
 * Reports any AFK member if any
 * @see pc_update_idle_time
 **/
void update_ebg_idle(struct map_session_data **sd_, enum e_battle_config_idletime *type) {
	struct map_session_data *sd = *sd_;
	if (sd != NULL) {
		struct sd_p_data* data = pdb_search(sd, false);
		if (data && data->eBG) {
			char output[128];
			struct battleground_data *bgd = bg->team_search(sd->bg_id);
#ifdef VIRT_GUILD
			struct bg_extra_info* bg_data = NULL;
#endif
			if (bgd != NULL) {
#ifdef VIRT_GUILD
				bg_data = bg_extra_create(bgd, false);
			}
			if (bg_data && data->flag.ebg_afk) {
				sprintf(output, "%s : %s is no longer away...", bg_data->g->name, sd->status.name);
				clif->bg_message(bgd, sd->bg_id, bg_data->g->name, output);
#else
				sprintf(output, "%s : %s is no longer away...", data->g->name, sd->status.name);
				clif->bg_message(bgd, sd->bg_id, data->g->name, output);
#endif
			}
			sd->idletime = sockt->last_tick;	// Should not be timer->gettick()
			data->flag.ebg_afk = 0;
		}
	}
}

/**
 * Sets the Member as Afk
 * @see DBApply
 **/
int set_ebg_idle(union DBKey *key, struct DBData **data_db, va_list ap)
{
	struct battleground_data *bgd = DB->data2ptr(*data_db);
	struct bg_extra_info *bg_data;
	int i;
	nullpo_ret(bgd);

	bg_data = bg_extra_create(bgd, false);

	//nullpo_ret(bg_data);
	if (bg_data == NULL)
		return 0;

	for (i = 0; i < MAX_BG_MEMBERS; i++) {
		struct map_session_data *sd;
		struct sd_p_data *sd_data;
		unsigned int color = 0xFF00FF;
		if ((sd = bgd->members[i].sd) == NULL || (sd_data = pdb_search(sd, false)) == NULL)
			continue;
#ifdef VIRT_GUILD
		if (bg_data->g == NULL) {	// Happens if intialized with 1 Player
			ShowError("set_idle_bg: Guild is NULL.\n");
			continue;
		}
		color = bg_colors[GET_ORIG_GUILD_ID(bg_data->g->guild_id)];
#else
		if (sd_data->g == NULL) {	// Happens if intialized with 1 Player
			ShowError("set_idle_bg: Guild is NULL.\n");
			continue;
		}
		color = bg_colors[GET_ORIG_GUILD_ID(sd_data->g->guild_id)];
#endif
		// Reveal done in other function.
		if (bg_idle_announce > 0 && sd_data->eBG && sd_data->flag.ebg_afk == 0 && DIFF_TICK32(sockt->last_tick, sd->idletime) >= bg_idle_announce) { // Idle announces
			char output[200];
			if (bg_kick_idle > 0) {
				sprintf(output, "- AFK [%s] AutoKicked -", sd->status.name);
				ebg_broadcast2(&sd->bl, output, (int)strlen(output)+1, color, 0x190, 20, 0, 0, CLIENT_EBG);
				
				bg->team_leave(sd, BGTL_AFK);
				clif->message(sd->fd, "You have been kicked from Battleground because of your AFK status.");
				pc->setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, CLR_OUTSIGHT);
				continue;
			}
			sd_data->flag.ebg_afk = 1;

#ifdef VIRT_GUILD
			sprintf(output, "%s : %s seems to be away. AFK Warning - Can be kicked out with @reportafk", bg_data->g->name, sd->status.name);
#else
			sprintf(output, "%s : %s seems to be away. AFK Warning - Can be kicked out with @reportafk", sd_data->g->name, sd->status.name);
#endif
			ebg_broadcast2(&sd->bl, output, (int)strlen(output)+1, color, 0x190, 20, 0, 0, CLIENT_EBG);
		}
	}
	return 1;
}

/**
 * Checks if monster can move or not
 * @see unit_movetoxy
 **/
int mob_can_move(struct block_list **bl, short *x, short *y, int *flag)
{
	nullpo_ret(*bl);
	
	if ((*bl)->type == BL_MOB) {
		struct mob_extra_data *data = mob_e_search((TBL_MOB*)(*bl), false);
		if (data && !data->can_move) {
			hookStop();
			return 0;
		}
	}
	return 1;
}

/**
 * Checks if monster can move or not
 * @see unit_walktobl
 **/
int mob_can_move_v2(struct block_list **bl, struct block_list **tbl, int *range, int *flag)
{
	nullpo_ret(*bl);
	nullpo_ret(*tbl);
	if ((*bl)->type == BL_MOB) {
		struct mob_extra_data *data = mob_e_search((TBL_MOB*)(*bl), false);
		if (data && !data->can_move) {
			hookStop();
			return 0;
		}
	}
	return 1;
}

/**
 * Iterates each timer and shows it value to the player attached.
 * @see struct DBData
 **/
int bg_timer_db_debug(union DBKey key, struct DBData *data, va_list ap)
{
	struct bg_timerdb_struct *tdb = (struct bg_timerdb_struct *)DB->data2ptr(data);
	struct map_session_data *sd = (struct map_session_data *)va_arg(ap, struct map_session_data *);	
	
	if (sd != NULL && tdb != NULL) {
		char message[150];
		sprintf(message, "UID: %d, Operation: %d, timer_id: %d, Timer: %d, ParentUID: %d, Random Timer: %d~%d s", key.i, tdb->operations, tdb->timer_id, tdb->timer_, tdb->parent_uid, tdb->random_timer[0], tdb->random_timer[1]);
		clif->message(sd->fd, message);
		return 1;
	}
	return 0;
}

#ifdef EBG_RANKING

/**
 * Stores Max Damage/Damage
 **/
int store_damage_ranking(struct block_list *src, struct block_list *dst, int64 in_damage, int64 in_damage2)
{
	struct map_session_data *sd = BL_UCAST(BL_PC, src);
	struct sd_p_data *sd_data;
	int is_bg = MAP_IS_NONE;
	uint64 *max_damage;
	int64 total_damage;

	sd_data = pdb_search(sd, false);

	EBG_OR_WOE(sd, sd_data, is_bg);

	if (is_bg == MAP_IS_NONE)
		return 0;

	max_damage = GET_VARIABLE_SIZE(sd, RANKING_MAX_DAMAGE, true, uint64);
	total_damage = in_damage + in_damage2;

	if (total_damage > 0) {
		uint64 *damage;
		if (max_damage && *max_damage < (uint64)total_damage) {
			*max_damage = total_damage;
		}
		SET_VARIABLE_ADD(sd, damage, RANKING_DAMAGE_DEALT, total_damage, uint64);
		if (dst->type == BL_PC) {
			struct map_session_data *tsd = BL_UCAST(BL_PC, dst);
			SET_VARIABLE_ADD(tsd, damage, RANKING_DAMAGE_RECEIVED, total_damage, uint64);
		}
	}
	return 1;
}

/**
 * Stores Max Damage
 * Also Records Acid Demonstration Use
 * clif_skill_damage preHooked
 * @see clif_skill_damage
 **/
int record_max_damage(struct block_list **src, struct block_list **dst, int64 *tick, int *sdelay, int *ddelay, int64 *in_damage, int *div, uint16 *skill_id, uint16 *skill_lv, int *type)
{
	nullpo_ret(*src);
	if ((*src)->type == BL_PC) {
		if (store_damage_ranking(*src, *dst, *in_damage, 0) == 1) {
			struct map_session_data *sd = BL_UCAST(BL_PC, *src);
			if (sd != NULL) {
				if ((*skill_id) == CR_ACIDDEMONSTRATION) {
					unsigned int *ranking;
					if (*in_damage > 0) {
						SET_VARIABLE_ADD(sd, ranking, RANKING_ACID_DEMONSTRATION, 1, unsigned int);
					} else {
						SET_VARIABLE_ADD(sd, ranking, RANKING_ACID_DEMONSTRATION_F, 1, unsigned int);
					}
				}
			}
		}
	}
	return 0;
}

/**
 * Stores Max Damage
 * type: @see enum battle_dmg_type
 * clif_damage PreHooked
 * @see clif_damage
 **/
int record_max_damage2(struct block_list **src, struct block_list **dst, int *sdelay, int *ddelay, int64 *in_damage, short *div, unsigned char *type, int64 *in_damage2)
{
	nullpo_ret(*src);
	nullpo_ret(*dst);
	// ToDo: Should All Damage ranking happen if both are player?
	if (*src != *dst && (*src)->type == BL_PC && *in_damage > 0) {
		store_damage_ranking(*src, *dst, *in_damage, *in_damage2);
	}
	return 0;
}

/**
 * Stores Damage Dealt to Monsters
 * mob_damage PreHooked
 * @see mob_damage
 **/
void record_damage2(struct mob_data **md_, struct block_list **src_, int *damage)
{
	struct map_session_data *sd;
	struct sd_p_data *sd_data;
	int is_bg = MAP_IS_NONE;
	uint64 *damage_record = NULL;
	struct block_list *src = *src_;
	struct mob_data *md = *md_;
	if (md == NULL || src == NULL || src->type != BL_PC || *damage <= 0)
		return;
	
	sd = BL_UCAST(BL_PC, src);
	sd_data = pdb_search(sd, false);
	
	EBG_OR_WOE(sd, sd_data, is_bg);
	
	if (is_bg == MAP_IS_NONE)
		return;

	if (is_bg == MAP_IS_BG && md->class_ >= 2100 && md->class_ <= 2104) {
		SET_VARIABLE_ADD(sd, damage_record, RANKING_BG_BOSS_DMG, *damage, uint64);
	} else if (is_bg == MAP_IS_WOE && md->guardian_data) {
		switch(md->class_) {
			case MOBID_EMPELIUM:
				SET_VARIABLE_ADD(sd, damage_record, RANKING_WOE_EMPERIUM_DAMAGE, *damage, uint64);
				break;
			case MOBID_BARRICADE:
				SET_VARIABLE_ADD(sd, damage_record, RANKING_WOE_BARRICADE_DAMAGE, *damage, uint64);
				break;
			case MOBID_S_EMPEL_1:
			case MOBID_S_EMPEL_2:
				SET_VARIABLE_ADD(sd, damage_record, RANKING_WOE_GSTONE_DAMAGE, *damage, uint64);
				break;
			default:
				SET_VARIABLE_ADD(sd, damage_record, RANKING_WOE_GUARDIAN_DAMAGE, *damage, uint64);
				break;
		}
	}
	return;
}

/**
 * Records Ammo Used, 
 * battle_consume_ammo PreHooked
 * @see battle_consume_ammo
 **/
void record_ammo_spent(struct map_session_data **sd_, int *skill_id, int *lv)
{
	int qty=1;
	struct sd_p_data *sd_data;
	int is_bg = MAP_IS_NONE;
	struct map_session_data *sd = *sd_;
	
	nullpo_retv(sd);
	
	if (!battle->bc->arrow_decrement)
		return;
	
	sd_data = pdb_search(sd, false);
	
	EBG_OR_WOE(sd, sd_data, is_bg);

	if (is_bg == MAP_IS_NONE)
		return;

	if (*skill_id && *lv) {
		qty = skill->get_ammo_qty(*skill_id, *lv);
		if (!qty)
			qty = 1;
	}
	if (sd->equip_index[EQI_AMMO] >= 0) {
		unsigned int *ammo_used;
		SET_VARIABLE_ADD(sd, ammo_used, RANKING_AMMOS, qty, unsigned int);
	}
}


/**
 * Heals a character. If flag&1, this is forced healing (otherwise stuff like Berserk can block it)
 * Does Not Really Heal, and returns heal amount.
 * @see status_heal
 **/
int status_fake_heal(struct block_list *bl, int64 in_hp, int64 in_sp, int flag)
{
	struct status_data *st;
	struct status_change *sc;
	int hp,sp;

	st = status->get_status_data(bl);

	if (st == &status->dummy || !st->hp)
		return 0;

	/* From here onwards, we consider it a 32-type as the client does not support higher and the value doesn't get through percentage modifiers **/
	hp = (int)cap_value(in_hp,INT_MIN,INT_MAX);
	sp = (int)cap_value(in_sp,INT_MIN,INT_MAX);

	sc = status->get_sc(bl);
	if (sc && !sc->count)
		sc = NULL;

	if (hp < 0) {
		hp = 0;
	}

	if (hp) {
		if (sc && sc->data[SC_BERSERK]) {
			if (flag&1)
				flag &= ~2;
			else
				hp = 0;
		}

		if ((unsigned int)hp > st->max_hp - st->hp)
			hp = st->max_hp - st->hp;
	}

	if (sp < 0) {
		sp = 0;
	}

	if (sp) {
		if ((unsigned int)sp > (st->max_sp - st->sp))
			sp = st->max_sp - st->sp;
	}

	if (!sp && !hp)
		return 0;

	return (int)(hp+sp);
}

/**
 * Calculate Heal Done to TeamMate and Enemy for Rankings
 * skill_calc_heal PostHooked
 * @see skill_calc_heal
 **/
int record_healing(int retVal, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, bool heal)
{
	struct map_session_data *sd;
	struct map_session_data *tsd;
	struct status_change *tsc;
	int heal_capacity = 0;
	int64 heal_amount = retVal;
	if (!heal || retVal <= 0)
		return retVal;

	if (src == NULL || target == NULL || src->type != BL_PC || target->type != BL_PC || src == target || status->isimmune(target))
		return retVal;
	
	sd = BL_UCAST(BL_PC, src);
	tsd = BL_UCAST(BL_PC, target);

	switch(skill_id) {
		case AB_HIGHNESSHEAL:
			heal_amount = heal_amount * (17 + 3 * (skill_lv)) / 10;
		case AL_HEAL:
		case HLIF_HEAL:
			break;
		default:
			return retVal;
	}
	tsc = status->get_sc(target);
	
	if (sd && tsd && sd->status.partner_id == tsd->status.char_id && (sd->job&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && sd->status.sex == 0)
		heal_amount = heal_amount * 2;
	
	if (tsc && tsc->data[SC_AKAITSUKI] && heal_amount && skill_id != HLIF_HEAL)
		heal_amount = ~heal_amount + 1;

	heal_capacity = status_fake_heal(target, heal_amount, 0, 0);
	
	if (heal_capacity > 0) {
		int is_bg = MAP_IS_NONE;
		struct sd_p_data *sd_data = pdb_search(sd, false);
		uint64 *heal_ptr;

		EBG_OR_WOE(sd, sd_data, is_bg);

		switch(is_bg) {
			case MAP_IS_NONE:
				return retVal;
			case MAP_IS_WOE:
				if (sd->status.guild_id == tsd->status.guild_id || 
					(/* ToDo: !gvg_noally MapFlag && **/ guild->isallied(sd->status.guild_id, tsd->status.guild_id))
					) {
					SET_VARIABLE_ADD(sd, heal_ptr, RANKING_HEAL_TEAM, heal_capacity, uint64);
				} else {
					SET_VARIABLE_ADD(sd, heal_ptr, RANKING_HEAL_ENEMY, heal_capacity, uint64);
				}
				break;
			case MAP_IS_BG:
				if (sd->bg_id == tsd->bg_id) {
					SET_VARIABLE_ADD(sd, heal_ptr, RANKING_HEAL_TEAM, heal_capacity, uint64);
				} else {
					SET_VARIABLE_ADD(sd, heal_ptr, RANKING_HEAL_ENEMY, heal_capacity, uint64);
				}
		}
	}
	return retVal;
}

/**
 * Calculate Rankings for All Types of Requirements
 * type&2: consume items (after skill was used)
 * type&1: consume the others (before skill was used)
 * skill_consume_requirement PreHooked
 * @see skill_consume_requirement
 **/
int record_requirement(struct map_session_data **sd_, uint16 *skill_id, uint16 *skill_lv, short *type)
{
	struct map_session_data *sd = *sd_;
	struct skill_condition req;
	int is_bg = MAP_IS_NONE;
	struct sd_p_data* sd_data;

	nullpo_ret(sd);
	
	sd_data = pdb_search(sd, false);
	
	EBG_OR_WOE(sd, sd_data, is_bg);
	if (is_bg == MAP_IS_NONE)
		return 0;

	req = skill->get_requirement(sd, *skill_id, *skill_lv);
	if ((*type)&1) {
		unsigned int *ranking;
		switch( *skill_id ) {
			case CG_TAROTCARD:
			case MC_IDENTIFY:
				req.sp = 0;
				break;
			default:
				if (sd->state.autocast)
					req.sp = 0;
				break;
		}
		if (req.sp > 0) {
			SET_VARIABLE_ADD(sd, ranking, RANKING_SP_USED, req.sp, unsigned int);
		}
		if (req.spiritball > 0) {
			SET_VARIABLE_ADD(sd, ranking, RANKING_SPIRITB, req.spiritball, unsigned int);
		}
		if (req.zeny > 0 && (*skill_id) != NJ_ZENYNAGE) {
			if (sd->status.zeny < req.zeny)
				req.zeny = sd->status.zeny;
			SET_VARIABLE_ADD(sd, ranking, RANKING_ZENY, req.zeny, unsigned int);
		}
	}
	return 1;
}

/**
 * Calculate Rankings for Item Requirements
 * pc_delitem PreHooked
 * @see pc_delitem
 **/
int record_requirement_item(struct map_session_data **sd_, int *n, int *amount, int *type, short *reason, e_log_pick_type *log_type)
{
	struct map_session_data *sd = *sd_;
	int is_bg = MAP_IS_NONE;
	struct sd_p_data* sd_data;
	unsigned int *ranking;
	nullpo_retr(1, sd);
	
	if ((*reason) != DELITEM_SKILLUSE)
		return 1;

	if (sd->status.inventory[*n].nameid == 0 || *amount <= 0 || sd->status.inventory[*n].amount < *amount || sd->inventory_data[*n] == NULL)
		return 1;
	
	sd_data = pdb_search(sd, false);
	
	EBG_OR_WOE(sd, sd_data, is_bg);
	if (is_bg == MAP_IS_NONE)
		return 1;
	
	switch (sd->status.inventory[*n].nameid) {
		case ITEMID_POISON_BOTTLE:
			SET_VARIABLE_ADD(sd, ranking, RANKING_POISON_BOTTLE, *amount, unsigned int);
			break;
		case ITEMID_YELLOW_GEMSTONE:
			SET_VARIABLE_ADD(sd, ranking, RANKING_YELLOW_GEMSTONE, *amount, unsigned int);
			break;
		case ITEMID_RED_GEMSTONE:
			SET_VARIABLE_ADD(sd, ranking, RANKING_RED_GEMSTONE, *amount, unsigned int);
			break;
		case ITEMID_BLUE_GEMSTONE:
			SET_VARIABLE_ADD(sd, ranking, RANKING_BLUE_GEMSTONE, *amount, unsigned int);
			break;
	}
	return 0;
}

/**
 * Calculate HP/SP Potions Used
 * pc_itemheal PreHooked
 * @see pc_itemheal
 **/
int record_potion_used(struct map_session_data **sd_, int *itemid, int *hp, int *sp)
{
	struct map_session_data *sd = *sd_;
	int is_bg = MAP_IS_NONE;
	struct sd_p_data* sd_data;
	unsigned int *ranking;
	
	if (*hp <= 0 && *sp <= 0)
		return 0;
	
	if (sd == NULL)
		return 0;
	
	sd_data = pdb_search(sd, false);
	
	EBG_OR_WOE(sd, sd_data, is_bg);
	if (is_bg == MAP_IS_NONE)
		return 0;
	
	if (*hp > 0) {
		SET_VARIABLE_ADD(sd, ranking, RANKING_HP_POTION, 1, unsigned int);
	}
	if (*sp > 0) {
		SET_VARIABLE_ADD(sd, ranking, RANKING_SP_POTION, 1, unsigned int);
	}
	return 1;
}

/**
 * Records Support Skill Used
 * skill_castend_nodamage_id PostHooked
 * @see skill_castend_nodamage_id
 **/
int record_support_skill(int retVal, struct block_list *src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, int64 tick, int flag)
{
	struct map_session_data *sd, *dstsd;
	struct status_data *tstatus;
	struct sd_p_data *sd_data;
	int is_bg = MAP_IS_NONE;
	
	if (retVal != 0)
		return retVal;

	if (skill_id > 0 && !skill_lv)
		return retVal;

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if (src->m != bl->m)
		return retVal;

	sd = BL_CAST(BL_PC, src);
	if (sd == NULL)
		return retVal;

	sd_data = pdb_search(sd, false);
	EBG_OR_WOE(sd, sd_data, is_bg);
	
	if (is_bg == MAP_IS_NONE)
		return retVal;

	dstsd = BL_CAST(BL_PC, bl);

	// Supportive skills that can't be cast in users with mado
	if (sd && dstsd && pc_ismadogear(dstsd)) {
		switch( skill_id ) {
			case AL_HEAL:
			case AL_INCAGI:
			case AL_DECAGI:
			case AB_RENOVATIO:
			case AB_HIGHNESSHEAL:
				return retVal;
			default:
				break;
		}
	}

	tstatus = status->get_status_data(bl);

	//Check for undead skills that convert a no-damage skill into a damage one. [Skotlex]
	switch (skill_id) {
		case AL_HEAL:
		/**
		 * Arch Bishop
		 **/
		case AB_RENOVATIO:
		case AB_HIGHNESSHEAL:
		case AL_INCAGI:
		case ALL_RESURRECTION:
		case PR_ASPERSIO:
			//Apparently only player casted skills can be offensive like this.
			if (sd && battle->check_undead(tstatus->race,tstatus->def_ele) && skill_id != AL_INCAGI) {
				return retVal;
			}
			break;
		case SO_ELEMENTAL_SHIELD:
		case NPC_SMOKING:
		case MH_STEINWAND:
			return 0;
		case RK_MILLENNIUMSHIELD:
		case RK_CRUSHSTRIKE:
		case RK_REFRESH:
		case RK_GIANTGROWTH:
		case RK_STONEHARDSKIN:
		case RK_VITALITYACTIVATION:
		case RK_STORMBLAST:
		case RK_FIGHTINGSPIRIT:
		case RK_ABUNDANCE:
			if (sd && !pc->checkskill(sd, RK_RUNEMASTERY)) {
				return retVal;
			}
			break;
		default:
			if (skill->castend_nodamage_id_undead_unknown(src, bl, &skill_id, &skill_lv, &tick, &flag)) {
				//Skill is actually ground placed.
				if (src == bl && skill->get_unit_id(skill_id,0))
					return retVal;
			}
			break;
	}

	if (skill->get_inf(skill_id)&INF_SUPPORT_SKILL && sd && dstsd && sd != dstsd) {
		unsigned int *skill_ranking;
		if (is_bg == MAP_IS_WOE) {
			if ( sd->status.guild_id == dstsd->status.guild_id ||
				(/* ToDo: !gvg_noally MapFlag && **/ guild->isallied(sd->status.guild_id, dstsd->status.guild_id))
				) {
					SET_VARIABLE_ADD(sd, skill_ranking, RANKING_SUPPORT_SKILL_TEAM, 1, unsigned int);
				} else {
					SET_VARIABLE_ADD(sd, skill_ranking, RANKING_SUPPORT_SKILL_ENEMY, 1, unsigned int);
				}
		} else if (is_bg == MAP_IS_BG && dstsd->bg_id) {
			if (sd->bg_id == dstsd->bg_id) {
				SET_VARIABLE_ADD(sd, skill_ranking, RANKING_SUPPORT_SKILL_TEAM, 1, unsigned int);
			} else {
				SET_VARIABLE_ADD(sd, skill_ranking, RANKING_SUPPORT_SKILL_ENEMY, 1, unsigned int);
			}
		}
	}
	return retVal;
}


/**
 * Notifies clients in area, that an object is about to use a skill.
 * Sets SkillUsed Counter
 * @see clif_useskill
 **/
void record_skill(struct block_list **bl, int *src_id, int *dst_id, int *dst_x, int *dst_y, uint16 *skill_id, uint16 *skill_lv, int *casttime)
{
	if (*bl != NULL && (*bl)->type == BL_PC) {
		struct map_session_data *sd = BL_UCAST(BL_PC, *bl);
		if (sd) {
			uint16 *skill_used;
			SET_VARIABLE_REPLACE_(skill_used, RANKING_SKILL_USED, *skill_id, uint16);
		}
	}
}

/**
 * Attack request
 * If type is an ongoing attack
 * Reset's Skill Used variable
 * unit_atttack preHooked.
 * @param src Attacker
 * @param target_id Target
 * @param continuos Attack once or non-stop
 * @see unit_attack
 **/
int record_bare_attack(struct block_list **src, int *target_id, int *continuous) {
	if (*src != NULL && (*src)->type == BL_PC) {
		struct map_session_data *sd = BL_UCAST(BL_PC, *src);
		if (sd) {
			uint16 *skill_used;
			SET_VARIABLE_REPLACE_(skill_used, RANKING_SKILL_USED, 0, uint16);
		}
	}
	return 0;
}
#endif

/**
 * Updates Regular and Ranked BG Points/Rankings.
 * @param fd
 * @refer bg_fame_receive
 **/
void update_bg_fame(int *fd)
{
	bg_fame_receive();
}

/**
 * Clears the Hp/Sp/Extra Reduction Timer.
 * @param sd Player Data
 * @param type ebg_hpsp_timer enum
 * @see ebg_hpsp_timer
 **/
bool ebg_clear_hpsp_timer(struct map_session_data *sd, int type)
{
	struct sd_p_data *data;
	int *bg_hp_tid,*bg_sp_tid;
	int *bg_time;
	int *bg_value;
	
	if (sd == NULL) {
		ShowWarning("ebg_clear_hpsp_timer: Player Not Attached.\n");
		return false;
	}
	
	data = pdb_search(sd, false);
	
	bg_hp_tid = GET_VARIABLE_SIZE(sd, BG_HP_TID, false, int);
	bg_sp_tid = GET_VARIABLE_SIZE(sd, BG_SP_TID, false, int);
	if (type&EBG_HP_TIME && bg_hp_tid != NULL && *bg_hp_tid != INVALID_TIMER){
		bg_time = GET_VARIABLE_SIZE(sd, BG_HP_RATE, false, int);
		bg_value = GET_VARIABLE_SIZE(sd, BG_HP_VALUE, false, int);
		timer->delete(*bg_hp_tid, bg_hp_loss_function);
		*bg_hp_tid = INVALID_TIMER;
		*bg_time = 0;
		*bg_value = 0;
		
	}
	if (type&EBG_SP_TIME && bg_sp_tid != NULL && *bg_sp_tid != INVALID_TIMER){
		bg_time = GET_VARIABLE_SIZE(sd, BG_SP_RATE, false, int);
		bg_value = GET_VARIABLE_SIZE(sd, BG_SP_VALUE, false, int);
		timer->delete(*bg_sp_tid, bg_sp_loss_function);
		*bg_sp_tid = INVALID_TIMER;
		*bg_time = 0;
		*bg_value = 0;
	}
	if (type&EBG_EXTRA_TIME) {
		if (data != NULL){
			if (data->save.extra != NULL) {
				if (data->save.extra->npcs != NULL) {
					aFree(data->save.extra->npcs);
					data->save.extra->npcs = NULL;
				}
				aFree(data->save.extra);
				data->save.extra = NULL;
			}
		}
	}
	return true;
}

/**
 * bg_team_delete PreHooked
 * Removes all Player from eBG and
 * clears all entry related to eBG
 * @param bg_id BattlegrounID
 * @param type ebg_hpsp_timer enum
 * @see bg_team_delete
 **/
bool bg_team_delete_pre(int *bg_id_)
{
	int i;
	int bg_id = *bg_id_;
	struct battleground_data *bgd = bg->team_search(bg_id);
	struct bg_extra_info* bg_data_t;

	bool hookS = false;	// hookStop?
	if (bgd == NULL)
		return false;
	for (i = 0; i < MAX_BG_MEMBERS; i++) {
		struct map_session_data *sd = bgd->members[i].sd;
		struct sd_p_data *data;
		if (sd == NULL)
			continue;
		data = pdb_search(sd, false);
		if (data != NULL && data->eBG) {
			// @Todo: Just call bg->team_leave?
			int bg_id_var = pc->readreg(sd, script->add_str("@BG_ID"));
			int bg_team = pc->readreg(sd, script->add_str("@BG_Team"));
			hookS = true;
			ebg_clear_hpsp_timer(sd, EBG_HP_TIME | EBG_SP_TIME | EBG_EXTRA_TIME);
			bg->send_dot_remove(sd);
			if (bg_id_var == BGT_TOUCHDOWN) {
				pc->changelook(sd, LOOK_CLOTHES_COLOR, pc_readglobalreg(sd, script->add_str("eBG_tod_pallete")));
			}
			if (bg_team > 0) {
				eBG_script_bg_end(sd, bg_id_var);	// Save the Variables, and TurnOff the bg
				bg->team_leave(sd, BGTL_LEFT);	// Leave the Team and Guild
			}
		} else {
			continue;
		}
		bg->send_dot_remove(sd);
		clear_bg_guild_data(sd, bgd);
	}

	bg_data_t = getFromBGDATA(bgd, 0);
	if (bg_data_t != NULL) {
#ifndef LEADER_INT
		if (bg_data_t->leader != NULL) {
			aFree(bg_data_t->leader);
			bg_data_t->leader = NULL;
		}
#endif
		removeFromBGDATA(bgd, 0);
	}
	if (hookS) {
		hookStop();
	}
	idb_remove(bg->team_db, bg_id);
	return true;
}

/**
 * @see DBApply
 */
int bg_team_db_final_pre(union DBKey *key, struct DBData **data, va_list ap)
{
	struct battleground_data *bgd = DB->data2ptr(*data);
	struct bg_extra_info *bg_data_t = getFromBGDATA(bgd, 0);

	if (bg_data_t != NULL) {
#ifndef LEADER_INT
		if (bg_data_t->leader != NULL) {
			aFree(bg_data_t->leader);
			bg_data_t->leader = NULL;
		}
#endif
		removeFromBGDATA(bgd, 0);
	}

	return 0;
}

// PACKET_CM_FAME_DATA
void ebg_char_p_fame_data(int fd) {
	int num, size = RFIFOL(fd, 2);
	int total = 0, len = 8;

	int type = RFIFOW(fd, 6);

	if (type == 1) {
		memset(bgrank_fame_list, 0, sizeof(bgrank_fame_list));
		for (num = 0; len < size && num < fame_list_size_bgrank; num++) {
			memcpy(&bgrank_fame_list[num], RFIFOP(fd, len), sizeof(struct fame_list));
			len += sizeof(struct fame_list);
			total += 1;
		}
		ShowInfo("Received Fame List of '"CL_WHITE"%d"CL_RESET"' characters(BG Rank List).\n", total);
	} else if (type == 2) {
		memset(bg_fame_list, 0, sizeof(bg_fame_list));
		for (num = 0; len < size && num < fame_list_size_bg; num++) {
			memcpy(&bg_fame_list[num], RFIFOP(fd, len), sizeof(struct fame_list));
			len += sizeof(struct fame_list);
			total += 1;
		}
		ShowInfo("Received Fame List of '"CL_WHITE"%d"CL_RESET"' characters(BG Regular List).\n", total);
	}
}

/// PACKET_CM_SYNC_DATA
void ebg_char_p_sync_data(int fd) {
	int account_id, char_id;
	int64 type;
//	int offset = 0;
	struct map_session_data *sd;
	struct sd_p_data *sd_data;

	int length = RFIFOL(fd, 2);
	account_id = RFIFOL(fd, 6);
	char_id = RFIFOL(fd, 10);
	type = RFIFOQ(fd, 14);

	if (length == 22 || type == 0) {
		ShowError("ebg_char_p_sync_data: Packet Received with no Data.\n");
		return;
	}

	sd = map->id2sd(account_id);

	if (sd == NULL) {
		sd = sockt->session_is_valid(fd) ? sockt->session[fd]->session_data : NULL;
		if (sd == NULL) {
			ShowError("ebg_char_p_sync_data: Failed to Syncronize to map-server, Character went offline before data arrived(A:%d, C:%d).\n", account_id, char_id);
			return;
		}
	}

	sd_data = pdb_search(sd, true);
	if (sd_data->esd == NULL) {
		CREATE(sd_data->esd, struct ebg_save_data, 1);
	}
#ifdef CHAR_SERVER_SAVE
	bg_load_char_data_sub(sd, NULL, sd_data->esd, 22, fd, false, type);
#else
	ShowError("ebg_char_p_sync_data: Called but CHAR_SERVER_SAVE isn't defined.\n");
#endif
}

/**
 * Gets Nick(Name) of required character
 * @method map_reqnickdb_pre
 * @param  sd                player's map_session_data
 * @param  char_id           character id
 */
void map_reqnickdb_pre(struct map_session_data **sd, int *char_id)
{
	if (*sd == NULL)
		return;

	if (bg_reserved_char_id && bg_reserved_char_id == *char_id) {
		clif->solved_charname((*sd)->fd, *char_id, "Battleground");
		hookStop();
	} else if (woe_reserved_char_id && woe_reserved_char_id == *char_id) {
		clif->solved_charname((*sd)->fd, *char_id, "WoE");
		hookStop();
	}
	return;
}

/**
 * Gets Called when char-server is connected.
 * @method chrif_on_ready_post
 */
void chrif_on_ready_post(void)
{
	// Get the Fame Data.
	bg_fame_receive();      // Builds FameData.
}

void bg_initialize_constants(void)
{
	/// Common Constants 
	script->set_constant("BGT_COMMON",  BGT_COMMON, false, false);
	script->set_constant("BGT_CTF", BGT_CTF, false, false);
	script->set_constant("BGT_BOSS", BGT_BOSS, false, false);
	script->set_constant("BGT_TI", BGT_TI, false, false);
	script->set_constant("BGT_EOS", BGT_EOS, false, false);
	script->set_constant("BGT_TD", BGT_TD, false, false);
	script->set_constant("BGT_SC", BGT_SC, false, false);
	script->set_constant("BGT_CONQ", BGT_CONQ, false, false);
	script->set_constant("BGT_RUSH", BGT_RUSH, false, false);
	script->set_constant("BGT_DOM", BGT_DOM, false, false);
	script->set_constant("BGT_TOUCHDOWN", BGT_TOUCHDOWN, false, false);
	script->set_constant("BGT_MAX", BGT_MAX, false, false);
	/// BG Constants 
	script->set_constant("WALK_ONLY", WALK_ONLY, false, false);
	script->set_constant("BG_TEAM", BG_TEAM, false, false);
	script->set_constant("BG_SAVE_TID", BG_SAVE_TID, false, false);
	/// Capture the Flag 
	script->set_constant("BG_CTF_WON", BG_CTF_WON, false, false);
	script->set_constant("BG_CTF_LOSS", BG_CTF_LOSS, false, false);
	script->set_constant("BG_CTF_TIE", BG_CTF_TIE, false, false);
	script->set_constant("CTF_ONHAND", CTF_ONHAND, false, false);
	script->set_constant("CTF_DROPPED", CTF_DROPPED, false, false);
	script->set_constant("CTF_CAPTURED", CTF_CAPTURED, false, false);
	/// Bossnia 
	script->set_constant("BG_BOSS_WON", BG_BOSS_WON, false, false);
	script->set_constant("BG_BOSS_LOSS", BG_BOSS_LOSS, false, false);
	script->set_constant("BG_BOSS_TIE", BG_BOSS_TIE, false, false);
	script->set_constant("BOSS_KILLED", BOSS_KILLED, false, false);
	script->set_constant("BOSS_NEUT_FLAG", BOSS_NEUT_FLAG, false, false);
	/// Tierra Infierno 
	script->set_constant("BG_TI_WON", BG_TI_WON, false, false);
	script->set_constant("BG_TI_LOSS", BG_TI_LOSS, false, false);
	script->set_constant("BG_TI_TIE", BG_TI_TIE, false, false);
	script->set_constant("BG_TI_SKULL", BG_TI_SKULL, false, false);
	/// EyeOfStorm 
	script->set_constant("BG_EOS_WON", BG_EOS_WON, false, false);
	script->set_constant("BG_EOS_LOSS", BG_EOS_LOSS, false, false);
	script->set_constant("BG_EOS_TIE", BG_EOS_TIE, false, false);
	script->set_constant("BG_EOS_BASE", BG_EOS_BASE, false, false);
	script->set_constant("BG_EOS_FLAG", BG_EOS_FLAG, false, false);
	/// Team DeathMatch 
	script->set_constant("BG_TD_WON", BG_TD_WON, false, false);
	script->set_constant("BG_TD_LOSS", BG_TD_LOSS, false, false);
	script->set_constant("BG_TD_TIE", BG_TD_TIE, false, false);
	script->set_constant("BG_TD_KILL", BG_TD_KILL, false, false);
	script->set_constant("BG_TD_DIE", BG_TD_DIE, false, false);
	/// Stone Control 
	script->set_constant("BG_SC_WON", BG_SC_WON, false, false);
	script->set_constant("BG_SC_LOSS", BG_SC_LOSS, false, false);
	script->set_constant("BG_SC_TIE", BG_SC_TIE, false, false);
	script->set_constant("BG_SC_DROPPED", BG_SC_DROPPED, false, false);
	script->set_constant("BG_SC_STOLE", BG_SC_STOLE, false, false);
	script->set_constant("BG_SC_CAPTURED", BG_SC_CAPTURED, false, false);
	/// Conquest 
	script->set_constant("BG_CONQ_WON", BG_CONQ_WON, false, false);
	script->set_constant("BG_CONQ_LOSS", BG_CONQ_LOSS, false, false);
	script->set_constant("BG_CONQ_EMPERIUM", BG_CONQ_EMPERIUM, false, false);
	script->set_constant("BG_CONQ_BARRICADE", BG_CONQ_BARRICADE, false, false);
	script->set_constant("BG_CONQ_GUARDIAN_S", BG_CONQ_GUARDIAN_S, false, false);
	/// Rush
	script->set_constant("BG_RUSH_WON", BG_RUSH_WON, false, false);
	script->set_constant("BG_RUSH_LOSS", BG_RUSH_LOSS, false, false);
	script->set_constant("BG_RUSH_EMPERIUM", BG_RUSH_EMPERIUM, false, false);
	/// Domination
	script->set_constant("BG_DOM_WON", BG_DOM_WON, false, false);
	script->set_constant("BG_DOM_LOSS", BG_DOM_LOSS, false, false);
	script->set_constant("BG_DOM_TIE", BG_DOM_TIE, false, false);
	script->set_constant("BG_DOM_BASE", BG_DOM_BASE, false, false);
	script->set_constant("BG_DOM_OFF_KILL", BG_DOM_OFF_KILL, false, false);
	script->set_constant("BG_DOM_DEF_KILL", BG_DOM_DEF_KILL, false, false);
	/// Points 
	script->set_constant("BG_POINTS", BG_POINTS, false, false);
	script->set_constant("BG_RANKED_POINTS", BG_RANKED_POINTS, false, false);
	
	script->set_constant("BG_TOTAL_RANKED_GAMES", BG_TOTAL_RANKED_GAMES, false, false);
	/// Leader 
	script->set_constant("BG_LEADER_WON", BG_LEADER_WON, false, false);
	script->set_constant("BG_LEADER_LOSS", BG_LEADER_LOSS, false, false);
	script->set_constant("BG_LEADER_TIE", BG_LEADER_TIE, false, false);
	/// BG Status 
	script->set_constant("BG_WON", BG_WON, false, false);
	script->set_constant("BG_TIE", BG_TIE, false, false);
	script->set_constant("BG_LOSS", BG_LOSS, false, false);
	/// HP Loss Timer 
	script->set_constant("BG_HP_VALUE", BG_HP_VALUE, false, false);
	script->set_constant("BG_HP_RATE", BG_HP_RATE, false, false);
	script->set_constant("BG_HP_TID", BG_HP_TID, false, false);
	/// SP Loss Timer 
	script->set_constant("BG_SP_VALUE", BG_SP_VALUE, false, false);
	script->set_constant("BG_SP_RATE", BG_SP_RATE, false, false);
	script->set_constant("BG_SP_TID", BG_SP_TID, false, false);
	
	
	/// Script Related Constants 

	/// Common Status
	script->set_constant("EBG_NOT_STARTED", 0, false, false);
	script->set_constant("EBG_RUNNING", 1, false, false);
	script->set_constant("EBG_ENDING", 2, false, false);
	
	/// CTF .status:
	script->set_constant("CTF_UNTOUCHED", 0, false, false);
	script->set_constant("CTF_ONGROUND", 1, false, false);
	script->set_constant("CTF_TAKEN", 2, false, false);
	
	/// EOS
	script->set_constant("EOS_ONGROUND", 0, false, false);
	script->set_constant("EOS_TAKEN", 1, false, false);
	
	/// pc_block_si
	script->set_constant("PC_BLOCK_MOVE", 1, false, false);
	script->set_constant("PC_BLOCK_ALL", 2, false, false);
	
	/// Teams
	script->set_constant("GUILLAUME", 1, false, false);
	script->set_constant("CROIX", 2, false, false);
	script->set_constant("TRAITOR", 3, false, false);
	
	/// BG Conditions
	script->set_constant("BGC_WON", BGC_WON, false, false);
	script->set_constant("BGC_LOSS", BGC_LOSS, false, false);
	script->set_constant("BGC_TIE", BGC_TIE, false, false);
	
	/// Arithmetic Constants
	script->set_constant("EBG_ADD", EBG_A_ADD, false, false);                               ///< 0
	script->set_constant("EBG_SUBSTRACT", EBG_A_SUBSTRACT, false, false);                   ///< 1
	script->set_constant("EBG_MULTIPLY", EBG_A_MULTIPLY, false, false);                     ///< 2
	script->set_constant("EBG_DIVIDE", EBG_A_DIVIDE, false, false);                         ///< 3
	script->set_constant("EBG_ZEROED", EBG_A_ZEROED, false, false);                         ///< 4
	script->set_constant("EBG_MAX", EBG_A_MAX, false, false);                               ///< 5

	/// bg_npc_cond_timer
	script->set_constant("EBG_CONDITION_LESS",  EBG_CONDITION_LESS,  false, false);         ///< 0x1
	script->set_constant("EBG_CONDITION_EQUAL",  EBG_CONDITION_EQUAL,  false, false);       ///< 0x2
	script->set_constant("EBG_CONDITION_GREATER",  EBG_CONDITION_GREATER,  false, false);   ///< 0x4
	script->set_constant("EBG_CONDITION_ELSE",  EBG_CONDITION_ELSE,  false, false);         ///< 0x8

	/// Operations
	script->set_constant("EBG_OP_MIN", BG_T_O_MIN, false, false);                         ///< 0x00000
	script->set_constant("EBG_OP_HEAL", BG_T_O_AREAHEALER, false, false);                 ///< 0x00001
	script->set_constant("EBG_OP_WARP", BG_T_O_AREAWARP, false, false);                   ///< 0x00002
	script->set_constant("EBG_OP_ADDTIME", BG_T_O_ADDTIMER, false, false);                ///< 0x00004
	script->set_constant("EBG_OP_EFFECT", BG_T_O_SHOWEFFECT, false, false);               ///< 0x00008
	script->set_constant("EBG_OP_DELETE", BG_T_O_SELFDELETE, false, false);               ///< 0x00010
	script->set_constant("EBG_OP_ADDSELF", BG_T_O_ADDSELF, false, false);                 ///< 0x00020
	script->set_constant("EBG_OP_ADDTIMESELF", BG_T_O_ADDTIMERSELF, false, false);        ///< 0x00040
	script->set_constant("EBG_OP_VIEWPOINT", BG_T_O_VIEWPOINTMAP, false, false);          ///< 0x00080
	script->set_constant("EBG_OP_CONDITION", BG_T_O_NPC_CONDITION, false, false);         ///< 0x00100
	script->set_constant("EBG_OP_REVEAL_MOB", BG_T_O_REVEAL_MOB, false, false);           ///< 0x00200
                                                                                          ///< 0x00400 (Unused)
	script->set_constant("EBG_OP_ARITHMETIC", BG_T_O_ARITHMETIC_OP, false, false);        ///< 0x00800
	script->set_constant("EBG_OP_DOEVENT", BG_T_O_DO_EVENT, false, false);                ///< 0x01000
	script->set_constant("EBG_OP_MAPANNOUNCE", BG_T_O_MAPANNOUNCE, false, false);         ///< 0x02000
	script->set_constant("EBG_OP_DELAY", BG_T_O_DELAY, false, false);                     ///< 0x04000
	script->set_constant("EBG_OP_SCRIPT", BG_T_O_SCRIPT, false, false);                   ///< 0x08000
	script->set_constant("EBG_OP_MAX", BG_T_O_MAX, false, false);                         ///< 0x10000
	
	
	/// HP/SP Time related Constants
	script->set_constant("EBG_HP_TIME", EBG_HP_TIME, false, false);                       ///< 0x1
	script->set_constant("EBG_SP_TIME", EBG_SP_TIME, false, false);                       ///< 0x2
	script->set_constant("EBG_EXTRA_TIME", EBG_EXTRA_TIME, false, false);                 ///< 0x4
	
	/// bg_can_move Constants
	script->set_constant("EBG_MOVABLE", 1, false, false);
	script->set_constant("EBG_IMMOVABLE", 0, false, false);
}

#include "eBG_common.c"
#include "eBG_functions.c"
#include "AtCommands.inc"
#include "ScriptCommands.inc"

/**
 * Executed Upon Server Start
 **/
HPExport void plugin_init(void)
{
	// Pre Hooks
	addHookPre(pc, useitem, pc_useitem_pre);
	addHookPre(pc, setpos, pc_setpos_pre);
	addHookPre(skill, not_ok, skill_notok_pre);
	addHookPre(clif, pLoadEndAck, clif_parse_LoadEndAck_pre);
	addHookPre(bg, team_leave, bg_e_team_leave);
	addHookPre(clif, guild_memberlist, send_bg_memberlist_);
#if PACKETVER <= 20120000  // For Showing Emblems.
	addHookPre(clif, sendbgemblem_area, send_bg_emblem_area);
	addHookPre(clif, sendbgemblem_single, send_bg_emblem_single);
	addHookPre(clif, maptypeproperty2, send_map_property);
	//addHookPost(clif, map_type, Stop_This_Shitty_Battleground_Check);
#endif
	addHookPre(map, quit, bg_clear_char_data_hook);
	addHookPre(skill, check_condition_castbegin, gmaster_skill_cast);
	addHookPre(clif, pUseSkillToId, unit_guild_skill);
	addHookPre(skill, castend_nodamage_id, skill_castend_guild);
	addHookPre(clif, guild_basicinfo, send_bg_basicinfo);
	addHookPre(clif, pGuildRequestEmblem, send_bg_emblem_single_);
	addHookPre(status, damage, mob_immunity);
	addHookPre(npc, reload, npc_reload_pre);
	addHookPre(npc, parse_unknown_mapflag, parse_mapflags);
	addHookPre(clif, bg_updatescore_single, bg_updatescore_mf);
	addHookPre(pc, dead, pc_dead_pre);
	addHookPre(pc, update_idle_time, update_ebg_idle);
	addHookPre(bg, send_xy_timer_sub, set_ebg_idle);
	// Post Hooks
	addHookPost(pc, dead, pc_dead_post);
	addHookPost(bg, send_dot_remove, remove_teampos);
	addHookPost(bg, send_xy_timer, send_pos_location);

	// Atcommands
	addAtcommand("get_value", get_value);
	addAtcommand("set_value", set_value);
	addAtcommand("esend",esend);
	addAtcommand("bgsend_",bgsend_);
	
	addAtcommand("bgranked", bgranked);
	addAtcommand("bgregular", bgregular);
	addAtcommand("order", order);
	addAtcommand("leader", leader);
	addAtcommand("reportafk", reportafk);
	addAtcommand("ignorebg", ignorebg);
	
	addAtcommand("debug_time", debug_time);
	addAtcommand("debug_time2", debug_time2);
	// ScriptCommands
	addScriptCommand("pc_block_si", "ii?", pc_block_si);
	addScriptCommand("bg_addpoints", "ivi???????", bg_addpoints);
	addScriptCommand("bg_reward", "iiiiisiii", bg_reward);
	addScriptCommand("map_exists", "s", map_exists);
	addScriptCommand("viewpointmap", "siiiii", viewpointmap);
	addScriptCommand("bg_hpsp_time", "iii?", bg_hpsp_time);
	addScriptCommand("bg_hpsp_time_del", "i", bg_hpsp_time_del);
	addScriptCommand("bg_e_join_team", "ii?", bg_e_join_team);
	addScriptCommand("bg_end", "i", bg_end);
	addScriptCommand("bg_change_team", "ii", bg_change_team);
	addScriptCommand("bg_reset_rank_stat","",bg_reset_rank_stat);
	addScriptCommand("on_ebg","",on_ebg);
	addScriptCommand("bg_logincheck", "s", bg_logincheck);
	addScriptCommand("bg_mapwarp", "s", bg_mapwarp);
	addScriptCommand("bg_addfame", "?", bg_addfame);
	addScriptCommand("bg_immunemob", "ii", bg_immunemob);
	addScriptCommand("bg_mob_move", "ii", bg_mob_move);
	addScriptCommand("bg_revealmob", "iii", bg_revealmob);
	addScriptCommand("bg_timer_start", "ii??", bg_timer_start);
	addScriptCommand("bg_areaheal_timer", "isiiiiii?", bg_areaheal_timer);
	addScriptCommand("bg_areawarp_timer", "isiiiisii???", bg_areawarp_timer);
	addScriptCommand("bg_effect_timer", "ii??", bg_effect_timer);
	addScriptCommand("bg_viewpoint_timer", "isiiiii?", bg_viewpoint_timer);
	addScriptCommand("bg_npc_cond_timer", "isiii???", bg_npc_cond_timer);
	addScriptCommand("bg_timer_child", "iii", bg_timer_child);
	addScriptCommand("bg_operations_timer", "ii?", bg_operations_timer);
	addScriptCommand("bg_timer_stop", "i", bg_timer_stop);
	addScriptCommand("bg_npc_data", "i??", bg_npc_data);
	addScriptCommand("bg_get_npc_data", "??", bg_get_npc_data);
	addScriptCommand("bg_areaitem", "siiii", bg_areaitem);
	addScriptCommand("bg_team_reveal", "i", bg_team_reveal);
	addScriptCommand("bg_team_score", "ii", bg_team_score);
	addScriptCommand("bg_hpsp_time_extra", "iiii", bg_hpsp_time_extra);
	addScriptCommand("bg_npc_cond_hpsp", "sii??", bg_npc_cond_hpsp);
	addScriptCommand("bg_player_cond_timer", "isiiis???", bg_player_cond_timer);
	addScriptCommand("bg_timer_exists", "i", bg_timer_exists);
	addScriptCommand("bg_insert_npc_name", "is", bg_insert_npc_name);
	addScriptCommand("bg_perform_arithmetic", "isii??", bg_perform_arithmetic);
	addScriptCommand("bg_mapannounce_timer", "issis?", bg_mapannounce_timer);
	addScriptCommand("bg_info", "ii", bg_info);
	addScriptCommand("bg_insert_event_name", "is", bg_insert_event_name);
	addScriptCommand("bg_timer_link", "iii", bg_timer_link);
	addScriptCommand("bg_announce", "sis????", bg_announce);
	addScriptCommand("bg_script_timer", "is?", bg_script_timer);
	addScriptCommand("bg_get_guild_id", "i", bg_get_guild_id);
	addScriptCommand("checkwall", "s", checkwall);
	//#include "NewVersion_init.inc"
	bg_initialize_constants();
	
#ifdef VIRT_GUILD
	addHookPre(status, get_guild_id, status_virt_guild_id);
	addHookPre(status, get_emblem_id, status_virt_emblem_id);
	addHookPre(clif, charnameupdate, clif_charname_virt);
	addHookPre(guild, getposition, guild_getposition_pre);
	addHookPre(clif, guild_belonginfo, clif_belonginfo_virt);
	addHookPre(guild, search, virt_search);
	addHookPre(clif, guild_emblem, clif_virt_guild_emblem);
	addHookPre(clif, guild_positionnamelist, clif_virt_positionname);
	addHookPre(clif, guild_positioninfolist, clif_virt_positioninfo);
	addHookPre(clif, guild_allianceinfo, clif_virt_alliance);
	addHookPre(clif, guild_skillinfo, clif_virt_skill);
	addHookPre(guild, isallied, guild_virt_allied);
	addHookPre(clif, charnameack, clif_charname_virt2);
#endif
	addHookPre(unit, walktoxy, mob_can_move);
	addHookPre(unit, walktobl, mob_can_move_v2);
#ifdef EBG_RANKING
	addHookPre(clif, skill_damage, record_max_damage);
	addHookPre(clif, damage, record_max_damage2);
	addHookPre(mob, damage, record_damage2);
	addHookPre(battle, consume_ammo, record_ammo_spent);
	addHookPost(skill, calc_heal, record_healing);
	addHookPre(skill, consume_requirement, record_requirement);
	addHookPre(pc, delitem, record_requirement_item);
	addHookPre(pc, itemheal, record_potion_used);
	addHookPost(skill, castend_nodamage_id, record_support_skill);
	addHookPre(clif, useskill, record_skill);
	addHookPre(unit, attack, record_bare_attack);
#endif
	addHookPre(bg, team_delete, bg_team_delete_pre);
	addHookPre(map, reqnickdb, map_reqnickdb_pre);
	addHookPre(bg, team_db_final, bg_team_db_final_pre);
#ifdef CHAR_SERVER_SAVE
	//addHookPost(pc, authok, pc_authok_post);
	addPacket(PACKET_CM_SYNC_DATA, -1, ebg_char_p_sync_data, hpChrif_Parse);
	addPacket(PACKET_CM_FAME_DATA, -1, ebg_char_p_fame_data, hpChrif_Parse);
	addHookPost(chrif, on_ready, chrif_on_ready_post);
#else
	addHookPre(chrif, recvfamelist, update_bg_fame);
#endif
}

/**
 * Executed Before Server Start(All BattleConfig Settings)
 **/
HPExport void server_preinit(void)
{
	addBattleConf("battle_configuration/bg_reward_rates", ebg_battleconf, ebg_battleconf_return, false);      // Reward Rates
	addBattleConf("battle_configuration/bg_ranked_mode", ebg_battleconf, ebg_battleconf_return, false);       // Rank Mode(on or off)
	addBattleConf("battle_configuration/bg_rank_bonus", ebg_battleconf, ebg_battleconf_return, false);        // Give Ranking Bonus or not(Top 10)
	addBattleConf("battle_configuration/bg_max_rank_game", ebg_battleconf, ebg_battleconf_return, false);     // Max BG Ranked Games
	addBattleConf("battle_configuration/bg_rank_rates", ebg_battleconf, ebg_battleconf_return, false);        // BG Rank Bonus
	addBattleConf("battle_configuration/bg_queue_townonly", ebg_battleconf, ebg_battleconf_return, false);    // Join only when Town
	addBattleConf("battle_configuration/bg_idle_announce", ebg_battleconf, ebg_battleconf_return, false);     // Idle Time before marking afk
	addBattleConf("battle_configuration/bg_kick_idle", ebg_battleconf, ebg_battleconf_return, false);         // AutoKick Idle
	addBattleConf("battle_configuration/bg_reportafk_leader", ebg_battleconf, ebg_battleconf_return, false);  // Usage of @reportafk
	addBattleConf("battle_configuration/bg_log_kill", ebg_battleconf, ebg_battleconf_return, false);          // Logs kill
	addBattleConf("battle_configuration/bg_reserved_char_id", ebg_battleconf, ebg_battleconf_return, false);  // BattleGround CharID
	addBattleConf("battle_configuration/woe_reserved_char_id", ebg_battleconf, ebg_battleconf_return, false); // WoE CharID
	
	// BG_Timer DB
	bg_timer_db = idb_alloc(DB_OPT_BASE);    // Build up TimerDatabase
}

/**
 * Executed Upon Server Start
 **/
HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
	timer->add_func_list(bg_hp_loss_function, "bg_hp_loss_function");   // HpSp Loss Timer.
	timer->add_func_list(bg_sp_loss_function, "bg_sp_loss_function");   // HpSp Loss Timer.
	timer->add_func_list(bg_timer_function, "bg_timer_function");       // AreaHeal NPC's
	
#ifndef VIRT_GUILD
	guild_data_load_eBG();  // Loads BG Data.
#endif

	bg_guild_build_data();  // Builds BGData.
}

/**
 * Executed Upon Server Closing
 **/
HPExport void plugin_final(void)
{
	//bg_timer_db->destroy(bg_timer_db, bg_timer_db_clear); //Destroy TimerDB
	bg_timer_db->foreach(bg_timer_db, bg_timer_db_clear);
	db_destroy(bg_timer_db);
}
/*
To Hook:
	Notice Function.
	
ToDo:
	1) ARR_FIND(0, g->max_member, i, g->member[i].account_id == ssd->status.account_id && g->member[i].char_id == ssd->status.char_id);
		if (i < g->max_member) ps = g->member[i].position;
*/

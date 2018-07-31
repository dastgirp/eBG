/**
 * Common eBG File for Char/Map Server.
 * Change only if you know what you are doing.
 */

/// Warp Location
#define EBG_WARP 32000

/// Load/Save the Data from CharServer?
/// Default: Enabled
#define CHAR_SERVER_SAVE

/**
 * eBG Configurations
 * Change if you know what you are doing.
 */
/// Max SpecialEffect that can be shown in condition timer.
#define BMAX_EFFECT 2

/// Maximum Condition Checking from timer.
#define BNPC_MAX_CONDITION 8

/// Maximum Values a NPC can save(used in bg_npc_data)
#define BEXTRA_MAX_VALUE 7

/// Maximum Child Timer a Parent can have
#define MAX_CHILD_TIMER 10

/// Battleground Colors
const unsigned int bg_colors[4] = {
	0x000000FF, ///< Guillaume
	0x00FF0000, ///< Croix
	0x0000FF00, ///< Green Team
	0x00FFFFFF  ///< Default
};

/// Enable Rankings
#define EBG_RANKING

/**
 * Guild Settings
 */
/// Enable Virtual Guild
#define VIRT_GUILD
/// Total Number of Guilds
#define TOTAL_GUILD 13
/**
 * eBG Start/End
 * Used only if VIRT_GUILD is not set.
 */
#ifndef VIRT_GUILD
	/// eBG Guild Start ID
	#define EBG_GUILDSTART 901
	/// eBG Guild End ID
	#define EBG_GUILDEND 913
#endif

/**
 * Debug Settings
 */ 
/// Enable Debug mode?
bool debug_mode = false;

/// Enable Tracking of Timer Deletion/Insertion?
/// Used For Tracking Timer, in case of crash.
/// Default: Disabled
//#define TIMER_LOG

/// Leader Variable will be integer
/// Default: Disabled
//#define LEADER_INT

/**
 * Send Types
 * @see ebg_clif_send
 */
typedef enum ebg_target {
	CLIENT_EBG = 1,       ///< Send to same team member on eBG 
	ANNOUNCE_EBG,         ///< Send to everyone who isn't ignoring
} ebg_target;

typedef enum ebg_bc_types {
	/**
	 * Announce Type (Triggers ANNOUNCE_EBG)
	 * @see ebg_target
	 */
	BC_EBG = 1,            ///< Announce only on eBG
} ebg_bc_types;

#ifdef CHAR_SERVER_SAVE
enum packets_ebg {
	EBG_PACKET_MIN_MAP = 0x4000,
	/// Map to Char
	/// <account_id>.L <char_id>.L <bg_save_flag>.Q
	PACKET_MC_REQ_DATA,  ///< Request the Character Data (MC)
	/// <len>.L <char_id>.L <bg_save_flag>.Q <Data>.Variable
	PACKET_MC_SYNC_DATA, ///< Syncronize MapData with Char-Server
	/// <type>.W
	PACKET_MC_REQ_FAME_DATA,
	EBG_PACKET_MAX_MAP,
	/// Char to Map
	EBG_PACKET_MIN_CHAR,
	/// <len>.L <account_id>.L <char_id>.L <bg_save_flag>.Q <Data>.Variable
	PACKET_CM_SYNC_DATA,  ///< Sends the Char Data to MapServer (CM)
	/// <len>.L <type>.W {<name>.24C <char_id>.L <point>.L}*10
	PACKET_CM_FAME_DATA,
	EBG_PACKET_MAX_CHAR,
};

/**
 * Required for Map->Char Server, to verify the length.
 * @var int
 */
int packets_ebg_len[EBG_PACKET_MAX_MAP - EBG_PACKET_MIN_MAP - 1] = {
	18, -1, 4,                         /// 0x4000 - 0x4010
};
#endif

enum Player_Data {
	WALK_ONLY = 1,           ///< Can Walk or Not
	BG_SAVE_TID,             ///< Save Timer ID
	/// 11-20 CTF
	CTF_MIN = 11,
	BG_CTF_WON = 11,         ///< Number of Times Player Won in CTF
	BG_CTF_LOSS,             ///< Number of Times Player Loss in CTF
	BG_CTF_TIE,              ///< Number of Times Player Tie in CTF
	CTF_ONHAND,              ///< Number of Times Player Carried Flag
	CTF_DROPPED,             ///< Number of Times Player Dropped Flag
	CTF_CAPTURED,            ///< Number of Times Player Captured Flag
	CTF_MAX,
	/// 21-30 BOSS
	BOSS_MIN = 21,
	BG_BOSS_WON = 21,        ///< Number of Times Player Won in Boss
	BG_BOSS_LOSS,            ///< Number of Times Player Loss in Boss
	BG_BOSS_TIE,             ///< Number of Times Player Tie in Boss
	BOSS_KILLED,             ///< Number of Times Player has killed Boss
	BOSS_NEUT_FLAG,          ///< Number of Times Neutral Flags has been killed
	BOSS_MAX,
	/// 31-40 TI
	TI_MIN = 31,
	BG_TI_WON = 31,          ///< Number of Times Player Won in TI
	BG_TI_LOSS,              ///< Number of Times Player Loss in TI
	BG_TI_TIE,               ///< Number of Times Player Tie in TI
	BG_TI_SKULL,             ///< Skulls got in TI
	TI_MAX,
	/// 41-50 EOS
	EOS_MIN = 41,
	BG_EOS_WON = 41,        ///< Number of Times Player Won in EyeOfStorm
	BG_EOS_LOSS,            ///< Number of Times Player Loss in EyeOfStorm
	BG_EOS_TIE,             ///< Number of Times Player Tie in EyeOfStorm
	BG_EOS_BASE,            ///< Number of Times Player have captured Base in EyeOfStorm
	BG_EOS_FLAG,            ///< Number of Times Player have captured NeutralFlag in EyeOfStorm
	EOS_MAX,
	/// 51-60 TD
	TD_MIN = 51,
	BG_TD_WON = 51,         ///< Number of Times Player Won in TeamDeathMatch
	BG_TD_LOSS,             ///< Number of Times Player Loss in TeamDeathMatch
	BG_TD_TIE,              ///< Number of Times Player Tie in TeamDeathMatch
	BG_TD_KILL,             ///< Number of Times Player have killed other player
	BG_TD_DIE,              ///< Number of Times Player have died
	TD_MAX,
	/// 61-70 SC
	BG_SC_MIN = 61,
	BG_SC_WON = 61,         ///< Number of Times Player Won in StoneControl
	BG_SC_LOSS,             ///< Number of Times Player Loss in StoneControl
	BG_SC_TIE,              ///< Number of Times Player Tie in StoneControl
	BG_SC_DROPPED,          ///< Number of Times Player Dropped Stone
	BG_SC_STOLE,            ///< Number of Times Player Stole Stone
	BG_SC_CAPTURED,         ///< Number of Times Player Captured Stone
	BG_SC_MAX,
	/// 71-80 CONQ
	CONQ_MIN = 71,
	BG_CONQ_WON = 71,       ///< Number of Times Player Won in Conquest
	BG_CONQ_LOSS,           ///< Number of Times Player Loss in Conquest
	BG_CONQ_EMPERIUM,       ///< Number of Times Player Captured Emperium
	BG_CONQ_BARRICADE,      ///< Number of Times Player Destroyed Barricade
	BG_CONQ_GUARDIAN_S,     ///< Number of Times Player Destroyed Guardian Stone
	///BG_CONQ_TIE,         ///< No Tie
	CONQ_MAX,
	/// 81-90 RUSH
	RUSH_MIN = 81,
	BG_RUSH_WON = 81,       ///< Number of Times Player Won in Rush
	BG_RUSH_LOSS,           ///< Number of Times Player Loss in Rush
	///BG_RUSH_TIE,         ///< No Tie
	BG_RUSH_EMPERIUM,       ///< Number of Times Player Captured Emperium
	RUSH_MAX,
	/// 91-100 DOM
	DOM_MIN = 91,
	BG_DOM_WON = 91,        ///< Number of Times Player Won in Domination
	BG_DOM_LOSS,            ///< Number of Times Player Loss in Domination
	BG_DOM_TIE,             ///< Number of Times Player Tie in Domination
	BG_DOM_BASE,            ///< Number of Times Player have captured Base in Domination
	BG_DOM_OFF_KILL,        ///< Number of Offensive Kills
	BG_DOM_DEF_KILL,        ///< Number of Defensive Kills
	DOM_MAX,
	/// 101-110 Common Settings
	/// 101-102 Points
	COMMON_MIN = 101,
	BG_POINTS = 101,        ///< BG Points
	BG_RANKED_POINTS,       ///< BG Ranked Points
	BG_TOTAL_RANKED_GAMES,  ///< Total Ranked Games Played.
	BG_TEAM,                ///< Which Team Player Belongs to
	COMMON_MAX, // 105
	/// 106
	BG_RANKED_GAMES = 106,  ///< Number of Ranked Games Played.
	/// 112-115 Free
	/// 116-120 Leader
	LEADER_MIN = 116,
	BG_LEADER_WON = 116,    ///< Number of Times Won as a Leader
	BG_LEADER_LOSS,         ///< Number of Times Loss as a Leader
	BG_LEADER_TIE,          ///< Number of Times Tie as a Leader
	LEADER_MAX,
	/// 121-125 Win/Tie/Loss
	COMMON_STATS_MIN = 121,
	BG_WON = 121,           ///< Constant for Win
	BG_TIE,                 ///< Constant for Tie
	BG_LOSS,                ///< Constant for Loss
	COMMON_STATS_MAX,
	/// 124, 125 Free
	/// 126-130 HP Loss
	BG_HP_VALUE = 126,      ///< HP Loss Percentage
	BG_HP_RATE,             ///< HP Loss Timer(in ms)
	BG_HP_TID,              ///< HP Loss TimerID
	/// 129, 130 Free
	/// 131-135 SP Loss
	BG_SP_VALUE = 131,      ///< SP Loss Percentage
	BG_SP_RATE,             ///< SP Loss Timer(in ms)
	BG_SP_TID,              ///< SP Loss TimerID
#ifdef EBG_RANKING
	/// 200-299 Common Ranking
	RANKING_MAX_DAMAGE = 200,            ///< MaxDamage in 1 Hit
	RANKING_DAMAGE_DEALT,                ///< Total Damage Dealt
	RANKING_DAMAGE_RECEIVED,             ///< Total Damage Received

	RANKING_HEAL_TEAM = 210,             ///< Total Healing Done to TeamMate
	RANKING_HEAL_ENEMY,                  ///< Total Healing Done to Enemy
	RANKING_SP_USED,                     ///< SP Used
	RANKING_HP_POTION,                   ///< HP Potion Consumed
	RANKING_SP_POTION,                   ///< SP Potion Consumed

	RANKING_RED_GEMSTONE = 220,          ///< Total Red Gemstone Used
	RANKING_BLUE_GEMSTONE,               ///< Total Blue Gemstone Used
	RANKING_YELLOW_GEMSTONE,             ///< Total Yellow Gemstone Used
	RANKING_POISON_BOTTLE,               ///< Total Poison Bottle Used
	RANKING_ACID_DEMONSTRATION,          ///< Total Acid Demonstration Used (> 0 Damage)
	RANKING_ACID_DEMONSTRATION_F,        ///< Total Acid Demonstration Used (No Damage)

	RANKING_SUPPORT_SKILL_TEAM = 230,    ///< Support Skill Casted at TeamMate
	RANKING_SUPPORT_SKILL_ENEMY,         ///< Support Skill Casted at Enemy

	RANKING_SPIRITB = 240,               ///< Total Spirit Ball Used
	RANKING_ZENY,                        ///< Total Zeny Used(By Skills)

	RANKING_KILLS = 250,                 ///< Total Kills
	RANKING_DEATHS,                      ///< Total Deaths
	RANKING_AMMOS,                       ///< Total Ammo Used(By Skills/Attacks)
	RANKING_SCORE,                       ///< Elo Score
	
	RANKING_SKILL_USED = 260,            ///< Skill Used(for logs)
	/// 300-399 BG Ranking
	RANKING_BG_FORCE = 300,              ///< Assume as BG Map (@see get_variable)
	RANKING_BG_BOSS_DMG = 301,           ///< Damage done to MvP's
	/// 400-499 WoE Rankings
	RANKING_WOE_FORCE = 400,             ///< Assume as WoE Map (@see get_variable)
	RANKING_WOE_EMPERIUM = 401,          ///< Emperium Destroyed
	RANKING_WOE_BARRICADE,               ///< Barriacde Destroyed
	RANKING_WOE_GUARDIAN,                ///< Guardian Killed
	RANKING_WOE_GSTONE,                  ///< Guardian Stone Destroyed

	RANKING_WOE_EMPERIUM_DAMAGE = 410,   ///< Total Damage Dealt to Emperium
	RANKING_WOE_BARRICADE_DAMAGE,        ///< Total Damage Dealt to Barriacde
	RANKING_WOE_GUARDIAN_DAMAGE,         ///< Total Damage Dealt to Guardian
	RANKING_WOE_GSTONE_DAMAGE,           ///< Total Damage Dealt to Guardian Stone
#endif
	PLAYER_DATA_MAX,
};

enum bg_save_flag {
	/// BG: 0x0001 - 0x8000
	EBG_SAVE_COMMON         = 0x00000001,     ///< Saves Common Data
	EBG_SAVE_CTF            = 0x00000002,     ///< Saves CTF Data
	EBG_SAVE_BOSS           = 0x00000004,     ///< Saves Boss Data
	EBG_SAVE_TI             = 0x00000008,     ///< Saves TI Data
	EBG_SAVE_EOS            = 0x00000010,     ///< Saves EoS Data
	EBG_SAVE_TD             = 0x00000020,     ///< Saves TeamDeathMatch Data
	EBG_SAVE_SC             = 0x00000040,     ///< Saves StoneControl Data
	EBG_SAVE_CONQ           = 0x00000080,     ///< Saves Conquest Data
	EBG_SAVE_RUSH           = 0x00000100,     ///< Saves Rush Data
	EBG_SAVE_DOM            = 0x00000200,     ///< Saves Domination Data
	//EBG_SAVE_TOD          = 0x00000400,     ///< Saves Touchdown Data
	EBG_FREE1               = 0x00000400,     ///< Free Slot 1
	EBG_SAVE_LEADER         = 0x00000800,     ///< Saves Leader Data
	// Free: 0x1000 - 0x8000
	EBG_FREE2               = 0x00001000,     ///< Free Slot 2
	EBG_FREE3               = 0x00002000,     ///< Free Slot 3
	EBG_FREE4               = 0x00004000,     ///< Free Slot 4
	EBG_FREE5               = 0x00008000,     ///< Free Slot 5
	/// BG Rank: 0x010000 - 0x400000
	EBG_SAVE_BG_RANK_DMG    = 0x00010000,     ///< Saves BG Damage Ranking Data
	EBG_SAVE_BG_RANK_HEAL   = 0x00020000,     ///< Saves BG Heal Ranking Data
	EBG_SAVE_BG_RANK_ITEM   = 0x00040000,     ///< Saves BG Item Ranking Data
	EBG_SAVE_BG_RANK_SKILL  = 0x00080000,     ///< Saves BG Skill Ranking Data
	EBG_SAVE_BG_RANK_MISC   = 0x00100000,     ///< Saves BG Misc Ranking Data
	EBG_SAVE_BG_RANK_DMG2   = 0x00200000,     ///< Saves BG Misc Ranking Data
	EBG_SAVE_BG_RANK_ALL    = 0x003F0000,     ///< Saves All BG Ranking Data
	EBG_FREE6               = 0x00400000,     ///< Free Slot 6
	// Free: 0x00400000
	/// WoE Rank: 0x00800000 - 0x20000000
	EBG_SAVE_WOE_RANK_DMG   = 0x00800000,     ///< Saves WoE Damage Ranking Data 
	EBG_SAVE_WOE_RANK_HEAL  = 0x01000000,     ///< Saves WoE Heal Ranking Data 
	EBG_SAVE_WOE_RANK_ITEM  = 0x02000000,     ///< Saves WoE Item Ranking Data 
	EBG_SAVE_WOE_RANK_SKILL = 0x04000000,     ///< Saves WoE Skill Ranking Data 
	EBG_SAVE_WOE_RANK_MISC  = 0x08000000,     ///< Saves WoE Misc Ranking Data 
	EBG_SAVE_WOE_RANK_DMG2  = 0x10000000,     ///< Saves WoE Damage Ranking Data
	EBG_SAVE_WOE_RANK_MISC2 = 0x20000000,     ///< Saves WoE Misc Ranking Data 
	EBG_SAVE_WOE_RANK_ALL   = 0x3F800000,     ///< Saves All WoE Ranking Data 
	EBG_SAVE_COMMON_STATS   = 0x40000000,     ///< Saves Kills/Deaths Log Data
	/// Save All
	EBG_SAVE_ALL            = 0x7FFFFFFF,     ///< Saves All the Data
	/// Save Common
	EBG_SAVE_ALL_COMMON     = EBG_SAVE_COMMON | EBG_SAVE_COMMON_STATS,
};


/// eBG Structures (for map/char Plugin)

/**
 * Match Result
 * Every eBG Type inherits this.
 */
struct bg_match_result {
	int win;  ///< Number of Wins in BG Match
	int loss; ///< Number of Loss in BG Match
	int tie;  ///< Number of Ties in BG Match
};

/**
 * Common Match Data
 */
struct bg_data_main {
	int points;      ///< Points Gained by Normal BG Matches
	int rank_points; ///< Points Gained by Ranked BG Matches
	int team;        ///< TeamID
	int rank_games;  ///< Total Rank Games
	int daymonth;    ///< Last Ranked Game Day
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Battleground Result Data
 * Only only for Leader as they don't have additional data to be saved.
 */
struct bg_status_data {
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Capture the Flag Data
 */
struct ctf_bg_data {
	int onhand;   ///< Flags OnHand
	int dropped;  ///< Flags Dropped
	int captured; ///< Flags Captured Successfully
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Bossnia Data
 */
struct boss_bg_data {
	int killed; ///< Boss Killed
	int flags;  ///< Flags Killed
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Tierra Infierno Data
 */
struct ti_bg_data {
	int skulls; ///< Total Skulls Gained
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Eye of Storm Data
 */
struct eos_bg_data {
	int flags; ///< Total Flags Captured
	int bases; ///< Total Bases Captured
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Team DeathMatch Data
 */
struct tdm_bg_data {
	int kills;  ///< Total Kills
	int deaths; ///< Total Deaths
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Stone Control Data
 */
struct sc_bg_data {
	int drops;    ///< Stone Drops
	int captures; ///< Stone Captures
	int steals;   ///< Stone Steals
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Conquest Data
 */
struct conq_bg_data {
	int emperium;  ///< Emperium Captured
	int barricade; ///< Barricade Destroyed
	int guardian;  ///< Guardian Stone Destroyed
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Rush Data
 */
struct rush_bg_data {
	int emp_captured;   ///< Emperium Captured
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Domination Data
 */
struct dom_bg_data {
	int bases;   ///< Total Base Captured
	int offKill; ///< Offensive Kills
	int defKill; ///< Defensive Kills
	struct bg_match_result *bgm_status; ///< Stores Match Status.
};

/**
 * Structure for all Data.
 * Contains BG and Ranking Data.
 */
struct ebg_save_data {    ///< Structure used for storing All Possible data from different kinds of BG
	struct ebg_save_data_bg {
		struct bg_data_main *common;       ///< Pointer to Common Battleground Data
		struct ctf_bg_data *ctf;           ///< Pointer to Capture the Flag Data
		struct boss_bg_data *boss;         ///< Pointer to Bossnia Data
		struct ti_bg_data *ti;             ///< Pointer to Tierra Infierno Data
		struct eos_bg_data *eos;           ///< Pointer to Eye Of Storm Data
		struct tdm_bg_data *td;            ///< Pointer to Team DeathMatch Data
		struct sc_bg_data *sc;             ///< Pointer to Stone Control data
		struct conq_bg_data *conq;         ///< Pointer to Conquest Data
		struct rush_bg_data *rush;         ///< Pointer to Rush data
		struct dom_bg_data *dom;           ///< Pointer to Domination Data
		struct bg_status_data *leader;     ///< Pointer to Leader Data
		int ranked_games;                  ///< How Many Rank games?
	} bg;
#ifdef EBG_RANKING
	struct ebg_save_data_rank {
		struct ranking_data_bg *bg;      ///< Ranking Data for BG are saved here.
		struct ranking_data_woe *woe;    ///< Ranking Data for WoE are saved here.
	} rank;
#endif
};

/**
 * HP/SP Loss Data
 */
struct bg_loss {
	int value; ///< Percentage of HP/SP to be reduced
	int rate;  ///< Time in ms after which HP/SP will be reduced
	int tid;   ///< TimerID of HP/SP Loss Timer will be saved here
};

/**
 * Used by Timer
 * Data for Operations based on Conditions
 * @see bg_timerdb_struct
 */
struct bg_cond_struct {
	char name[NAME_LENGTH];                 ///< Name of NPC
	short value[BNPC_MAX_CONDITION];        ///< Values are Stored here
	short condition[BNPC_MAX_CONDITION];    ///< 1=Less,2=Equal,4=Greater,8=Else
	int operations[BNPC_MAX_CONDITION];     ///< Operations are Stored Here
	short checkat[BNPC_MAX_CONDITION];      ///< Check condition at Index x
	bool npc_or_var;                        ///< (Player or NPC), true = npc, false = player variable
	char var[NAME_LENGTH];                  ///< Variable Name(Player Variable to be read)
	char event[NAME_LENGTH];                ///< Event to be executed
};

/**
 * HP/SP Loss Additional Data
 * Losses HP/SP every x Seconds
 * @see bg_loss
 */
struct bg_loss_extra {
	struct {
		short id;                ///< EffectID (EF_BOWLINGBASH,etc..)
	} effect;                    ///< Stores all Data related to Effect to be shown when HP/SP is loss.
	struct {
		short id;                ///< EmotionID (e_hlp,etc..)
	} emotion;                   ///< Stores all Data related to Emotion to be shown when HP/SP is loss.
	struct {
		short id;				 ///< PointNumber of viewpoint
		int color;	 ///< Color to Display
	} viewpoint;                 ///< Stores all Data related to ViewPoint to be shown when HP/SP is loss.
	struct bg_cond_struct *npcs; ///< Conditions and Operations to be checked are stored here.
};

#ifdef EBG_RANKING
/**
 * Ranking Data
 * Damage Rankings
 * @see ranking_data
 */
struct ranking_sub_damage {
	uint64 max_damage;               ///< MaxDamage in 1 Hit
	uint64 damage;                   ///< Total Damage Dealt
	uint64 damage_received;          ///< Total Damage Received
	uint64 boss_damage;              ///< Damage to MvP
};

/**
 * Ranking Data
 * Heal Related Rankings
 * @see ranking_data
 */
struct ranking_sub_heal {
	uint64 healing;                  ///< Total Healing Done to TeamMate
	uint64 healing_fail;             ///< Total Healing Done to Enemy
	unsigned int sp;                       ///< Total SP Used
	unsigned int hp_potions;               ///< Total HP Potion Used
	unsigned int sp_potions;               ///< Total SP Potion Used
};

/**
 * Ranking Data
 * Item Related Rankings
 * @see ranking_data
 */
struct ranking_sub_item {
	unsigned int red_gemstones;            ///< Total Red Gemstone Used
	unsigned int blue_gemstones;           ///< Total Blue Gemstone Used
	unsigned int yellow_gemstones;         ///< Total Yellow Gemstone Used
	unsigned int poison_bottles;           ///< Total Poison Bottle Used
	unsigned int acid_demostration;        ///< Total Acid Demonstration Used (> 0 Damage)
	unsigned int acid_demostration_fail;   ///< Total Acid Demonstration Used (No Damage)
};

/**
 * Ranking Data
 * Heal Rankings
 * @see ranking_data
 */
struct ranking_sub_skill {
	unsigned int support_skills;           ///< Support Skill Casted at TeamMate
	unsigned int support_skills_fail;      ///< Support Skill Casted at Enemy

	unsigned int spirit;                   ///< Total Spirit Ball Used
	unsigned int zeny;                     ///< Total Zeny Used(By Skills)
};

/**
 * Ranking Data
 * Miscellaneous Rankings
 * @see ranking_data
 */
struct ranking_sub_misc {
	unsigned int kills;                    ///< Total Kills
	unsigned int deaths;                   ///< Total Deaths

	unsigned int ammo;                     ///< Total Ammo Used(By Skills/Attacks)
	
	unsigned int score;                    ///< Elo Score
};

/**
 * Structure used for storing All Possible data for ranks
 * @see ranking_data_bg
 * @see ranking_data_woe
 */
struct ranking_data {
	struct ranking_sub_damage *damage;     ///< Damage Rankings are stored here
	struct ranking_sub_heal *heal;         ///< Heal Rankings are stored here
	struct ranking_sub_item *item;         ///< Item Related Rankings are stored here
	struct ranking_sub_skill *skill;       ///< Skill Related Rankings are stored here
	struct ranking_sub_misc *misc;         ///< Miscellaneous Rankings are stored here
};

/**
 * BG Ranking Data
 * Used to store Damages
 * @see ranking_data_bg
 */
struct ranking_bg_sub_damage {
	uint64 boss_dmg;       ///< Total Damage Dealt to Boss
};


/**
 * BG Ranking Data
 * @see ranking_data
 */
struct ranking_data_bg {
	struct ranking_bg_sub_damage *damage;    ///< Additional damage
	struct ranking_data *rank;               ///< Common Ranking
};

/**
 * WoE Ranking Data
 * Used to store Damages
 * @see ranking_data_woe
 */
struct ranking_woe_sub_damage {
	uint64 emperium_dmg;     ///< Total Damage Dealt to Emperium
	uint64 barricade_dmg;    ///< Total Damage Dealt to Barricade
	uint64 guardian_dmg;     ///< Total Damage Dealt to Guardian
	uint64 gstone_dmg;       ///< Total Damage Dealt to Guardian Stone
};

/**
 * WoE Ranking Data
 * Used to store how many things you have captured
 * @see ranking_data_woe
 */
struct ranking_woe_sub_misc {
	unsigned int emperium;         ///< Emperium Destroyed
	unsigned int barricade;        ///< Barricade Destroyed
	unsigned int guardian;         ///< Guardian Killed
	unsigned int gstone;           ///< Guardian Stone Destroyed
};

/**
 * WoE Ranking Data
 * @see ranking_data
 */
struct ranking_data_woe {
	struct ranking_woe_sub_damage *damage;  ///< Damage Rankings related to WoE are stored here
	struct ranking_woe_sub_misc *misc;      ///< Rankings(Capturing) related to WoE are stored here

	struct ranking_data *rank;              ///< Common Ranking
};
#endif

/**
 * Player Data
 */
struct sd_p_data {
	int char_id;           ///< CharacterID 
	/// BG Data
	struct ebg_save_data *esd;   ///< Pointer to All Kinds of eBG Data
	/// Save Data
	struct {
		/// HP/SP Loss
		struct bg_loss *hp_loss;     ///< HPLoss Data
		struct bg_loss *sp_loss;     ///< SPLoss Data
		struct bg_loss_extra *extra; ///< Saves Additional Data like Emotion/Effect.
		/// Save Timers
		int tid;                     ///< TimerID of save
	} save;
#ifndef VIRT_GUILD
	/// Temporary Guild
	struct guild *g;                 ///< Pointer to GuildData is stored here to access quickly
#endif
	/// Flags
	struct {
		unsigned int only_walk : 1;  ///< Walk only parameter.
		unsigned int ebg_afk : 1;    ///< is player AFK?
	} flag;                          ///< Flags Required for BG
	/// Ranked Matches
	struct {
		bool match;                  ///< Is this Ranked Match?
	} ranked;                        ///< All Ranked Matches Data
	bool eBG;                        ///< Participated in ExtendedBG?
	bool leader;                     ///< Player is Leader
	int kills;                       ///< Total kills in current match
	bool ignore;                     ///< Ignore BG Announcements
	int64 save_flag;                 ///< Saving Flag (enum bg_save_flag)
#ifdef EBG_RANKING
	uint16 skill_id;                 ///< SkillID Used
#endif
};

/// Enumerators
/**
 * To know whether map is BG/WoE/None
 * Used in EBG_OR_WOE
 * @see EBG_OR_WOE
 **/ 
enum {
	MAP_IS_NONE = 0x0,
	MAP_IS_BG   = 0x1,
	MAP_IS_WOE  = 0x2
};

/**
 * Allocates Memory of required structure
 * Allocates memory to the structure passed to the macro
 * @param data Structure of sd_p_data/ebg_save_data(Or Any Structure (bg_type 4))
 * @param bg_s Name of the structure in 'data'
 * @param bg_struct Structure name (used for allocating memory)
 * @param bg_type Type Of BG (BitWise), 1=Allocate Memory for this struct, 2=Allocate Memory for inner struct
 * @see BG_CREATE_DATA_SUB
 */
#define BG_CREATE_DATA(data, bg_s, bg_struct, bg_type) \
		if (create == true) { \
			if ((bg_type)&1) { \
				BG_CREATE_DATA_SUB(&(data->esd->bg), bg_s, bg_struct) \
			} \
			if ((bg_type)&2) { \
				BG_CREATE_DATA_SUB(data->esd->bg.bg_s, bgm_status, bg_match_result) \
			} \
		}

/**
 * Allocates Memory of required structure
 * Allocates memory to the structure passed to the macro
 * @param data Structure of sd_p_data/ebg_save_data(Or Any Structure (bg_type 4))
 * @param bg_s Name of the structure in 'data'
 * @param bg_struct Structure name (used for allocating memory)
 * @param bg_type Type Of BG (BitWise), 1=Allocate Memory for this struct, 2=Allocate Memory for inner struct
 * @see BG_CREATE_DATA_SUB
 */
#define BG_CREATE_DATA_CHAR(bgd, bg_s, bg_struct, bg_type) \
		if (create == true) { \
			if ((bg_type)&1) { \
				BG_CREATE_DATA_SUB(bgd, bg_s, bg_struct) \
			} \
			if ((bg_type)&2) { \
				BG_CREATE_DATA_SUB(bgd->bg_s, bgm_status, bg_match_result) \
			} \
		}

/**
 * Allocates Memory of required structure
 * Allocates memory to the structure passed to the macro
 * @param data Structure of sd_p_data/ebg_save_data(Or Any Structure (bg_type 4))
 * @param bg_s Name of the structure in 'data'
 * @param bg_struct Structure name (used for allocating memory)
 * @see BG_CREATE_DATA
 * @todo Add ZEROed BLOCK
 */
#define BG_CREATE_DATA_SUB(data, bg_s, bg_struct) \
			if ((data)->bg_s == NULL) { \
				CREATE((data)->bg_s, struct bg_struct, 1);	\
				memset((data)->bg_s, 0, sizeof(struct bg_struct)); \
			}

/**
 * Returns NULL if create is false and data is NULL.
 * @param data pointer
 **/
#define CHECK_CREATE(data) \
			if ((data) == NULL && !create) { \
				ShowWarning("CHECK_CREATE: data and create is NULL(%d) \n", type); \
				return NULL; \
			}

/**
 * Shows message only if debug_mode is true
 * @see debug_mode
 * @see showDebug
 **/
#define eShowDebug(fmt, ...) if (debug_mode) \
								showmsg->showDebug((fmt), ##__VA_ARGS__)

uint16 GetWord(uint32 val, int idx);
uint16 MakeWord(uint8 byte0, uint8 byte1);
uint32 MakeDWord(uint16 word0, uint16 word1);
#ifdef EBG_RANKING
	void *bg_get_variable_common(struct ebg_save_data *esdb, int type, bool create, int64 *save_flag, int is_bg);
#else
	void *bg_get_variable_common(struct ebg_save_data *esdb, int type, bool create, int64 *save_flag);
#endif
void set_ebg_buffer(unsigned char *buf, int *offset_, int64 *type, struct ebg_save_data *esdb);
#ifdef EBG_MAP
	void bg_load_char_data_sub(struct map_session_data *sd, struct Sql *sql_handle, struct ebg_save_data *esdb, int offset, int fd, bool use_sql, int64 save_type);
	void bg_save_common(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_leader(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_ctf(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_boss(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_ti(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_eos(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_tdm(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_sc(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_conq(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_rush(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_dom(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_ranking(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
#else
	void bg_load_char_data_sub(struct Sql *sql_handle, struct ebg_save_data *esdb, int char_id, int fd, bool use_sql, int64 save_type);
	void bg_save_common(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_leader(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_ctf(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_boss(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_ti(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_eos(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_tdm(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_sc(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_conq(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_rush(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_dom(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
	void bg_save_ranking(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type);
#endif

#ifdef CHAR_SERVER_SAVE
	void bg_clear_char_data(struct ebg_save_data *esdb, struct sd_p_data *sd_data);
#else
	void bg_clear_char_data(struct map_session_data *sd);
#endif

#define fame_list_size_bgrank MAX_FAME_LIST                  ///< Ranked BG Fame List Size
#define fame_list_size_bg MAX_FAME_LIST                      ///< Normal BG Fame List Size
struct fame_list bgrank_fame_list[fame_list_size_bgrank];    ///< Ranked BG Fame List
struct fame_list bg_fame_list[fame_list_size_bg];            ///< Normal BG Fame List

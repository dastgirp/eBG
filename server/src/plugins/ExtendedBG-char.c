#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/hercules.h"

#include "common/HPMi.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/mapindex.h"
#include "common/strlib.h"
#include "common/utils.h"
#include "common/cbasetypes.h"
#include "common/timer.h"
#include "common/showmsg.h"
#include "common/conf.h"
#include "common/db.h"
#include "common/sql.h"

#include "char/char.h"
#include "char/int_guild.h"
#include "char/inter.h"
#include "char/mapif.h"
#include "char/int_mercenary.h"

#include "HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"ExtendedBG-Char",	// Plugin name
	SERVER_TYPE_CHAR,	// Which server types this plugin works with?
	"0.1",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

#ifdef EBG_MAP
	#undef EBG_MAP	// Not Map Server
#endif

/// ebg_char_p => Character Server Packet Function
/// ebg_char => Character Server Function

struct DBMap *bg_char_db = NULL;

struct ebg_save_data;	// Structure Declaration

/**
 * It Fetches the data corresponding to constant,
 * saves it in variable(v_name) and replace value specified.
 * @param v_name Variable Name(must be a pointer)
 * @param constant Which data to be altered
 * @param value Value to be added to variable
 * @param ptr_type DataType of the variable
 **/
#define SET_VARIABLE_REPLACE_(v_name, constant, value, ptr_type) \
		(v_name) = GET_VARIABLE_SIZE(esdb, (constant), true, ptr_type); \
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
	(v_name) = GET_VARIABLE_SIZE(esdb, (constant), true, ptr_type); \
	if ((v_name) == NULL) { \
		ShowError("SET_VARIABLE_REPLACE: Cannot Replace %ld in %ld.\n", (long int)value, (long int)constant); \
	} else { \
		*(v_name) = (value); \
	} \
	i++

/**
 * Returns the Address of the said type
 * @param bgd ebg_save_data struct
 * @param type Check Player_Data enum
 * @param create Create the memory?
 * @param ptr_type DataType of the variable
 * @return Address of the type.
 **/
#ifdef EBG_RANKING
	#define GET_VARIABLE_SIZE(esdb, type, create, ptr_type) (ptr_type *)bg_get_variable_common(esdb, type, create, NULL)
#else
	#define GET_VARIABLE_SIZE(esdb, type, create, ptr_type) (ptr_type *)bg_get_variable_common(esdb, type, create, NULL, MAP_IS_NONE)
#endif


#define EBG_SAVE_DATA_INIT(high_const, low_const, data, data_type, map_type) \
		int i; \
		int sub_with = low_const; \
		bool save = false; /* Save the Data? **/ \
		data_type values[(high_const)-(low_const)+1] = { 0 }; \
		data_type *value_ptr; \
		if (esdb == NULL)	\
			return;	\
		for (i=(low_const); i <= (high_const); i++) { \
			if (map_type == MAP_IS_NONE) { \
				value_ptr = GET_VARIABLE_SIZE(esdb, i, false, data_type); \
			} else if (map_type == MAP_IS_BG) { \
				value_ptr = GET_VARIABLE_SIZE(esdb, MakeDWord(i, RANKING_BG_FORCE), false, data_type); \
			} else if (map_type == MAP_IS_WOE) { \
				value_ptr = GET_VARIABLE_SIZE(esdb, MakeDWord(i, RANKING_WOE_FORCE), false, data_type); \
			} \
			if (value_ptr != NULL) { \
				save = true; \
				values[i-(low_const)] = *value_ptr; \
			} else { \
				values[i-(low_const)] = 0; \
			} \
		}

int get_variable_(struct ebg_save_data *esdb, int type, bool create, int retVal);

#include "eBG_common.c"

 /**
 * Returns the Value of the variable.
 * @param sd Map_Session_Data
 * @param type Check Player_Data enum
 * @param create Create the memory?
 * @param retVal Returns retVal if memory is not to be created.
 * @return value of the parameter
 **/
int get_variable_(struct ebg_save_data *esdb, int type, bool create, int retVal)
{
	int *val;

	if (esdb == NULL) {
		ShowError("get_variable_: Save Data is NULL(Type:%d)\n", type);
		return 0;	/// SD doesn't exists
	}

	val = GET_VARIABLE_SIZE(esdb, type, create, int);

	if (val == NULL) {
		return retVal;
	}

	return *val;
}

/**
 * Searches for Chracter BG Data
 * @param char_id CharacterID
 * @return bg_char data as (ebg_save_data)
 **/
struct ebg_save_data* bg_char_search(int char_id)
{
	return (struct ebg_save_data*)idb_get(bg_char_db, char_id);
}

/**
 * Fetches all eBG Related Data from SQL
 **/
struct ebg_save_data *ebg_fetch_sql_db(int char_id, bool fetch_data)
{
	struct ebg_save_data *bgdb = NULL;
	CREATE(bgdb, struct ebg_save_data, 1);
	if (fetch_data) {
		bg_load_char_data_sub(inter->sql_handle, bgdb, char_id, -1, true, EBG_SAVE_ALL);
	}
	idb_put(bg_char_db, char_id, bgdb);
	return bgdb;
}

/**
 * Sends the BG Data back to Map-Server
 * <account_id>.L <char_id>.L <bg_save_flag>.L
 */
void ebg_char_p_send_data(int fd)
{
	int account_id = RFIFOL(fd, 2);
	int char_id = RFIFOL(fd, 6);
	int64 type = RFIFOQ(fd, 10);
	unsigned char buf[1024];
	struct ebg_save_data *bgdb = NULL;
	int offset = 0;
	if (char_id && type) {
		/**
		 * Skipping till 18 bytes, since if bgdb is not found, it would 
		 * fetch from SQL, which assumes no buffer.
		 */
		RFIFOSKIP(fd, 18);
		if ((bgdb = bg_char_search(char_id)) == NULL) {	// Create SQL
			bgdb = ebg_fetch_sql_db(char_id, true);
		}
		WBUFW(buf, 0) = PACKET_CM_SYNC_DATA;
		WBUFL(buf, 6) = account_id;
		WBUFL(buf, 10) = char_id;
		offset += 22;
		set_ebg_buffer(buf, &offset, &type, bgdb);
		WBUFQ(buf, 14) = type;
		WBUFL(buf, 2) = offset;
		if (offset == 22) {
			return;
		}
	}
	if (offset > 0) {
		mapif->send(fd, buf, offset);
		return;
	}
	return;
}

int ebg_char_p_save_data(int fd)
{
	// 2 = length
	int char_id = RFIFOL(fd, 6);
	int64 type = RFIFOQ(fd, 10);
	struct ebg_save_data *bgdb = NULL;	
	if (char_id && type) {
		if ((bgdb = bg_char_search(char_id)) == NULL) {	// Create SQL
			bgdb = ebg_fetch_sql_db(char_id, false);
			ShowWarning("ebg_char_p_save_data: Player tried to save data without any previous history(CharID:%d, Type:'%"PRId64"')\n", char_id, type);
			ShowWarning("Most Probably, the char-server went down. Trusting map-server data without any verification.\n");
		}
		RFIFOSKIP(fd, 18);	// Skip the 18 bytes that are already read, then continue reading other.
		bg_load_char_data_sub(inter->sql_handle, bgdb, char_id, fd, false, type);
	}
	return 1;	
}

void ebg_char_p_fame_data_sub(int fd, int type) {
	unsigned char buf[32000];
	int i, len = 8;
	
	WBUFW(buf, 0) = PACKET_CM_FAME_DATA;
	WBUFW(buf, 6) = type;	// no bitwise, send individual
	if (type == 1) {
		for(i = 0; i < fame_list_size_bgrank && bgrank_fame_list[i].id; i++) {
			memcpy(WBUFP(buf, len), &bgrank_fame_list[i], sizeof(struct fame_list));
			len += sizeof(struct fame_list);
		}
	} else if (type == 2) {
		for(i = 0; i < fame_list_size_bg && bg_fame_list[i].id; i++) {
			memcpy(WBUFP(buf, len), &bg_fame_list[i], sizeof(struct fame_list));
			len += sizeof(struct fame_list);
		}
	}

	WBUFL(buf, 2) = len;
	if (len > 8)
		mapif->send(fd, buf, len);
}

int ebg_char_p_fame_data(int fd) {
#define min_num(a, b) ((a) < (b) ? (a) : (b))
	int type = RFIFOW(fd, 2);

	int i;
	char* data;
	size_t len;
	memset(bgrank_fame_list, 0, sizeof(struct fame_list));
	memset(bg_fame_list, 0, sizeof(struct fame_list));

	RFIFOSKIP(fd, 4);

	if (type&1) {
		// BG Ranked.
		if (SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `eBG_main`.`char_id`, `eBG_main`.`rank_points`, `char`.`name` FROM `eBG_main` LEFT JOIN `char` ON `char`.`char_id` = `eBG_main`.`char_id` WHERE `eBG_main`.`rank_points` > '0' ORDER BY `eBG_main`.`rank_points` DESC LIMIT 0,%d", fame_list_size_bgrank))
			Sql_ShowDebug(inter->sql_handle);
		for (i = 0; i < fame_list_size_bgrank && SQL_SUCCESS == SQL->NextRow(inter->sql_handle); ++i) {
			SQL->GetData(inter->sql_handle, 0, &data, NULL); bgrank_fame_list[i].id = atoi(data);
			SQL->GetData(inter->sql_handle, 1, &data, NULL); bgrank_fame_list[i].fame = atoi(data);
			SQL->GetData(inter->sql_handle, 2, &data, &len); 
			if (data == NULL) {
				data = "Unknown Character";
				len = strlen(data);
			}
			memcpy(bgrank_fame_list[i].name, data, min_num(len, NAME_LENGTH));
		}
		SQL->FreeResult(inter->sql_handle);
		ebg_char_p_fame_data_sub(fd, 1);
	}
	if (type&2) {
		// BG Normal
		if (SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `eBG_main`.`char_id`, `eBG_main`.`points`, `char`.`name` FROM `eBG_main` LEFT JOIN `char` ON `char`.`char_id` = `eBG_main`.`char_id` WHERE `eBG_main`.`points` > '0' ORDER BY `eBG_main`.`points` DESC LIMIT 0,%d",  fame_list_size_bg))
			Sql_ShowDebug(inter->sql_handle);
		for (i = 0; i < fame_list_size_bg && SQL_SUCCESS == SQL->NextRow(inter->sql_handle); ++i) {
			SQL->GetData(inter->sql_handle, 0, &data, NULL); bg_fame_list[i].id = atoi(data);
			SQL->GetData(inter->sql_handle, 1, &data, NULL); bg_fame_list[i].fame = atoi(data);
			SQL->GetData(inter->sql_handle, 2, &data, &len);
			if (data == NULL) {
				data = "Unknown Character";
				len = strlen(data);
			}
			memcpy(bg_fame_list[i].name, data, min_num(len, NAME_LENGTH));
		}
		SQL->FreeResult(inter->sql_handle);
		ebg_char_p_fame_data_sub(fd, 2);
	}
	// Send Data
	return 1;
#undef min_num
}

int inter_parse_frommap_pre(int *fd_)
{
	int cmd;
	int fd = *fd_;
	int len = 0;

	cmd = RFIFOW(fd, 0);
	if (cmd <= EBG_PACKET_MIN_MAP || cmd >= EBG_PACKET_MAX_MAP) {
		//ShowDebug("ExtendedBG-char: Invalid Packet %d\n", cmd); // No error, can be map-char packet, not related to eBG
		return 1;
	}

	if ((len = inter->check_length(fd, packets_ebg_len[cmd - EBG_PACKET_MIN_MAP - 1])) == 0) {
		ShowError("ExtendedBG-char: Invalid Packet Length(%d) for Packet %d\n", len, cmd);
		hookStop();
		return 2;
	}
	
	switch(cmd) {
		case PACKET_MC_REQ_DATA:
			ebg_char_p_send_data(fd);
			break;
		case PACKET_MC_SYNC_DATA:
			ebg_char_p_save_data(fd);
			break;
		case PACKET_MC_REQ_FAME_DATA:
			ebg_char_p_fame_data(fd);
			break;
		default:
			return 0;
	}
	// No RFIFOSKIP, functions take care of it.
	hookStop();
	return 1;
}

/// Only Define all functions if VIRT_GUILD is disabled
#ifndef VIRT_GUILD

#define GS_MEMBER_UNMODIFIED 0x00
#define GS_MEMBER_MODIFIED 0x01
#define GS_MEMBER_NEW 0x02
#define GS_MEMBER_NEW 0x02

#define GS_POSITION_UNMODIFIED 0x00
#define GS_POSITION_MODIFIED 0x01

// LSB = 0 => Alliance, LSB = 1 => Opposition
#define GUILD_ALLIANCE_TYPE_MASK 0x01
#define GUILD_ALLIANCE_REMOVE 0x08

char guild_db[256] = "guild";
char guild_alliance_db[256] = "guild_alliance";
char guild_castle_db[256] = "guild_castle";
char guild_expulsion_db[256] = "guild_expulsion";
char guild_member_db[256] = "guild_member";
char guild_position_db[256] = "guild_position";
char guild_skill_db[256] = "guild_skill";
char char_db[256] = "char";
char party_db[256] = "party";
char pet_db[256] = "pet";
char mail_db[256] = "mail"; // MAIL SYSTEM
char auction_db[256] = "auction"; // Auctions System
char friend_db[256] = "friends";
char hotkey_db[256] = "hotkey";
char quest_db[256] = "quest";
char account_data_db[256] = "account_data";
char memo_db[256] = "memo";
char skill_db[256] = "skill";

static const char dataToHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

int inter_guild_tosql_pre(struct guild** g_,int *flag_){
	struct guild* g = *g_;
	int flag = *flag_;
	// Table guild (GS_BASIC_MASK)
	// GS_EMBLEM `emblem_len`,`emblem_id`,`emblem_data`
	// GS_CONNECT `connect_member`,`average_lv`
	// GS_MES `mes1`,`mes2`
	// GS_LEVEL `guild_lv`,`max_member`,`exp`,`next_exp`,`skill_point`
	// GS_BASIC `name`,`master`,`char_id`

	// GS_MEMBER `guild_member` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name`)
	// GS_POSITION `guild_position` (`guild_id`,`position`,`name`,`mode`,`exp_mode`)
	// GS_ALLIANCE `guild_alliance` (`guild_id`,`opposition`,`alliance_id`,`name`)
	// GS_EXPULSION `guild_expulsion` (`guild_id`,`account_id`,`name`,`mes`)
	// GS_SKILL `guild_skill` (`guild_id`,`id`,`lv`)

	// temporary storage for str conversion. They must be twice the size of the
	// original string to ensure no overflows will occur. [Skotlex]
	char t_info[256];
	char esc_name[NAME_LENGTH*2+1];
	char esc_master[NAME_LENGTH*2+1];
	int i=0;
	StringBuf buf;
	
	if (g->guild_id >= EBG_GUILDSTART && g->guild_id <= EBG_GUILDEND){
		if (flag&GS_MEMBER){
			SQL->EscapeStringLen(inter->sql_handle, esc_name, g->name, strnlen(g->name, NAME_LENGTH));
			SQL->EscapeStringLen(inter->sql_handle, esc_master, g->master, strnlen(g->master, NAME_LENGTH));
			*t_info = '\0';

			strcat(t_info, " members");
			// Update only needed players
			for(i=0;i<g->max_member;i++){
				struct guild_member *m = &g->member[i];
				if (!m->modified)
					continue;
				if(m->account_id) {
					//Since nothing references guild member table as foreign keys, it's safe to use REPLACE INTO
					SQL->EscapeStringLen(inter->sql_handle, esc_name, m->name, strnlen(m->name, NAME_LENGTH));
					if( SQL_ERROR == SQL->Query(inter->sql_handle, "REPLACE INTO `%s` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name`) "
						"VALUES ('%d','%d','%d','%d','%d','%d','%d','%d','%"PRIu64"','%d','%d','%d','%s')",
						"eBG_member", g->guild_id, m->account_id, m->char_id,
						m->hair, m->hair_color, m->gender,
						m->class, m->lv, m->exp, m->exp_payper, m->online, m->position, esc_name) )
						Sql_ShowDebug(inter->sql_handle);
					m->modified = GS_MEMBER_UNMODIFIED;
				}
			}
		}
		
		if (flag & GS_EMBLEM)
		{
			char emblem_data[sizeof(g->emblem_data)*2+1];
			char* pData = emblem_data;
			StrBuf->Init(&buf);
			StrBuf->Printf(&buf, "UPDATE `%s` SET ", guild_db);

			strcat(t_info, " emblem");
			// Convert emblem_data to hex
			//TODO: why not use binary directly? [ultramage]
			for(i=0; i<g->emblem_len; i++){
				*pData++ = dataToHex[(g->emblem_data[i] >> 4) & 0x0F];
				*pData++ = dataToHex[g->emblem_data[i] & 0x0F];
			}
			*pData = 0;
			StrBuf->Printf(&buf, "`emblem_len`=%d, `emblem_id`=%d, `emblem_data`='%s'", g->emblem_len, g->emblem_id, emblem_data);
			StrBuf->Printf(&buf, " WHERE `guild_id`=%d", g->guild_id);
			if( SQL_ERROR == SQL->QueryStr(inter->sql_handle, StrBuf->Value(&buf)) )
				Sql_ShowDebug(inter->sql_handle);
			StrBuf->Destroy(&buf);
		}
		/* Add Here for all guild operations.*/
		ShowInfo("Saved eBGguild (%d - %s):%s:%d\n",g->guild_id,g->name,t_info,flag&GS_MEMBER);
		hookStop();
	}
	return 1;
}

struct guild * inter_guild_fromsql_pre(int *guild_id_){
	char* data;
	size_t len;
	char* p;
	int i;
	int guild_id = *guild_id_;
		
	if (guild_id >= EBG_GUILDSTART && guild_id <= EBG_GUILDEND){
		struct guild *g = NULL;
		g = (struct guild*)idb_get(inter_guild->guild_db, guild_id);
		if( g ){
			hookStop();
			return g;
		}

		if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT g.`name`,c.`name`,g.`guild_lv`,g.`connect_member`,g.`max_member`,g.`average_lv`,g.`exp`,g.`next_exp`,g.`skill_point`,g.`mes1`,g.`mes2`,g.`emblem_len`,g.`emblem_id`,g.`emblem_data` "
			"FROM `%s` g LEFT JOIN `%s` c ON c.`char_id` = g.`char_id` WHERE g.`guild_id`='%d'", guild_db, char_db, guild_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			hookStop();
			return NULL;
		}

		if( SQL_SUCCESS != SQL->NextRow(inter->sql_handle) ){
			hookStop();
			return NULL;// Guild does not exists.
		}

		CREATE(g, struct guild, 1);

		g->guild_id = guild_id;
		SQL->GetData(inter->sql_handle,  0, &data, &len); memcpy(g->name, data, min(len, NAME_LENGTH));
		SQL->GetData(inter->sql_handle,  1, &data, &len); memcpy(g->master, data, min(len, NAME_LENGTH));
		SQL->GetData(inter->sql_handle,  2, &data, NULL); g->guild_lv = atoi(data);
		SQL->GetData(inter->sql_handle,  3, &data, NULL); g->connect_member = atoi(data);
		SQL->GetData(inter->sql_handle,  4, &data, NULL); g->max_member = atoi(data);
		if (g->max_member > MAX_GUILD) {
			// Fix reduction of MAX_GUILD [PoW]
			ShowWarning("Guild %d:%s specifies higher capacity (%d) than MAX_GUILD (%d)\n", guild_id, g->name, g->max_member, MAX_GUILD);
			g->max_member = MAX_GUILD;
		}
		SQL->GetData(inter->sql_handle,  5, &data, NULL); g->average_lv = atoi(data);
		SQL->GetData(inter->sql_handle,  6, &data, NULL); g->exp = strtoull(data, NULL, 10);
		SQL->GetData(inter->sql_handle,  7, &data, NULL); g->next_exp = (unsigned int)strtoul(data, NULL, 10);
		SQL->GetData(inter->sql_handle,  8, &data, NULL); g->skill_point = atoi(data);
		SQL->GetData(inter->sql_handle,  9, &data, &len); memcpy(g->mes1, data, min(len, sizeof(g->mes1)));
		SQL->GetData(inter->sql_handle, 10, &data, &len); memcpy(g->mes2, data, min(len, sizeof(g->mes2)));
		SQL->GetData(inter->sql_handle, 11, &data, &len); g->emblem_len = atoi(data);
		SQL->GetData(inter->sql_handle, 12, &data, &len); g->emblem_id = atoi(data);
		SQL->GetData(inter->sql_handle, 13, &data, &len);
		// convert emblem data from hexadecimal to binary
		//TODO: why not store it in the db as binary directly? [ultramage]
		for( i = 0, p = g->emblem_data; i < g->emblem_len; ++i, ++p )
		{
			if( *data >= '0' && *data <= '9' )
				*p = *data - '0';
			else if( *data >= 'a' && *data <= 'f' )
				*p = *data - 'a' + 10;
			else if( *data >= 'A' && *data <= 'F' )
				*p = *data - 'A' + 10;
			*p <<= 4;
			++data;

			if( *data >= '0' && *data <= '9' )
				*p |= *data - '0';
			else if( *data >= 'a' && *data <= 'f' )
				*p |= *data - 'a' + 10;
			else if( *data >= 'A' && *data <= 'F' )
				*p |= *data - 'A' + 10;
			++data;
		}

		// load guild member info
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name` "
			"FROM `%s` WHERE `guild_id`='%d' ORDER BY `position`", "eBG_member", guild_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			aFree(g);
			hookStop();
			return NULL;
		}
		for( i = 0; i < g->max_member && SQL_SUCCESS == SQL->NextRow(inter->sql_handle); ++i )
		{
			struct guild_member* m = &g->member[i];

			SQL->GetData(inter->sql_handle,  0, &data, NULL); m->account_id = atoi(data);
			SQL->GetData(inter->sql_handle,  1, &data, NULL); m->char_id = atoi(data);
			SQL->GetData(inter->sql_handle,  2, &data, NULL); m->hair = atoi(data);
			SQL->GetData(inter->sql_handle,  3, &data, NULL); m->hair_color = atoi(data);
			SQL->GetData(inter->sql_handle,  4, &data, NULL); m->gender = atoi(data);
			SQL->GetData(inter->sql_handle,  5, &data, NULL); m->class = atoi(data);
			SQL->GetData(inter->sql_handle,  6, &data, NULL); m->lv = atoi(data);
			SQL->GetData(inter->sql_handle,  7, &data, NULL); m->exp = strtoull(data, NULL, 10);
			SQL->GetData(inter->sql_handle,  8, &data, NULL); m->exp_payper = (unsigned int)atoi(data);
			SQL->GetData(inter->sql_handle,  9, &data, NULL); m->online = atoi(data);
			SQL->GetData(inter->sql_handle, 10, &data, NULL); m->position = atoi(data);
			if( m->position >= MAX_GUILDPOSITION ) // Fix reduction of MAX_GUILDPOSITION [PoW]
				m->position = MAX_GUILDPOSITION - 1;
			SQL->GetData(inter->sql_handle, 11, &data, &len); memcpy(m->name, data, min(len, NAME_LENGTH));
			m->modified = GS_MEMBER_UNMODIFIED;
		}

		//printf("- Read guild_position %d from sql \n",guild_id);
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `position`,`name`,`mode`,`exp_mode` FROM `%s` WHERE `guild_id`='%d'", guild_position_db, guild_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			aFree(g);
			hookStop();
			return NULL;
		}
		while( SQL_SUCCESS == SQL->NextRow(inter->sql_handle) )
		{
			int position;
			struct guild_position *pos;

			SQL->GetData(inter->sql_handle, 0, &data, NULL); position = atoi(data);
			if( position < 0 || position >= MAX_GUILDPOSITION )
				continue;// invalid position
			pos = &g->position[position];
			SQL->GetData(inter->sql_handle, 1, &data, &len); memcpy(pos->name, data, min(len, NAME_LENGTH));
			SQL->GetData(inter->sql_handle, 2, &data, NULL); pos->mode = atoi(data);
			SQL->GetData(inter->sql_handle, 3, &data, NULL); pos->exp_mode = atoi(data);
			pos->modified = GS_POSITION_UNMODIFIED;
		}

		//printf("- Read guild_alliance %d from sql \n",guild_id);
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `opposition`,`alliance_id`,`name` FROM `%s` WHERE `guild_id`='%d'", guild_alliance_db, guild_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			aFree(g);
			hookStop();
			return NULL;
		}
		for( i = 0; i < MAX_GUILDALLIANCE && SQL_SUCCESS == SQL->NextRow(inter->sql_handle); ++i )
		{
			struct guild_alliance* a = &g->alliance[i];

			SQL->GetData(inter->sql_handle, 0, &data, NULL); a->opposition = atoi(data);
			SQL->GetData(inter->sql_handle, 1, &data, NULL); a->guild_id = atoi(data);
			SQL->GetData(inter->sql_handle, 2, &data, &len); memcpy(a->name, data, min(len, NAME_LENGTH));
		}

		//printf("- Read guild_expulsion %d from sql \n",guild_id);
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `account_id`,`name`,`mes` FROM `%s` WHERE `guild_id`='%d'", guild_expulsion_db, guild_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			aFree(g);
			hookStop();
			return NULL;
		}
		for( i = 0; i < MAX_GUILDEXPULSION && SQL_SUCCESS == SQL->NextRow(inter->sql_handle); ++i )
		{
			struct guild_expulsion *e = &g->expulsion[i];

			SQL->GetData(inter->sql_handle, 0, &data, NULL); e->account_id = atoi(data);
			SQL->GetData(inter->sql_handle, 1, &data, &len); memcpy(e->name, data, min(len, NAME_LENGTH));
			SQL->GetData(inter->sql_handle, 2, &data, &len); memcpy(e->mes, data, min(len, sizeof(e->mes)));
		}

		//printf("- Read guild_skill %d from sql \n",guild_id);
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `id`,`lv` FROM `%s` WHERE `guild_id`='%d' ORDER BY `id`", guild_skill_db, guild_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			aFree(g);
			hookStop();
			return NULL;
		}

		for (i = 0; i < MAX_GUILDSKILL; i++) {
			//Skill IDs must always be initialized. [Skotlex]
			g->skill[i].id = i + GD_SKILLBASE;
		}

		while( SQL_SUCCESS == SQL->NextRow(inter->sql_handle) )
		{
			int id;
			SQL->GetData(inter->sql_handle, 0, &data, NULL); id = atoi(data) - GD_SKILLBASE;
			if( id < 0 || id >= MAX_GUILDSKILL )
				continue;// invalid guild skill
			SQL->GetData(inter->sql_handle, 1, &data, NULL); g->skill[id].lv = atoi(data);
		}
		SQL->FreeResult(inter->sql_handle);

		idb_put(inter_guild->guild_db, guild_id, g); //Add to cache
		g->save_flag |= GS_REMOVE; //But set it to be removed, in case it is not needed for long.

		ShowInfo("eBG Guild loaded (%d - %s)\n", guild_id, g->name);
		hookStop();
		return g;
	}

	return NULL;
}

int inter_guild_removemember_tosql_pre(int* account_id_, int* char_id_){
	int guild_id;
	int account_id = *account_id_, char_id = *char_id_;
	char* data;
	if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `guild_id` from `eBG_member` where `account_id` = '%d' and `char_id` = '%d'", account_id, char_id) )
	{
		Sql_ShowDebug(inter->sql_handle);
		return 0;
	}
	if (SQL_SUCCESS == SQL->NextRow(inter->sql_handle) )
	{
		SQL->GetData(inter->sql_handle,  0, &data, NULL); guild_id = atoi(data);
		if (guild_id >= EBG_GUILDSTART && guild_id <= EBG_GUILDEND){
			hookStop();
		}
		return 0;
	}
	return 0;
}

int inter_guild_broken_pre(int* guild_id)
{
	int g_id = *guild_id;
	if (g_id>= EBG_GUILDSTART && g_id <=EBG_GUILDEND){
		hookStop();
		return 0;
	}
	return 1;
}

int mapif_parse_BreakGuild_pre(int* fd, int* guild_id_)
{
	int guild_id = *guild_id_;
	if (guild_id >= EBG_GUILDSTART && guild_id <= EBG_GUILDEND){
		hookStop();
		return 0;
	}
	return 0;
}

int chr_mmo_char_tosql_pre(int *char_id_, struct mmo_charstatus** p_)
{
	int i = 0;
	int count = 0;
	int diff = 0;
	int char_id = *char_id_;
	char save_status[128]; //For displaying save information. [Skotlex]
	struct mmo_charstatus* p = *p_;
	struct mmo_charstatus *cp;
	int errors = 0; //If there are any errors while saving, "cp" will not be updated at the end.
	StringBuf buf;

	if (char_id!=p->char_id) {
		ShowError("Received Information for CharID: %d, but %d Information was saved.\n", char_id, p->char_id);
		return 0;
	}

	cp = idb_ensure(chr->char_db_, char_id, chr->create_charstatus);

	StrBuf->Init(&buf);
	memset(save_status, 0, sizeof(save_status));

	//map inventory data
	if( memcmp(p->inventory, cp->inventory, sizeof(p->inventory)) ) {
		if (!chr->memitemdata_to_sql(p->inventory, p->char_id, TABLE_INVENTORY))
			strcat(save_status, " inventory");
		else
			errors++;
	}

	//map cart data
	if( memcmp(p->cart, cp->cart, sizeof(p->cart)) ) {
		if (!chr->memitemdata_to_sql(p->cart, p->char_id, TABLE_CART))
			strcat(save_status, " cart");
		else
			errors++;
	}

	//map storage data
	if( memcmp(p->storage.items, cp->storage.items, sizeof(p->storage.items)) ) {
		if (!chr->memitemdata_to_sql(p->storage.items, MAX_STORAGE, p->account_id, TABLE_STORAGE))
			strcat(save_status, " storage");
		else
			errors++;
	}

	/*
	if (
		(
		(p->base_exp != cp->base_exp) || (p->base_level != cp->base_level) ||
		(p->job_level != cp->job_level) || (p->job_exp != cp->job_exp) ||
		(p->zeny != cp->zeny) ||
		(p->last_point.map != cp->last_point.map) ||
		(p->last_point.x != cp->last_point.x) || (p->last_point.y != cp->last_point.y) ||
		(p->max_hp != cp->max_hp) || (p->hp != cp->hp) ||
		(p->max_sp != cp->max_sp) || (p->sp != cp->sp) ||
		(p->status_point != cp->status_point) || (p->skill_point != cp->skill_point) ||
		(p->str != cp->str) || (p->agi != cp->agi) || (p->vit != cp->vit) ||
		(p->int_ != cp->int_) || (p->dex != cp->dex) || (p->luk != cp->luk) ||
		(p->option != cp->option) ||
		(p->party_id != cp->party_id) || (p->guild_id != cp->guild_id) ||
		(p->pet_id != cp->pet_id) || (p->weapon != cp->weapon) || (p->hom_id != cp->hom_id) ||
		(p->ele_id != cp->ele_id) || (p->shield != cp->shield) || (p->head_top != cp->head_top) ||
		(p->head_mid != cp->head_mid) || (p->head_bottom != cp->head_bottom) || (p->delete_date != cp->delete_date) ||
		(p->rename != cp->rename) || (p->slotchange != cp->slotchange) || (p->robe != cp->robe) ||
		(p->show_equip != cp->show_equip) || (p->allow_party != cp->allow_party) || (p->font != cp->font) ||
		(p->uniqueitem_counter != cp->uniqueitem_counter ) || (p->sex != cp->sex)
		)
	) {
		//Save status
		unsigned int opt = 0;
		char sex;

		if( p->allow_party )
			opt |= OPT_ALLOW_PARTY;
		if( p->show_equip )
			opt |= OPT_SHOW_EQUIP;

		switch (p->sex)
		{
			case 0:
				sex = 'F';
				break;
			case 1:
				sex = 'M';
				break;
			case 99:
			default:
				sex = 'U';
				break;
		}
		if (p->guild_id >= EBG_GUILDSTART && p->guild_id <= EBG_GUILDEND){
			if( SQL_ERROR == SQL->Query(
				inter->sql_handle, "UPDATE `%s` SET `base_level`='%d', `job_level`='%d',"
				"`base_exp`='%u', `job_exp`='%u', `zeny`='%d',"
				"`max_hp`='%d',`hp`='%d',`max_sp`='%d',`sp`='%d',`status_point`='%d',`skill_point`='%d',"
				"`str`='%d',`agi`='%d',`vit`='%d',`int`='%d',`dex`='%d',`luk`='%d',"
				"`option`='%d',`party_id`='%d',`pet_id`='%d',`homun_id`='%d',`elemental_id`='%d',"
				"`weapon`='%d',`shield`='%d',`head_top`='%d',`head_mid`='%d',`head_bottom`='%d',"
				"`last_map`='%s',`last_x`='%d',`last_y`='%d',`save_map`='%s',`save_x`='%d',`save_y`='%d', `rename`='%d',"
				"`delete_date`='%lu',`robe`='%d',`slotchange`='%d', `char_opt`='%u', `font`='%u', `uniqueitem_counter` ='%u',"
				" sex = '%c'"
				" WHERE  `account_id`='%d' AND `char_id` = '%d'",
				char_db, p->base_level, p->job_level,
				p->base_exp, p->job_exp, p->zeny,
				p->max_hp, p->hp, p->max_sp, p->sp, p->status_point, p->skill_point,
				p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
				p->option, p->party_id, p->pet_id, p->hom_id, p->ele_id,
				p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
				mapindex_id2name(p->last_point.map), p->last_point.x, p->last_point.y,
				mapindex_id2name(p->save_point.map), p->save_point.x, p->save_point.y, p->rename,
				(unsigned long)p->delete_date,  // FIXME: platform-dependent size
				p->robe,p->slotchange,opt,p->font,p->uniqueitem_counter, sex,
				p->account_id, p->char_id) )
			{
				Sql_ShowDebug(inter->sql_handle);
				errors++;
			} else
				strcat(save_status, " eBGStatus");
		} else{
			if( SQL_ERROR == SQL->Query(inter->sql_handle, "UPDATE `%s` SET `base_level`='%d', `job_level`='%d',"
				"`base_exp`='%u', `job_exp`='%u', `zeny`='%d',"
				"`max_hp`='%d',`hp`='%d',`max_sp`='%d',`sp`='%d',`status_point`='%d',`skill_point`='%d',"
				"`str`='%d',`agi`='%d',`vit`='%d',`int`='%d',`dex`='%d',`luk`='%d',"
				"`option`='%d',`party_id`='%d',`guild_id`='%d',`pet_id`='%d',`homun_id`='%d',`elemental_id`='%d',"
				"`weapon`='%d',`shield`='%d',`head_top`='%d',`head_mid`='%d',`head_bottom`='%d',"
				"`last_map`='%s',`last_x`='%d',`last_y`='%d',`save_map`='%s',`save_x`='%d',`save_y`='%d', `rename`='%d',"
				"`delete_date`='%lu',`robe`='%d',`slotchange`='%d', `char_opt`='%u', `font`='%u', `uniqueitem_counter` ='%u',"
				" sex = '%c'"
				" WHERE  `account_id`='%d' AND `char_id` = '%d'",
				char_db, p->base_level, p->job_level,
				p->base_exp, p->job_exp, p->zeny,
				p->max_hp, p->hp, p->max_sp, p->sp, p->status_point, p->skill_point,
				p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
				p->option, p->party_id, p->guild_id, p->pet_id, p->hom_id, p->ele_id,
				p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
				mapindex_id2name(p->last_point.map), p->last_point.x, p->last_point.y,
				mapindex_id2name(p->save_point.map), p->save_point.x, p->save_point.y, p->rename,
				(unsigned long)p->delete_date,  // FIXME: platform-dependent size
				p->robe,p->slotchange,opt,p->font,p->uniqueitem_counter, sex,
				p->account_id, p->char_id) )
			{
				Sql_ShowDebug(inter->sql_handle);
				errors++;
			} else
				strcat(save_status, " eStatus");

		}
	}
	*/

	if( p->bank_vault != cp->bank_vault || p->mod_exp != cp->mod_exp || p->mod_drop != cp->mod_drop || p->mod_death != cp->mod_death ) {
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "REPLACE INTO `%s` (`account_id`,`bank_vault`,`base_exp`,`base_drop`,`base_death`) VALUES ('%d','%d','%d','%d','%d')",account_data_db,p->account_id,p->bank_vault,p->mod_exp,p->mod_drop,p->mod_death) ) {
			Sql_ShowDebug(inter->sql_handle);
			errors++;
		} else
			strcat(save_status, " accdata");
	}

	//Values that will seldom change (to speed up saving)
	if (
		(p->hair != cp->hair) || (p->hair_color != cp->hair_color) || (p->clothes_color != cp->clothes_color) ||
		(p->class != cp->class) ||
		(p->partner_id != cp->partner_id) || (p->father != cp->father) ||
		(p->mother != cp->mother) || (p->child != cp->child) ||
		(p->karma != cp->karma) || (p->manner != cp->manner) ||
		(p->fame != cp->fame)
	)
	{
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "UPDATE `%s` SET `class`='%d',"
			"`hair`='%d',`hair_color`='%d',`clothes_color`='%d',"
			"`partner_id`='%d', `father`='%d', `mother`='%d', `child`='%d',"
			"`karma`='%d',`manner`='%d', `fame`='%d'"
			" WHERE  `account_id`='%d' AND `char_id` = '%d'",
			char_db, p->class,
			p->hair, p->hair_color, p->clothes_color,
			p->partner_id, p->father, p->mother, p->child,
			p->karma, p->manner, p->fame,
			p->account_id, p->char_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			errors++;
		} else
			strcat(save_status, " status2");
	}

	/* Mercenary Owner */
	if( (p->mer_id != cp->mer_id) ||
		(p->arch_calls != cp->arch_calls) || (p->arch_faith != cp->arch_faith) ||
		(p->spear_calls != cp->spear_calls) || (p->spear_faith != cp->spear_faith) ||
		(p->sword_calls != cp->sword_calls) || (p->sword_faith != cp->sword_faith) )
	{
		if (inter_mercenary->owner_tosql(char_id, p))
			strcat(save_status, " mercenary");
		else
			errors++;
	}

	//memo points
	if( memcmp(p->memo_point, cp->memo_point, sizeof(p->memo_point)) )
	{
		char esc_mapname[NAME_LENGTH*2+1];

		//`memo` (`memo_id`,`char_id`,`map`,`x`,`y`)
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `char_id`='%d'", memo_db, p->char_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			errors++;
		}

		//insert here.
		StrBuf->Clear(&buf);
		StrBuf->Printf(&buf, "INSERT INTO `%s`(`char_id`,`map`,`x`,`y`) VALUES ", memo_db);
		for( i = 0, count = 0; i < MAX_MEMOPOINTS; ++i )
		{
			if( p->memo_point[i].map )
			{
				if( count )
					StrBuf->AppendStr(&buf, ",");
				SQL->EscapeString(inter->sql_handle, esc_mapname, mapindex_id2name(p->memo_point[i].map));
				StrBuf->Printf(&buf, "('%d', '%s', '%d', '%d')", char_id, esc_mapname, p->memo_point[i].x, p->memo_point[i].y);
				++count;
			}
		}
		if( count )
		{
			if( SQL_ERROR == SQL->QueryStr(inter->sql_handle, StrBuf->Value(&buf)) )
			{
				Sql_ShowDebug(inter->sql_handle);
				errors++;
			}
		}
		strcat(save_status, " memo");
	}

	//skills
	if( memcmp(p->skill, cp->skill, sizeof(p->skill)) ) {
		//`skill` (`char_id`, `id`, `lv`)
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `char_id`='%d'", skill_db, p->char_id) ) {
			Sql_ShowDebug(inter->sql_handle);
			errors++;
		}

		StrBuf->Clear(&buf);
		StrBuf->Printf(&buf, "INSERT INTO `%s`(`char_id`,`id`,`lv`,`flag`) VALUES ", skill_db);
		//insert here.
		for( i = 0, count = 0; i < MAX_SKILL_DB; ++i ) {
			if( p->skill[i].id != 0 && p->skill[i].flag != SKILL_FLAG_TEMPORARY ) {
				if( p->skill[i].lv == 0 && ( p->skill[i].flag == SKILL_FLAG_PERM_GRANTED || p->skill[i].flag == SKILL_FLAG_PERMANENT ) )
					continue;
				if( p->skill[i].flag != SKILL_FLAG_PERMANENT && p->skill[i].flag != SKILL_FLAG_PERM_GRANTED && (p->skill[i].flag - SKILL_FLAG_REPLACED_LV_0) == 0 )
					continue;
				if( count )
					StrBuf->AppendStr(&buf, ",");
				StrBuf->Printf(&buf, "('%d','%d','%d','%d')", char_id, p->skill[i].id,
								 ( (p->skill[i].flag == SKILL_FLAG_PERMANENT || p->skill[i].flag == SKILL_FLAG_PERM_GRANTED) ? p->skill[i].lv : p->skill[i].flag - SKILL_FLAG_REPLACED_LV_0),
								 p->skill[i].flag == SKILL_FLAG_PERM_GRANTED ? p->skill[i].flag : 0);/* other flags do not need to be saved */
				++count;
			}
		}
		if( count )
		{
			if( SQL_ERROR == SQL->QueryStr(inter->sql_handle, StrBuf->Value(&buf)) )
			{
				Sql_ShowDebug(inter->sql_handle);
				errors++;
			}
		}

		strcat(save_status, " skills");
	}

	diff = 0;
	for(i = 0; i < MAX_FRIENDS; i++){
		if(p->friends[i].char_id != cp->friends[i].char_id ||
			p->friends[i].account_id != cp->friends[i].account_id){
			diff = 1;
			break;
		}
	}

	if(diff == 1) {
		//Save friends
		if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `char_id`='%d'", friend_db, char_id) )
		{
			Sql_ShowDebug(inter->sql_handle);
			errors++;
		}

		StrBuf->Clear(&buf);
		StrBuf->Printf(&buf, "INSERT INTO `%s` (`char_id`, `friend_account`, `friend_id`) VALUES ", friend_db);
		for( i = 0, count = 0; i < MAX_FRIENDS; ++i )
		{
			if( p->friends[i].char_id > 0 )
			{
				if( count )
					StrBuf->AppendStr(&buf, ",");
				StrBuf->Printf(&buf, "('%d','%d','%d')", char_id, p->friends[i].account_id, p->friends[i].char_id);
				count++;
			}
		}
		if( count )
		{
			if( SQL_ERROR == SQL->QueryStr(inter->sql_handle, StrBuf->Value(&buf)) )
			{
				Sql_ShowDebug(inter->sql_handle);
				errors++;
			}
		}
		strcat(save_status, " friends");
	}

#ifdef HOTKEY_SAVING
	// hotkeys
	StrBuf->Clear(&buf);
	StrBuf->Printf(&buf, "REPLACE INTO `%s` (`char_id`, `hotkey`, `type`, `itemskill_id`, `skill_lvl`) VALUES ", hotkey_db);
	diff = 0;
	for(i = 0; i < ARRAYLENGTH(p->hotkeys); i++){
		if(memcmp(&p->hotkeys[i], &cp->hotkeys[i], sizeof(struct hotkey)))
		{
			if( diff )
				StrBuf->AppendStr(&buf, ",");// not the first hotkey
			StrBuf->Printf(&buf, "('%d','%u','%u','%u','%u')", char_id, (unsigned int)i, (unsigned int)p->hotkeys[i].type, p->hotkeys[i].id , (unsigned int)p->hotkeys[i].lv);
			diff = 1;
		}
	}
	if(diff) {
		if( SQL_ERROR == SQL->QueryStr(inter->sql_handle, StrBuf->Value(&buf)) )
		{
			Sql_ShowDebug(inter->sql_handle);
			errors++;
		} else
			strcat(save_status, " hotkeys");
	}
#endif

	StrBuf->Destroy(&buf);
	if (save_status[0]!='\0')
		ShowInfo("Saved eBGchar %d - %s:%s.\n", char_id, p->name, save_status);
	if (!errors)
		memcpy(cp, p, sizeof(struct mmo_charstatus));
	if (p->guild_id >= EBG_GUILDSTART && p->guild_id <= EBG_GUILDEND)
		hookStop();
	return 0;
}
#endif
/**
 * Free's the Data occupied by the character
 * @method char_set_char_offline
 * @param  char_id               CharacterID
 * @param  account_id            AccountID
 */
void char_set_char_offline_pre(int *char_id, int *account_id) {
	struct ebg_save_data *bgdb = NULL;
	if (*char_id) {
		if ((bgdb = bg_char_search(*char_id)) == NULL) {	// No Data Found
			return;
		}
		bg_clear_char_data(bgdb, NULL);
		// Remove from Memory.
		idb_remove(bg_char_db, *char_id);
	}
	return;
}

/**
 * see \@DBMap
 * Runs from db->foreach
 * Clear's all the Character DB
 **/
int bg_char_db_clear(union DBKey key, struct DBData *data, va_list ap)
{
	struct ebg_save_data *esdb = (struct ebg_save_data*)DB->data2ptr(data);
	if (esdb) {
		//bg_timer_free(bgdb, key.i, 5);
		bg_clear_char_data(esdb, NULL);
		idb_remove(bg_char_db, key.i);
	}
	return 0;
}

HPExport void plugin_init(void) {
#ifndef VIRT_GUILD
	/// Pre Hooks
	addHookPre(inter_guild, tosql, inter_guild_tosql_pre);
	addHookPre(inter_guild, fromsql, inter_guild_fromsql_pre);
	addHookPre(inter_guild, removemember_tosql, inter_guild_removemember_tosql_pre);
	addHookPre(inter_guild, broken, inter_guild_broken_pre);
	addHookPre(mapif, parse_BreakGuild, mapif_parse_BreakGuild_pre);
	addHookPre(chr, mmo_char_tosql, chr_mmo_char_tosql_pre);
#endif
#if 0
	addHookPost(chr, auth_ok, chr_auth_ok_pre);
	//addHookPre()
#endif
#ifdef CHAR_SERVER_SAVE
	addHookPre(inter, parse_frommap, inter_parse_frommap_pre);
	addHookPre(chr, set_char_offline, char_set_char_offline_pre);
	bg_char_db = idb_alloc(DB_OPT_BASE);    // Create Character DB
#endif
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
#if defined(VIRT_GUILD)
	ShowInfo("eBG-Char: Enabled VirtGuild.\n");
#endif
#if defined(CHAR_SERVER_SAVE)
	ShowInfo("eBG-Char: Enabled CharServerSave.\n");
#endif
#if !defined(VIRT_GUILD) && !defined(CHAR_SERVER_SAVE)
	ShowInfo ("'eBG-Char: Disabled\n",pinfo.name,pinfo.version);
#endif
}

/**
 * Executed Upon Server Closing
 **/
HPExport void plugin_final(void)
{
#ifdef CHAR_SERVER_SAVE
	bg_char_db->foreach(bg_char_db, bg_char_db_clear);
	db_destroy(bg_char_db);
#endif
}

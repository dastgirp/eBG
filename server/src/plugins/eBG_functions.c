/**
 * Loads All Possible BGData.
 * @param sd Map_Session_Data
 */
void bg_load_char_data(struct map_session_data* sd)
{
#ifdef CHAR_SERVER_SAVE
	WFIFOHEAD(chrif->fd, 18);
	WFIFOW(chrif->fd, 0) = PACKET_MC_REQ_DATA;
	WFIFOL(chrif->fd, 2) = sd->status.account_id;
	WFIFOL(chrif->fd, 6) = sd->status.char_id;
	WFIFOQ(chrif->fd, 10) = EBG_SAVE_ALL;	// Which Data to Request?
	WFIFOSET(chrif->fd, 18);
#else
	struct sd_p_data *sd_data;

	if (sd == NULL) ///< SD doesn't exists
		return;
	sd_data = pdb_search(sd, true);
	if (sd_data->bgd == NULL)
		CREATE(sd_data->bgd, struct bg_data, 1);
	
#ifndef VIRT_GUILD
	if (sd->status.guild_id >= GET_EBG_GUILD_ID(0) && sd->status.guild_id <= GET_EBG_GUILD_ID(TOTAL_GUILD)) {
		sd->status.guild_id = 0;
		sd->guild = NULL;
	}
#endif // end VIRT_GUILD
	bg_load_char_data_sub(sd, map->mysql_handle, sd_data->bgd, sd->status.char_id, true);
#endif // end !CHAR_SERVER_SAVE
	return;
}

/**
 * Saves All Data of Player(BG)
 * @param sd Map_Session_Data
 * @param bg_type See Structure Battleground_Types
 */
void bg_save_all(struct map_session_data* sd, enum Battleground_Types bg_type)
{

/**
 * Checks if BgType matches and calls the save function.
 * @param type BG Type (@see Battleground_Types)
 * @param func Function Name to call
 * @param sv_flag enum bg_save_flag
 * If sv_flag is found, save it, and remove it.
 */
#define IF_BG(type, sv_flag) \
	if (((data->save_flag&(sv_flag)) && bg_type == BGT_MAX) || \
		((data->save_flag&(sv_flag)) && bg_type == type) ) { \
		ebg_request_save_data(sv_flag, data, sd); \
		data->save_flag &= ~(sv_flag); \
	}

	struct sd_p_data* data;
	data = pdb_search(sd, false);

	if (data == NULL)
		return;

	IF_BG(BGT_COMMON, EBG_SAVE_COMMON|EBG_SAVE_COMMON_STATS);
	IF_BG(BGT_CTF, EBG_SAVE_CTF);
	IF_BG(BGT_BOSS, EBG_SAVE_BOSS);
	IF_BG(BGT_TI, EBG_SAVE_TI);
	IF_BG(BGT_EOS, EBG_SAVE_EOS);
	IF_BG(BGT_TD, EBG_SAVE_TD);
	IF_BG(BGT_SC, EBG_SAVE_SC);
	IF_BG(BGT_CONQ, EBG_SAVE_CONQ);
	IF_BG(BGT_RUSH, EBG_SAVE_RUSH);
	IF_BG(BGT_DOM, EBG_SAVE_DOM);
	IF_BG(BGT_MAX, EBG_SAVE_LEADER); /// check this all the time.
	IF_BG(BGT_MAX, EBG_SAVE_BG_RANK_ALL);  /// BG Rankings
	IF_BG(BGT_MAX, EBG_SAVE_WOE_RANK_ALL); /// WoE Rankings
#undef IF_BG
}
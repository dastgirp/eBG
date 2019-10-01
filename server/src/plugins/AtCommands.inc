/**
 * @param fd Constant Value (fd of sd)
 * @param sd map_session_data (Cannot be null)
 * @param command CommandName that is executed
 * @param message All the arguments
 * @param info @see AtCommandInfo
 **/
ACMD(get_value)
{
	char output[100];
	int get_value = 0;
	int ret_val;
	if (sscanf(message, "%d", &get_value) < 1) {
		clif->message(fd, "get_value: Invalid Format..");
		return false;
	}
	ret_val = get_variable_(sd, get_value, false, 0);
	sprintf(output, "Value for %d: %d", get_value, ret_val);
	clif->message(fd, output);
	return true;
}

/**
 * For Debugging
 * @see get_value For all the ACMD parameters
 **/
ACMD(set_value)
{
	char output[100];
	int flag, set_value;
	int *bg_value;
	if (sscanf(message, "%d %d", &flag, &set_value) < 2){
		clif->message(fd, "get_value: Invalid Format..");
		return false;
	}
	bg_value = GET_VARIABLE_SIZE(sd, flag, true, int);
	if (bg_value != NULL) {
		*bg_value = (*bg_value) + set_value;
		sprintf(output, "Value for %d: %d", flag, *bg_value);
	} else {
		sprintf(output, "%d cannot be added to %d.", set_value, flag);
	}
	clif->message(fd, output);
	return true;
}

/**
 * For Debugging
 * @see get_value For all the ACMD parameters
 **/
ACMD(esend)
{
	uint8 buf[12];
	struct block_list* bl = &(sd->bl);


	// TODO this packet doesn't force the update of ui components that have the emblem visible
	//      (emblem in the flag npcs and emblem over the head in agit maps) [FlavioJS]
	WBUFW(buf,0) = 0x1b4;
	WBUFL(buf,2) = bl->id;
	WBUFL(buf,6) = status->get_guild_id(bl);
	WBUFW(buf,10) = status->get_emblem_id(bl);
	clif->send(buf, 12, bl, AREA);
	return true;
}

/**
 * For Debugging
 * @see get_value For all the ACMD parameters
 **/
ACMD(bgsend_)
{
	if (map->list[sd->bl.m].flag.battleground ){
		clif->map_type(sd, MAPTYPE_BATTLEFIELD); // Battleground Mode
		if( map->list[sd->bl.m].flag.battleground >= 2 )
			clif->bg_updatescore_single(sd);
	}
	if (map->list[sd->bl.m].flag.battleground)
		clif->map_property(sd, MAPPROPERTY_AGITZONE);
	return true;
}

/**
 * For Debugging
 * @see get_value For all the ACMD parameters
 **/
ACMD(debug_time)
{
	bg_timer_db->foreach(bg_timer_db, bg_timer_db_debug, sd);
	return true;
}

/**
 * For Debugging
 * Gives Detailed Explanation of given timer_id
 **/
ACMD(debug_time2)
{
	int uid = 0;
	struct bg_timerdb_struct* tdb;
	char output[256];
	int i;
	if (!*message || !(uid = atoi(message))) {
		clif->message(fd, "Usage: @debug_time2 <TimerID>");
		return false;
	}
	tdb = bg_timer_search(uid);
	if (tdb == NULL) {
		sprintf(output, "UID(%d) not found.", uid);
		clif->message(fd, output);
		return true;
	}
	sprintf(output, "UID: %d, Operation: %d, timer_id: %d, Timer: %d, ParentUID: %d, Random Timer: %d~%d s", uid, tdb->operations, tdb->timer_id, tdb->timer_, tdb->parent_uid, tdb->random_timer[0], tdb->random_timer[1]);
	clif->message(fd, output);
	for (i = BG_T_O_MIN + 1; i < BG_T_O_MAX; i = (i << 1)) {
		if (tdb->operations&i) {
			sprintf(output, "  Operation: %d:", i);
			clif->message(fd, output);
			switch(i) {
				case BG_T_O_AREAHEALER:
					if (tdb->areaheal != NULL) {
						sprintf(output, "\tx1: %d y1: %d, x2: %d y2: %d, HP: %d, SP: %d (HP/SP are in Percentage), MapIndex: %d",
										tdb->areaheal->x1, tdb->areaheal->y1, tdb->areaheal->x2, tdb->areaheal->y2,
										tdb->areaheal->hp, tdb->areaheal->sp, tdb->areaheal->m);
						clif->message(fd, output);
					} else {
						sprintf(output, "\tError: Areaheal Timer not found.");
						clif->message(fd, output);
					}
					break;
				
				case BG_T_O_AREAWARP:
					if (tdb->areawarp != NULL) {
						sprintf(output, "\tx0: %d, y0: %d, x1: %d y1: %d, x2: %d y2: %d, y3: %d, y4: %d, MapIndex: %d",
										tdb->areawarp->x0, tdb->areawarp->y0, tdb->areawarp->x1, tdb->areawarp->y1,
										tdb->areawarp->x2, tdb->areawarp->y2, tdb->areawarp->x3, tdb->areawarp->y3,
										tdb->areawarp->m);
						clif->message(fd, output);
					} else {
						sprintf(output, "\tError: AreaWarp Timer not found.");
						clif->message(fd, output);
					}
					break;
				
				default:
					sprintf(output, "\t Not Implemented");
					clif->message(fd, output);
			}
		}
	}
	return true;
}

/**
 * Display Ranked Ranking
 * @see get_value For all the ACMD parameters
 **/
ACMD(bgranked)
{
	int i;
	char output[100];

	clif->message(fd, "============= Ranked Battleground Ranking =============");
	for(i = 0; i < MAX_FAME_LIST && bgrank_fame_list[i].id; i++) {
		sprintf(output, "%d - %s - %d points", (i + 1), bgrank_fame_list[i].name, bgrank_fame_list[i].fame);
		clif->message(fd, output);
	}
	if (i == 0) {
		clif->message(fd, "No Players.");
	}
	clif->message(fd, "============= Ranked Battleground Ranking =============");
	return true;
}

/**
 * Display Regular Ranking
 * @see get_value For all the ACMD parameters
 **/
ACMD(bgregular)
{
	int i;
	char output[100];

	clif->message(fd, "============= Regular Battleground Ranking =============");
	for (i = 0; i < MAX_FAME_LIST && bg_fame_list[i].id; i++) {
		sprintf(output, "%d - %s - %d points", i + 1, bg_fame_list[i].name, bg_fame_list[i].fame);
		clif->message(fd, output);
	}
	if (i == 0) {
		clif->message(fd, "No Players.");
	}
	clif->message(fd, "============= Regular Battleground Ranking =============");
	return true;
}

/**
 * Announce message to team members
 * @see get_value For all the ACMD parameters
 **/
ACMD(order)
{
	struct sd_p_data *data = pdb_search(sd, false);
	if (!*message) {
		clif->message(fd, "Please, enter a message (usage: @order <message>).");
		return true;
	}

	if (data != NULL && data->eBG) {
		unsigned int color;
		struct battleground_data *bgd = bg->team_search(sd->bg_id);
		struct bg_extra_info *bg_data_t;
		if (bgd == NULL || (bg_data_t = bg_extra_create(bgd, false)) == NULL) {
			clif->message(fd, "This command is only allowed while in eBG.");
			return false;
		}
		if (!data->leader) {
			clif->message(fd, "This command is reserved for Team Leaders Only.");
			return false;
		}
		color = bg_colors[
#ifdef VIRT_GUILD
			GET_ORIG_GUILD_ID(bg_data_t->g->guild_id)
#else
			GET_ORIG_GUILD_ID(sd_data->g->guild_id)
#endif
		];
		ebg_broadcast2(&sd->bl, message, (int)strlen(message) + 1, color, 0x190, 20, 0, 0, CLIENT_EBG);
	} else {
		if (!sd->state.gmaster_flag) {
			clif->message(fd, "This command is reserved for Guild Leaders Only.");
			return false;
		}
		clif->broadcast2(&sd->bl, message, (int)strlen(message) + 1, 0xFF0000, 0x190, 20, 0, 0, GUILD);
	}

	return true;
}

/**
 * Changes Leader
 * @see get_value For all the ACMD parameters
 **/
ACMD(leader)
{
	struct map_session_data *pl_sd;
	struct sd_p_data *data = pdb_search(sd, false), *pl_data;
	struct bg_extra_info *bg_data_t;
	struct battleground_data *bgd;
	char output[100];
	unsigned int color;

	if (data == NULL || !data->eBG || sd->bg_id == 0 || (bgd = bg->team_search(sd->bg_id)) == NULL || (bg_data_t = bg_extra_create(bgd, false)) == NULL) {
		clif->message(fd, "Command only allowed while in eBG.");
		return false;
	}
	if (!data->leader) {
		clif->message(fd, "This command is reserved for Team Leaders Only.");
		return false;
	}
	if (sd->ud.skilltimer != INVALID_TIMER) {
		clif->message(fd, "Command not allow while casting a skill.");
		return false;
	}
	if (!*message) {
		clif->message(fd, "Please, enter the new Leader name (usage: @leader <name>).");
		return false;
	}
	if ((pl_sd = map->nick2sd(message, true)) == NULL) {
		clif->message(fd, msg_txt(3)); // Character not found.
		return false;
	}
	if (sd->bg_id != pl_sd->bg_id) {
		clif->message(fd, "Destination Player is not in your Team.");
		return false;
	}
	if (sd == pl_sd) {
		clif->message(fd, "You are already the Team Leader.");
		return false;
	}
	if ((pl_data = pdb_search(pl_sd, false)) == NULL) {
		clif->message(fd, "The Player Data not found.");
		return false;
	}
#ifdef VIRT_GUILD
	color = bg_colors[GET_ORIG_GUILD_ID(bg_data_t->g->guild_id)];
#else
	color = bg_colors[GET_ORIG_GUILD_ID(sd_data->g->guild_id)];
#endif

	sprintf(output, "Team Leader transfered to [%s]", pl_sd->status.name);

	ebg_broadcast2(&sd->bl, output, (int)strlen(output) + 1, color, 0x190, 20, 0, 0, CLIENT_EBG);

	data->leader = false;
#ifdef LEADER_INT
	bg_data_t->leader = pl_sd->status.char_id;
#else
	bg_data_t->leader->char_id = pl_sd->status.char_id;	
#endif
	pl_data->leader = true;

	// Update Name for both character
	clif->charnameupdate(sd);
	clif->charnameupdate(pl_sd);
	return true;
}

/**
 * Kicks AFK player
 * @see get_value For all the ACMD parameters
 **/
ACMD(reportafk)
{
	struct map_session_data *pl_sd;
	struct sd_p_data *data = pdb_search(sd, false), *pl_data;
	struct bg_extra_info *bg_data_t;
	struct battleground_data *bgd;
	char output[100];
	unsigned int color;

	if (data == NULL || !data->eBG || sd->bg_id == 0 || (bgd = bg->team_search(sd->bg_id)) == NULL || (bg_data_t = bg_extra_create(bgd, false)) == NULL) {
		clif->message(fd, "Command only allowed while in eBG.");
		return false;
	}
	if (!data->leader && bg_reportafk_leader == 0) {
		clif->message(fd, "This command is reserved for Team Leaders Only.");
		return false;
	}
	if (!*message) {
		clif->message(fd, "Please, enter the character name (usage: @reportafk <name>).");
		return false;
	}
	if ((pl_sd = map->nick2sd(message, true)) == NULL) {
		clif->message(fd, msg_txt(3)); // Character not found.
		return false;
	}
	if (sd->bg_id != pl_sd->bg_id) {
		clif->message(fd, "Destination Player is not in your Team.");
		return false;
	}
	if (sd == pl_sd) {
		clif->message(fd, "You cannot kick yourself,");
		return false;
	}
	if ((pl_data = pdb_search(pl_sd, false)) == NULL || pl_data->flag.ebg_afk == 0) {
		clif->message(fd, "The player is not AFK on this Battleground.");
		return false;
	}

#ifdef VIRT_GUILD
	color = bg_colors[GET_ORIG_GUILD_ID(bg_data_t->g->guild_id)];
#else
	color = bg_colors[GET_ORIG_GUILD_ID(sd_data->g->guild_id)];
#endif
	
	sprintf(output, "- AFK [%s] Kicked -", pl_sd->status.name);
	ebg_broadcast2(&sd->bl, output, (int)strlen(output) + 1, color, 0x190, 20, 0, 0, CLIENT_EBG);
	
	bg->team_leave(pl_sd, BGTL_AFK);
	clif->message(pl_sd->fd, "You have been kicked from Battleground because of your AFK status.");
	pc->setpos(pl_sd, pl_sd->status.save_point.map, pl_sd->status.save_point.x, pl_sd->status.save_point.y, CLR_OUTSIGHT);

	return true;
}

/**
 * Ignore all BG Announcements
 * @see get_value For all the ACMD parameters
 **/
ACMD(ignorebg)
{
	struct sd_p_data *data = pdb_search(sd, true);
	if (data == NULL) {
		return false;
	}
	
	if (data->ignore) {
		data->ignore = false;
		clif->message(fd, "You are now listening to Battleground announcements.");
	} else {
		data->ignore = true;
		clif->message(fd, "You are now ignoring Battleground announcements.");
	}

	return true;
}

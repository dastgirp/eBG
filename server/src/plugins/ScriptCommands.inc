/**
 * @pc_block_si
 * @param 1 : 1= Block Moves, 2= Can Only Move
 * @param 2 : Value to Set
 * @param 3 : Player AccountID(Optional)
 * Reference from pcblockmove
 **/
BUILDIN(pc_block_si)
{
	int id = 0, type = script_getnum(st, 2), flag = script_getnum(st, 3);
	struct map_session_data *sd = NULL;
	struct sd_p_data *data;

	if (script_hasdata(st, 4) && script_isinttype(st, 4))
		id = script_getnum(st, 4);

	if (id && !(sd = map->id2sd(id))){
		ShowWarning("pc_block_si: Cannot Find Player with ID %d.\n", id);
		return false;
	} else {
		sd = script->rid2sd(st);
		if (sd == NULL) {
			ShowWarning("pc_block_si: Cannot Attach Player\n");
			return false;
		}
	}

	switch (type) {
		case 1:
			eShowDebug("BlockedMove\n");
			sd->block_action.move = flag;
			break;
		case 2:
			data = pdb_search(sd, true);
			eShowDebug("OnlyWalk\n");
			data->flag.only_walk = flag;
			break;
	}

	return true;
}

/**
 * @bg_hpsp_time
 * Reduces HP/SP Continuously in (time) ms.
 * @param 1 : HP to reduce
 * @param 2 : SP to reduce
 * @param 3 : time in ms
 * @param 4 : Set BG Team
 **/
BUILDIN(bg_hpsp_time)
{
	int hp = script_getnum(st, 2);
	int sp = script_getnum(st, 3);
	int time = script_getnum(st, 4);
	struct map_session_data *sd;
	int *bg_time, *bg_value, *bg_tid, *bg_t; 
	sd = script->rid2sd(st);
	if (sd == NULL) {
		ShowWarning("bg_hpsp_time: Player Not Attached\n");
		script_pushint(st, 0);
		return false;
	}

	if (time < 0) {
		ShowWarning("bg_hpsp_time: Invalid Time(%d)\n", time);
		script_pushint(st, 0);
		return false;
	}

	if (hp > 0) {	// HP
		bg_value = GET_VARIABLE_SIZE(sd, BG_HP_VALUE, true, int);
		bg_time = GET_VARIABLE_SIZE(sd, BG_HP_RATE, true, int);
		bg_tid = GET_VARIABLE_SIZE(sd, BG_HP_TID, true, int);
		if (*bg_tid != INVALID_TIMER) {
			ebg_clear_hpsp_timer(sd, EBG_HP_TIME);
		}

		*bg_tid = timer->add_interval(timer->gettick() + time, bg_hp_loss_function, sd->bl.id, 1, time);
		*bg_value = hp;
		*bg_time = time;
	}
	if (sp > 0) {
		bg_value = GET_VARIABLE_SIZE(sd, BG_SP_VALUE, true, int);
		bg_time = GET_VARIABLE_SIZE(sd, BG_SP_RATE, true, int);
		bg_tid = GET_VARIABLE_SIZE(sd, BG_SP_TID, true, int);
		if (*bg_tid != INVALID_TIMER) {
			ebg_clear_hpsp_timer(sd, EBG_SP_TIME);
		}
		
		*bg_tid = timer->add_interval(timer->gettick() + time, bg_sp_loss_function, sd->bl.id, 2, time);
		*bg_value = sp;
		*bg_time = time;
		
	}
	if (script_hasdata(st, 5) && script_isinttype(st, 5)){
		bg_t = GET_VARIABLE_SIZE(sd, BG_TEAM, true, int);
		*bg_t = script_getnum(st, 5);
	}
	
	script_pushint(st, 1);
	return true;
}

/**
 * @bg_hpsp_time_del
 * Delete's HP/SP Timer
 * @param 1 : Type (1 = Hp, 2 = Sp), bitwise
 * @see bg_hpsp_time
 **/
BUILDIN(bg_hpsp_time_del)
{
	struct map_session_data *sd;
	int type = script_getnum(st, 2);	// 1=HP , 2=SP

	sd = script->rid2sd(st);
	
	return ebg_clear_hpsp_timer(sd, type);
}

/**
 * @bg_hpsp_time_extra
 * Display numerous extra things
 * @param 1 : Effect to show
 * @param 2 : Emotion to show
 * @param 3 : ViewPoint Number
 * @param 4 : ViewPoint Color
 * @see bg_hpsp_time
 **/
BUILDIN(bg_hpsp_time_extra)
{
	struct map_session_data *sd = NULL;
	struct sd_p_data *data;
	int effect = script_getnum(st, 2);
	int emotion = script_getnum(st, 3);
	int point_number = script_getnum(st, 4);
	int color = script_getnum(st, 5);

	sd = script->rid2sd(st);
	if (sd == NULL) {
		ShowWarning("bg_hpsp_time_extra: Player Not Attached.\n");
		script_pushint(st, 0);
		return false;
	}
	
	data = pdb_search(sd, true);
	
	if (data->save.extra == NULL) {
		CREATE(data->save.extra, struct bg_loss_extra, 1);
		data->save.extra->npcs = NULL;
	}
	data->save.extra->effect.id = effect;
	data->save.extra->emotion.id = emotion;
	data->save.extra->viewpoint.id = point_number;
	data->save.extra->viewpoint.color = color;
	script_pushint(st, 1);
	return true;
}

/**
 * @viewpointmap
 * Show's Location to all Players in the map.
 * @param 1 : MapName
 * @param 2 : Type
 * @param 3 : x
 * @param 4 : y
 * @param 5 : id
 * @param 6 : color
 **/
BUILDIN(viewpointmap)
{
	int type, x, y, id, color, m;
	const char *map_name;

	map_name = script_getstr(st, 2);
	if (!(m = map->mapname2mapid(map_name))) {
		ShowWarning("viewpointmap: Map '%s' Not Found.\n", map_name);
		return false;
	}

	type = script_getnum(st, 3);
	x = script_getnum(st, 4);
	y = script_getnum(st, 5);
	id = script_getnum(st, 6);
	color = script_getnum(st, 7);

	map->foreachinmap(viewpointmap_sub, m, BL_PC, st->oid, type, x, y, id, color);
	return true;
}

/**
 * @bg_reward
 * Gives Reward to Specific BG Team
 * @param 1 : Battleground ID
 * @param 2 : ItemID
 * @param 3 : Item Amount
 * @param 4 : Kafra Points
 * @param 5 : QuestID
 * @param 6 : Variable
 * @param 7 : Value to add in variable
 * @param 8 : BG Type (BGT_ constant)
 * @param 9 : BG Status (WON/LOSS/TIE)
 **/
BUILDIN(bg_reward)
{
	int bg_id = script_getnum(st, 2);
	int nameid = script_getnum(st, 3);
	int amount = script_getnum(st, 4);
	int kafrapoints = script_getnum(st, 5);
	int quest_id = script_getnum(st, 6);
	const char *var = script_getstr(st, 7);
	int add_value = script_getnum(st, 8);
	int bg_a_type = script_getnum(st, 9);
	int bg_win_loss = script_getnum(st, 10);

	bg_team_rewards(bg_id, nameid, amount, kafrapoints, quest_id, var, add_value, bg_a_type, bg_win_loss);
	return true;
}

/**
 * @bg_addfame
 * Adds Regular/Ranked Points 
 * @param 1 : PlayerID(Optional)
 **/
BUILDIN(bg_addfame)
{
	struct map_session_data *sd = NULL;	
	int i, flag = mapreg->readreg(script->add_str("$BGRanked"));	/// 1 = Ranked BG, 0 = NormalBG
	struct battleground_data *bgd;	/// BattleGround Info
	
	if (script_hasdata(st, 2) && script_isinttype(st, 2))
		sd = map->id2sd(script_getnum(st, 2));
	else
		sd = script->rid2sd(st);
		
	if (sd == NULL) {
		ShowWarning("bg_addfame: Player Not Attached.\n");
		return false;
	}
	if (sd->bg_id == 0 || (bgd = bg->team_search(sd->bg_id)) == NULL) {
		ShowWarning("bg_addfame: BattleGround Team not found(%u).\n", sd->bg_id);
		return false;
	}
		
	ARR_FIND(0, MAX_BG_MEMBERS, i, (bgd->members[i].sd == sd));
	if (i >= MAX_BG_MEMBERS) {
		ShowWarning("bg_addfame: Player not in bg.\n");
		return false;
	}
	
	pc_addpoint_bg(sd, 1, flag, false, 0);
	return true;
}

/**
 * @bg_addpoints
 * Grant's Various types of points to map/area.
 * @param 1 : BattleGroundID
 * @param 2 : Map Name
 * @param 3 : x0
 * @param 4 : y0
 * @param 5 : x1
 * @param 6 : y1
 * @param 7 : Type (See all Constants)
 * @param 8 : Value (Value to add in Type)
 * @param 9 : Fame to Add
 * Parameter 3-6 can be completely omitted,
 * if points are to be given to whole map.
 **/
BUILDIN(bg_addpoints)
{
	#define add_point()	\
		point = GET_VARIABLE_SIZE(sd, type, true, int);	\
		eShowDebug("Points: %d, Value:%d", *point, value);	\
		if (point == NULL){	\
			ShowError("Type Not Found(%d)\n", type);	\
			return false;	\
		}	\
		if (*point < 0)	\
			*point = 0;	\
		else	\
			*point = (*point)+value;	\
		pc_addpoint_bg(sd, fame, flag, false, 0)
	struct map_session_data *sd = NULL;
	struct battleground_data *bgd;  ///< BattleGround Info
	int type;                       ///< Type of points
	int i, value, flag, fame;
	int *point;
	
	const char *mapname;
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0, bg_id, m = 0;
	bool area = false;


	if (script_hasdata(st, 6) && script_isinttype(st, 6)) {
		area = true;
		bg_id = script_getnum(st, 2);
		mapname = script_getstr(st,3);
		x0 = script_getnum(st, 4);
		y0 = script_getnum(st, 5);
		x1 = script_getnum(st, 6);
		y1 = script_getnum(st, 7);
		type = script_getnum(st, 8);
		value = script_getnum(st, 9);
		fame = script_getnum(st, 10);
		
		if ((m = map->mapname2mapid(mapname)) < 0) {
			ShowWarning("bg_addpoints: No Valid Map '%s' found.\n",mapname);
			return false;
		}
		
	}
	if (!area) {
		type = script_getnum(st, 2);	///< Refer to constants.
		value = script_getnum(st, 3);	///< Value to Add
		fame = script_getnum(st, 4);
		if (script_hasdata(st, 5) && script_isinttype(st, 5))
			sd = map->id2sd(script_getnum(st, 5));
		else
			sd = script->rid2sd(st);
		if (sd == NULL) {
			ShowWarning("bg_addpoints: Player Not Attached.\n");
			return false;
		}
		bg_id = sd->bg_id;
	}
	
	if (!bg_id || (bgd = bg->team_search(bg_id)) == NULL) {
		ShowWarning("bg_addpoints: Battleground ID is not valid(%d).\n", bg_id);
		return false;
	}
	flag = mapreg->readreg(script->add_str("$BGRanked"));	//1=Ranked BG, 0=NormalBG
	
	if (area) {
		for (i = 0; i < MAX_BG_MEMBERS; i++) {
			if ((sd = bgd->members[i].sd) == NULL)
				continue;
			if (sd->bl.m != m || sd->bl.x < x0 || sd->bl.y < y0 || sd->bl.x > x1 || sd->bl.y > y1)
				continue;
			add_point();
		}
	} else {
		ARR_FIND(0, MAX_BG_MEMBERS, i, (bgd->members[i].sd == sd));
		if (i >= MAX_BG_MEMBERS)
			return false;
		add_point();
	}
	
#undef add_point
	return true;
}

/**
 * @map_exists
 * Check's if map exists or not
 * @param 1 : MapName
 * @return 'MapId' if map exists, 0 if map doesn't exist
 **/
BUILDIN(map_exists)
{
	const char *map_name = script_getstr(st, 2);
	script_pushint(st, (int)mapname2id(map_name));
	return true;
}

/**
 * @bg_change_team
 * Changes the BG Team of Player
 * @param 1 : TeamID
 * @param 2 : GuildID
 **/
BUILDIN(bg_change_team)
{
	struct map_session_data *sd = script->rid2sd(st);
	int team_add = script_getnum(st, 2);
	int guild_id = script_getnum(st, 3);
	if (sd == NULL || !sd->bg_id) {
		ShowWarning("bg_change_team: Player Not Attached/BG_ID not found.\n");
		return false;
	}

	bg_e_change_team(sd, team_add, guild_id);
	return true;
}

/**
 * @bg_e_join_team
 * Makes player Join the BG Team
 * @param 1 : TeamID
 * @param 2 : GuildID
 * @param 3 : Player ID (Optional)
 * @return BG_ID if Successfully joined, 0 if cannot join
 **/
BUILDIN(bg_e_join_team)
{
	struct map_session_data *sd;
	int team_id = script_getnum(st, 2);
	int guild_id = script_getnum(st, 3);

	if (script_hasdata(st, 4))
		sd = map->id2sd(script_getnum(st, 4));
	else
		sd = script->rid2sd(st);
	eShowDebug("bg_e_join_team: %d\n",sd->status.char_id);
	if (sd == NULL)
		script_pushint(st, 0);
	else
		script_pushint(st,bg_e_team_join(team_id, sd, guild_id));

	return true;
}

/**
 * @eBG_script_bg_end
 * End's the BG and Saves All the Data
 * @param sd Player Data
 * @param bg_type BG Type, which was ongoing.
 **/
void eBG_script_bg_end(struct map_session_data *sd, int bg_type)
{
	bg_save_all(sd, bg_type);
	bg_save_all(sd, BGT_COMMON);
	/* Set Ranking Match to 0 **/
	eBG_turnOff(sd);
	return;
}

/**
 * @bg_end
 * End's the BG and Saves All the Data
 * @param 1 : BG Type
 * @return 0 if player not found, else 1
 **/
BUILDIN(bg_end)
{
	int bg_type = script_getnum(st,2);
	struct map_session_data *sd;
	sd = script->rid2sd(st);
	if (sd == NULL){
		script_pushint(st,0);
		return false;
	}
	eBG_script_bg_end(sd, bg_type);
	script_pushint(st, 1);
	return true;
}

/**
 * @bg_reset_rank_stat
 * Reset's Ranked Games Stats.
 * @return 0 if player/data not found, else 1
 **/
BUILDIN(bg_reset_rank_stat)
{
	struct sd_p_data *data;
	struct map_session_data *sd;
	sd = script->rid2sd(st);
	if (sd == NULL) {
		script_pushint(st,0);
		return false;
	}
	data = pdb_search(sd, false);
	if (data == NULL){
		script_pushint(st, 0);
		return false;
	}
	data->esd->bg.ranked_games = 0;
	script_pushint(st,1);
	return true;
}

/**
 * @on_ebg
 * Checks eBG Status of Player
 * @return 0 if player not found/Player not on eBG, else 1
 **/
BUILDIN(on_ebg)
{
	struct sd_p_data *data;
	struct map_session_data *sd;
	sd = script->rid2sd(st);
	if (sd == NULL) {
		script_pushint(st, 0);
		return false;
	}
	data = pdb_search(sd, false);
	if (data && data->eBG)
		script_pushint(st, 1);
	else
		script_pushint(st, 0);
	return true;
}

/**
 * @bg_logincheck
 * Checks number of player with same IP/Mac in a map
 * @param 1 : MapName
 * @return number of players
 **/
BUILDIN(bg_logincheck)
{
	struct map_session_data *sd;
	const char* map_name = script_getstr(st,2);
	int c;
	sd = script->rid2sd(st);
	if (sd == NULL) {
		script_pushint(st,0);
		return false;
	}
	c = bg_ipcheck(sd, false, map_name);
	script_pushint(st, c);
	return true;
}

/**
 * @bg_mapwarp
 * Warps All Player from map to savepoint
 * @param 1 : MapName
 **/
BUILDIN(bg_mapwarp)
{
	int m;
	const char *mapname;
	mapname = script_getstr(st,2);

	if ((m=map->mapname2mapid(mapname)) < 0)
		return true;

	map->foreachinmap(bg_mapwarp_sub, m, BL_PC);

	return true;
}

/**
 * @bg_immunemob
 * Makes Monster immune to attacks(takes only 1 atk)
 * @param 1 : GID
 * @param 2 : Activate(0 = Deactivate)
 **/
BUILDIN(bg_immunemob)
{
	struct mob_extra_data *data;
	struct mob_data *md;
	struct block_list *bl;
	int id = script_getnum(st,2);
	int activate = script_getnum(st,3);
	if (id == 0 || (bl = map->id2bl(id)) == NULL || bl->type != BL_MOB) {
		ShowWarning("bg_immunemob: Monster with id(%d) Not found.\n", id);
		return false;
	}

	md = BL_CAST(BL_MOB, bl);
	data = mob_e_search(md, true);
	if (activate) {
		data->immunity = true;
	} else {
		data->immunity = false;
	}
	
	return true;
}

/**
 * @bg_mob_move
 * Makes Monster Im/movable
 * @param 1 : GID
 * @param 2 : Activate(0 = Deactivate)
 **/
BUILDIN(bg_mob_move)
{
	struct mob_extra_data *data;
	struct mob_data *md;
	struct block_list *bl;
	int id = script_getnum(st,2);
	int activate = script_getnum(st,3);
	if (id == 0 || (bl = map->id2bl(id)) == NULL || bl->type != BL_MOB) {
		ShowWarning("bg_mob_move: Monster with id(%d) Not found.\n", id);
		return false;
	}

	md = BL_CAST(BL_MOB, bl);
	data = mob_e_search(md, true);
	if (activate) {
		data->can_move = true;
	} else {
		data->can_move = false;
	}
	
	return true;
}

/**
 * @bg_revealmob
 * Reveals the Monster in viewpoint
 * @param 1 : GID
 * @param 2 : Flag
 * @param 3 : Color
 **/
BUILDIN(bg_revealmob)
{
	struct block_list *mbl;
	int id = script_getnum(st,2),
		flag = script_getnum(st,3),
		color = script_getnum(st,4);
	
	if (id == 0 || (mbl = map->id2bl(id)) == NULL || mbl->type != BL_MOB)
		return false;
	map->foreachinmap(viewpointmap_sub, mbl->m, BL_PC, st->oid, flag, mbl->x, mbl->y, mbl->id, color);
	return true;
}

BUILDIN(bg_timer_start)
{
	int timer_ = script_getnum(st,2);
	int interval = script_getnum(st,3);
	short random_timer[2] = {0, 0};
	int bg_timer_uid;
	int op = 0;
	
	if (script_hasdata(st, 4) && script_isinttype(st, 4)){
		if (timer_) {
			ShowWarning("bg_timer_start: timer_(%d) provided along with range",timer_);
			script_pushint(st,0);
			return true;
		}
		random_timer[0] = script_getnum(st, 4);
		random_timer[1] = script_getnum(st, 5);
		if (random_timer[1] < random_timer[0])
			swap(random_timer[0], random_timer[1]);
		op = BG_T_O_SELFDELETE;
	}
	
	if (interval < 0 || interval > 1) {
		ShowWarning("bg_timer_start: Interval %d should be between 0-1",interval);
		script_pushint(st,0);
		return true;
	}
	if (interval == 0)
		op |= BG_T_O_SELFDELETE;
	
	bg_timer_uid = bg_timer_create(timer_, interval, st->oid, 0, op, true, NULL, random_timer);
	
	script_pushint(st, bg_timer_uid);
	return true;
}

BUILDIN(bg_areaheal_timer)
{
	int uid = script_getnum(st,2), m,add_op=0;
	const char *mapname = script_getstr(st,3);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (script_hasdata(st, 10) && script_isinttype(st, 10))
		add_op = script_getnum(st, 10);
	
	if (tdb == NULL){
		ShowError("bg_areaheal_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (tdb->operations&BG_T_O_AREAHEALER){
		ShowError("bg_areaheal_timer: Timer with UID(%d) Already have AreaHealer.\n",uid);
		return true;
	}
	
	if( (m = map->mapname2mapid(mapname)) < 0){
		ShowWarning("bg_areaheal_timer: Map \"%s\" Doesn't Exist.\n",mapname);
		return true;
	}
	if (tdb->areaheal == NULL)
		CREATE(tdb->areaheal, struct bg_areaheal_struct, 1);
	if (!add_op)
		tdb->operations |= BG_T_O_AREAHEALER;
	tdb->areaheal->x1 = script_getnum(st, 4);
	tdb->areaheal->y1 = script_getnum(st, 5);
	tdb->areaheal->x2 = script_getnum(st, 6);
	tdb->areaheal->y2 = script_getnum(st, 7);
	tdb->areaheal->hp = script_getnum(st, 8);
	tdb->areaheal->sp = script_getnum(st, 9);
	tdb->areaheal->m = m;
	
	return true;
}

BUILDIN(bg_areawarp_timer)
{
	int uid = script_getnum(st,2), m, index, add_op = 0;
	const char *mapname = script_getstr(st,3), *str = script_getstr(st,8);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	int x2 = script_getnum(st,9), y2 = script_getnum(st,10);
	int x3 = 0, y3 = 0;
	
	if (script_hasdata(st,11) && script_hasdata(st,12)) { // Warp area to area
		if ((x3 = script_getnum(st,11)) < 0 || (y3 = script_getnum(st,12)) < 0) {
			x3 = 0;
			y3 = 0;
		} else if (x3 > 0 && y3 > 0) {
			// normalize x3/y3 coordinates
			if (x3 < x2)
				swap(x3, x2);
			if (y3 < y2)
				swap(y3, y2);
		}
		if (script_hasdata(st, 13) && script_isinttype(st, 13)) {
			add_op = script_getnum(st, 13);
		}
	} else if (script_hasdata(st, 11) && script_isinttype(st, 11)) {
		add_op = script_getnum(st, 11);
	}
	
	if (tdb == NULL) {
		ShowError("bg_areawarp_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (tdb->operations&BG_T_O_AREAWARP) {
		ShowError("bg_areawarp_timer: Timer with UID(%d) Already have AreaWarp.\n",uid);
		return true;
	}
	
	if ((m = map->mapname2mapid(mapname)) < 0) {
		ShowWarning("bg_areawarp_timer: Map \"%s\" Doesn't Exist.\n", mapname);
		return true;
	}
	if (strcmp(str, "Random") == 0)
		index = 0;
	else if (!(index = script->mapindexname2id(st, str)))
		return true;
	
	if (tdb->areawarp == NULL)
		CREATE(tdb->areawarp, struct bg_areawarp_struct, 1);
	if (!add_op)
		tdb->operations |= BG_T_O_AREAWARP;
	tdb->areawarp->x0 = script_getnum(st,4);
	tdb->areawarp->y0 = script_getnum(st,5);
	tdb->areawarp->x1 = script_getnum(st,6);
	tdb->areawarp->y1 = script_getnum(st,7);
	tdb->areawarp->x2 = x2;
	tdb->areawarp->y2 = y2;
	tdb->areawarp->x3 = x3;
	tdb->areawarp->y3 = y3;
	tdb->areawarp->m = m;
	tdb->areawarp->index = index;
	
	return true;
}

BUILDIN(bg_effect_timer)
{
	int uid = script_getnum(st,2), add_op = 0, index = 0;
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (script_hasdata(st, 4) && script_isinttype(st, 4))
		index = script_getnum(st, 4);
	
	if (script_hasdata(st, 5) && script_isinttype(st, 5))
		add_op = script_getnum(st, 5);
	
	if (tdb == NULL){
		ShowError("bg_effect_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	
	if (index >= BMAX_EFFECT){
		ShowError("bg_effect_timer: Invalid Index for Timer with UID(%d).\n",uid);
		return true;
	}
	
	if (tdb->operations&BG_T_O_SHOWEFFECT && tdb->effect[index]){
		ShowError("bg_effect_timer: Timer with UID(%d) Already have Effect.\n",uid);
		return true;
	}
	
	if (!add_op)
		tdb->operations |= BG_T_O_SHOWEFFECT;
	tdb->effect[index] = script_getnum(st,3);
	
	return true;
}

BUILDIN(bg_viewpoint_timer)
{
	int uid = script_getnum(st,2),add_op=0;
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	int m;
	const char *map_name = script_getstr(st,3);
	
	if (script_hasdata(st, 9) && script_isinttype(st, 9))
		add_op = script_getnum(st, 9);
	
	if (tdb == NULL){
		ShowError("bg_viewpoint_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (tdb->operations&BG_T_O_VIEWPOINTMAP){
		ShowError("bg_viewpoint_timer: Timer with UID(%d) Already have ViewPoint.\n",uid);
		return true;
	}
	
	if( (m = map->mapname2mapid(map_name)) < 0){
		ShowWarning("bg_viewpoint_timer: Map \"%s\" Doesn't Exist.\n",map_name);
		return true;
	}
	
	if (!add_op)
		tdb->operations |= BG_T_O_VIEWPOINTMAP;
	if (tdb->viewpointmap == NULL)
		CREATE(tdb->viewpointmap, struct bg_viewpoint_struct, 1);
	tdb->viewpointmap->m = m;
	tdb->viewpointmap->type = script_getnum(st,4);
	tdb->viewpointmap->x = script_getnum(st,5);
	tdb->viewpointmap->y = script_getnum(st,6);
	tdb->viewpointmap->id = script_getnum(st,7);
	tdb->viewpointmap->color = script_getnum(st,8);
	
	return true;
}

BUILDIN(bg_npc_cond_hpsp)
{
	int index = 0, index_at = 0;
	struct sd_p_data *data;
	struct bg_loss_extra *tdb;
	struct map_session_data *sd = script->rid2sd(st);
	
	if (sd == NULL) {
		eShowDebug("bg_npc_cond_hpsp: No Player Attached\n");
		return false;
	}
	data = pdb_search(sd, false);
	if (data == NULL){
		eShowDebug("bg_npc_cond_hpsp: Player with Null Data\n");
		return false;
	}
	if (data->save.extra == NULL) {
		CREATE(data->save.extra, struct bg_loss_extra, 1);
		data->save.extra->npcs = NULL;
	}
	tdb = data->save.extra;
	
	if (script_hasdata(st, 5) && script_isinttype(st, 5))
		index = script_getnum(st,5);
	
	if (script_hasdata(st, 6) && script_isinttype(st, 6))
		index_at = script_getnum(st,6);
	
	if (index < 0 || index >= BNPC_MAX_CONDITION){
		ShowError("bg_npc_cond_hpsp: Index(%d) Larger than limit(%d).\n",index,BNPC_MAX_CONDITION);
		return false;
	}
	
	if (index_at < 0 || index_at >= BEXTRA_MAX_VALUE){
		ShowError("bg_npc_cond_hpsp: Index(%d) Larger than limit(%d).\n",index_at,BEXTRA_MAX_VALUE);
		return false;
	}
	
	if (tdb->npcs == NULL)
		CREATE(tdb->npcs, struct bg_cond_struct, 1);
	sprintf(tdb->npcs->name,"%s",script_getstr(st,2));
	eShowDebug("NPC_NAME:%s\n",tdb->npcs->name);
	tdb->npcs->value[index] = script_getnum(st, 3);
	tdb->npcs->condition[index] = script_getnum(st, 4);
	tdb->npcs->checkat[index] = index_at;
	tdb->npcs->npc_or_var = true;
	
	return true;
}

BUILDIN(bg_player_cond_timer) {
	int uid = script_getnum(st,2),add_op=0,index=0,index_at=0;
	
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (script_hasdata(st, 8) && script_isinttype(st, 8))
		index = script_getnum(st, 8);
	
	if (script_hasdata(st, 9) && script_isinttype(st, 9))
		index_at = script_getnum(st, 9);
	
	if (script_hasdata(st, 10) && script_isinttype(st, 10))
		add_op = script_getnum(st, 10);
	
	if (index < 0 || index >= BNPC_MAX_CONDITION){
		ShowError("bg_player_cond_timer: Index(%d) Larger than limit(%d).\n",index,BNPC_MAX_CONDITION);
		return false;
	}
	
	if (index_at < 0 || index_at >= BEXTRA_MAX_VALUE){
		ShowError("bg_player_cond_timer: Index(%d) Larger than limit(%d).\n",index_at,BEXTRA_MAX_VALUE);
		return false;
	}
	
	if (tdb == NULL){
		ShowError("bg_player_cond_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (tdb->operations&BG_T_O_NPC_CONDITION && tdb->npcs->operations[index]!=0){
		ShowError("bg_player_cond_timer: Timer with UID(%d), Index(%d) Already have Condition.\n",uid,index);
		return true;
	}
	
	
	if (!add_op)
		tdb->operations |= BG_T_O_NPC_CONDITION;
	if (tdb->npcs == NULL)
		CREATE(tdb->npcs, struct bg_cond_struct, 1);
	sprintf(tdb->npcs->name,"%s",script_getstr(st,3));
	//tdb->npcs->name = script_getstr(st, 3);
	eShowDebug("NPC_NAME:%s\n",tdb->npcs->name);
	tdb->npcs->value[index] = script_getnum(st, 4);
	tdb->npcs->condition[index] = script_getnum(st, 5);
	tdb->npcs->operations[index] = script_getnum(st, 6);
	tdb->npcs->checkat[index] = index_at;
	tdb->npcs->npc_or_var = false;
	sprintf(tdb->npcs->var,"%s",script_getstr(st,7));
	
	return true;
}

BUILDIN(bg_npc_cond_timer) {
	int uid = script_getnum(st,2),add_op=0,index=0,index_at=0;
	
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (script_hasdata(st, 7) && script_isinttype(st, 7))
		index = script_getnum(st, 7);
	
	if (script_hasdata(st, 8) && script_isinttype(st, 8))
		index_at = script_getnum(st, 8);
	
	if (script_hasdata(st, 9) && script_isinttype(st, 9))
		add_op = script_getnum(st, 9);
	
	if (index < 0 || index >= BNPC_MAX_CONDITION){
		ShowError("bg_npc_cond_timer: Index(%d) Larger than limit(%d).\n",index,BNPC_MAX_CONDITION);
		return false;
	}
	
	if (index_at < 0 || index_at >= BEXTRA_MAX_VALUE){
		ShowError("bg_npc_cond_timer: Index(%d) Larger than limit(%d).\n",index_at,BEXTRA_MAX_VALUE);
		return false;
	}
	
	if (tdb == NULL){
		ShowError("bg_npc_cond_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (tdb->operations&BG_T_O_NPC_CONDITION && tdb->npcs->operations[index]!=0){
		ShowError("bg_npc_cond_timer: Timer with UID(%d), Index(%d) Already have Condition.\n",uid,index);
		return true;
	}
	
	if (!add_op)
		tdb->operations |= BG_T_O_NPC_CONDITION;
	if (tdb->npcs == NULL)
		CREATE(tdb->npcs, struct bg_cond_struct, 1);
	sprintf(tdb->npcs->name,"%s",script_getstr(st,3));
	//tdb->npcs->name = script_getstr(st, 3);
	eShowDebug("NPC_NAME:%s\n",tdb->npcs->name);
	tdb->npcs->value[index] = script_getnum(st, 4);
	tdb->npcs->condition[index] = script_getnum(st, 5);
	tdb->npcs->operations[index] = script_getnum(st, 6);
	tdb->npcs->checkat[index] = index_at;
	tdb->npcs->npc_or_var = true;
	
	return true;
}

BUILDIN(bg_timer_child){
	int uid = script_getnum(st,2);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (tdb == NULL){
		ShowError("bg_timer_child: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (tdb->operations&BG_T_O_ADDTIMER){
		ShowError("bg_timer_child: Timer with UID(%d) Already have ChildTimer.\n",uid);
		return true;
	}
	
	tdb->operations |= BG_T_O_ADDTIMER;
	if (tdb->timer == NULL)
		CREATE(tdb->timer, struct bg_timer_struct, 1);
	tdb->timer->timer_ = script_getnum(st, 3);
	tdb->timer->operations = script_getnum(st, 4);
	
	return true;
}

BUILDIN(bg_operations_timer) {
	int uid = script_getnum(st,2), operations = script_getnum(st,3),replace = 0;
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (script_hasdata(st, 4) && script_isinttype(st, 4))
		replace = script_getnum(st, 4);
	
	if (tdb == NULL){
		ShowError("bg_operations_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (!replace && ((tdb->operations&operations) == operations)){
		ShowError("bg_operations_timer: Timer with UID(%d) Already have Operation:%d.\n",uid,operations);
		return true;
	}
	
	if (replace)
		tdb->operations = operations;
	else
		tdb->operations |= operations;
	
	return true;
}

BUILDIN(bg_timer_stop)
{
	int uid = script_getnum(st,2);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (tdb == NULL){
		ShowError("bg_timer_stop: Timer with UID(%d) not found.\n",uid);
		script_pushint(st,0);
		return true;
	}
#ifdef TIMER_LOG
	insert_timerlog(uid,"TimerStop");
#endif
	bg_timer_free(tdb, uid, 0xF);

	return true;
}

BUILDIN(bg_timer_exists) {
	int uid = script_getnum(st,2);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (tdb == NULL){
		script_pushint(st,0);
	} else {
		script_pushint(st,1);
	}
	return true;
}

BUILDIN(bg_npc_data)
{
	struct npc_extra_data *data;
	struct npc_data* nd;
	int index = 0;
	
	if (script_hasdata(st, 3) && script_isinttype(st, 3)){
		index = script_getnum(st,3);
	}
	
	if (index < 0 || index >= BEXTRA_MAX_VALUE){
		ShowError("bg_npc_data: Index(%d) Larger than limit(%d).\n",index,BEXTRA_MAX_VALUE);
		return false;
	}
	
	if (script_hasdata(st, 4) && script_isstringtype(st, 4)){
		nd = npc->name2id(script_getstr(st,4));
	} else {
		nd = map->id2nd(st->oid);
	}
	
	if (nd == NULL) {
		ShowError("bg_npc_data: NPC Name Not Found.\n");
		return false;
	}
	
	data = npc_e_search(nd, true);
	data->value[index] = script_getnum(st, 2);
	return true;
}

BUILDIN(bg_get_npc_data)
{
	struct npc_extra_data *data;
	struct npc_data* nd;
	int index = 0;
	if (script_hasdata(st, 2) && script_isinttype(st, 2)){
		index = script_getnum(st,2);
	}
	
	if (index < 0 || index >= BEXTRA_MAX_VALUE){
		ShowError("bg_get_npc_data: Index(%d) Larger than limit(%d).\n",index,BEXTRA_MAX_VALUE);
		script_pushint(st, 0);
		return false;
	}
	
	if (script_hasdata(st, 3) && script_isstringtype(st, 3)){
		nd = npc->name2id(script_getstr(st,3));
	} else {
		nd = map->id2nd(st->oid);
	}
	
	data = npc_e_search(nd, false);
	if (data == NULL)
		script_pushint(st, 0);
	else
		script_pushint(st, data->value[index]);
	return true;
}

BUILDIN(bg_areaitem)
{
	struct item_data *data;
	const char *mapname = script_getstr(st,2);
	int x = script_getnum(st, 3);
	int y = script_getnum(st, 4);
	int nameid = script_getnum(st, 5);
	int amount = script_getnum(st, 6);
	int m;
	
	if( (m = map->mapname2mapid(mapname)) < 0 )
		return false;
	
	if( (data = itemdb->search(nameid)) == NULL )
		return false;

	area_flooritem( m, x, y, nameid, amount);
	return true;
}

BUILDIN(bg_team_reveal){
	struct battleground_data *bgd;
	struct bg_extra_info *bg_data_t;
	int id = script_getnum(st, 2);	//bg_id
	
	if (!id || (bgd = bg->team_search(id)) == NULL) {
		script_pushint(st, 0);
		return false;
	}
	
	bg_data_t = bg_extra_create(bgd, false);
	if (bg_data_t == NULL){
		ShowWarning("bg_team_reveal: bg_data_t is NULL, BG_ID: %d\n",id);
		script_pushint(st, 0);
		return false;
	}
	
	bg_data_t->reveal_pos = true; // Reveal Position Mode
	script_pushint(st, 1);
	return true;
}

BUILDIN(bg_team_score){
	struct battleground_data *bgd;
	struct bg_extra_info *bg_data_t;
	int id = script_getnum(st,2);
	int points = script_getnum(st,3);

	if ((bgd = bg->team_search(id)) != NULL) {
		bg_data_t = bg_extra_create(bgd, false);
		if (bg_data_t == NULL){
			ShowWarning("bg_team_score: bg_data_t is NULL, BG_ID: %d\n",id);
			script_pushint(st, 0);
			return false;
		}
		bg_data_t->points = points;
		bg_updatescore_team(bgd);
		script_pushint(st, 1);
	}
	script_pushint(st, 0);

	return true;
}

BUILDIN(bg_insert_npc_name) {
	int uid = script_getnum(st,2);
	
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (tdb == NULL){
		ShowError("bg_insert_npc_name: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	
	if (tdb->npcs == NULL)
		CREATE(tdb->npcs, struct bg_cond_struct, 1);
	sprintf(tdb->npcs->name,"%s",script_getstr(st,3));
	
	return true;
}

BUILDIN(bg_insert_event_name) {
	int uid = script_getnum(st,2);
	
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (tdb == NULL){
		ShowError("bg_insert_event_name: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	
	if (tdb->npcs == NULL)
		CREATE(tdb->npcs, struct bg_cond_struct, 1);
	sprintf(tdb->npcs->event,"%s",script_getstr(st,3));
	
	return true;
}

BUILDIN(bg_perform_arithmetic) {
	int uid = script_getnum(st,2);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	int index_at = 0,op = script_getnum(st, 5), value = script_getnum(st, 4);
	int add_op = 0;
	
	if (op < EBG_A_ADD || op >= EBG_A_MAX){
		ShowError("bg_perform_arithmetic: Invalid Operation for Timer with UID(%d).\n", uid);
		return true;
	}
	
	if (script_hasdata(st, 6) && script_isinttype(st, 6)) {
		index_at = script_getnum(st, 6);
		if (index_at < 0 || index_at >= BEXTRA_MAX_VALUE){
			ShowError("bg_perform_arithmetic: Invalid IndexRange(%d)- Maximum is (%d) for Timer with UID(%d).\n", index_at, BEXTRA_MAX_VALUE, uid);
			return true;
		}
	}
	
	if (script_hasdata(st, 7) && script_isinttype(st, 7)) 
		add_op = script_getnum(st, 7);
	
	if (tdb == NULL){
		ShowError("bg_perform_arithmetic: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	if (!add_op)
		tdb->operations |= BG_T_O_ARITHMETIC_OP;
	if (tdb->arith == NULL)
		CREATE(tdb->arith, struct bg_arith_struct, 1);
	sprintf(tdb->arith->name,"%s",script_getstr(st,3));
	tdb->arith->value = value;
	tdb->arith->operation = op;
	tdb->arith->index_at = index_at;
	
	return true;
}

BUILDIN(bg_mapannounce_timer) {
	int uid = script_getnum(st,2),add_op=0;
	int flag;
	int announce = 1;
	int16 m;
	const char* mapname;
	
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	mapname = script_getstr(st, 3);
	flag = script_getnum(st, 5);
	if (script_hasdata(st, 7) && script_isinttype(st, 7))
		announce = script_getnum(st, 7);
	
	if (script_hasdata(st, 8) && script_isinttype(st, 8))
		add_op = script_getnum(st, 8);
		
	if ((m = map->mapname2mapid(mapname)) < 0){
		ShowError("bg_mapannounce_timer: Non Existing mapname '%s' for Timer with UID(%d) .\n",mapname,uid);
		return true;
	}
		
	if (tdb == NULL){
		ShowError("bg_mapannounce_timer: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	
	if (tdb->operations&BG_T_O_MAPANNOUNCE){
		ShowError("bg_mapannounce_timer: Timer with UID(%d) Already have Condition.\n",uid);
		return true;
	}
	
	if (!add_op)
		tdb->operations |= BG_T_O_MAPANNOUNCE;
	if (tdb->mapannounce == NULL)
		CREATE(tdb->mapannounce, struct bg_mapannounce_struct, 1);
	sprintf(tdb->mapannounce->mes,"%s",script_getstr(st, 4));
	sprintf(tdb->mapannounce->fontColor,"%s",script_getstr(st, 6));
	//tdb->mapannounce->mes = mes;
	//tdb->mapannounce->fontColor = fontColor;
	tdb->mapannounce->m = m;
	tdb->mapannounce->flag = flag;
	tdb->mapannounce->announce = (announce)?true:false;
	
	return true;
}


BUILDIN(bg_info) {
	struct map_session_data* sd = script->rid2sd(st);
	int bg_id = script_getnum(st, 2),
		type = script_getnum(st, 3);
	int bg_index;
	bool no_data = false;
	struct guild *g = NULL;
#ifndef VIRT_GUILD
	struct sd_p_data *sd_data = pdb_search(sd, false);
	if (sd_data == NULL || sd_data->g == NULL) {
		ShowError("bg_info: Invalid Guild (bg_id:%d, type:%d).\n", bg_id, type);
		no_data = true;
	} else {
		g = sd_data->g;
	}
#else
	struct bg_extra_info *bg_data_t;
	struct battleground_data *bgd;
	if ((bgd = bg->team_search(bg_id)) != NULL) {
		bg_data_t = bg_extra_create(bgd, false);
		if (bg_data_t == NULL) {
			ShowError("bg_info: bg_data not found for bg_id:%d type:%d.\n", bg_id, type);
			no_data = true;
		} else {
			g = bg_data_t->g;
		}
	} else {
		no_data = true;
	}
#endif

	switch(type) {
		case 0: 
			script_pushconststr(st, ((no_data)? "NoGData" : ((g != NULL) ? g->name : "NoGName")));
			break;
		case 1:
			script_pushconststr(st, ((no_data)? "NoMData" : ((g != NULL) ? g->master : "NoMName")));
			break;
		case 2:
			if (no_data) {
				script_pushint(st, bg_colors[3]);	// Send White Color
				return true;
			}
			bg_index = GET_ORIG_GUILD_ID(g->guild_id);

			script_pushint(st, bg_colors[bg_index]);
			break;
		default:
			ShowError("script:bg_info: Unknown Type %d\n", type);
			script_pushint(st, 0);
			break;
	}

	return true;
}

BUILDIN(bg_timer_link){
	int uid = script_getnum(st,2);
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	int operation1 = script_getnum(st, 3);
	int operation2 = script_getnum(st, 4);	// Operation2 will run only after operation1 Runs(Operation1 Can execute anytime)
	if (tdb == NULL){
		ShowError("bg_timer_link: Timer with UID(%d) Not Found.\n",uid);
		return true;
	}
	tdb->link.operation_independent = operation1;
	tdb->link.operation_dependent = operation2;
	tdb->link.retVal = 0;
	return true;
}

BUILDIN(bg_announce) {
#define scrpt_data(num,default) (script_hasdata(st,(num)) ? script_getnum(st,(num)) : (default))
	const char *mes = script_getstr(st, 2);
	int flag = script_getnum(st, 3);
	const char *fontColor = script_hasdata(st, 4) ? script_getstr(st,4) : NULL;
	int fontType = scrpt_data(5, 0x190); // default fontType (FW_NORMAL)
	int fontSize = scrpt_data(6, 12); // default fontSize
	int fontAlign = scrpt_data(7, 0); // default fontAlign
	int fontY = scrpt_data(8, 0); // default fontY
	ebg_target target = SELF_EBG;
	struct block_list *bl = NULL; // use NPC as broadcast source
	if (flag&BC_NPC) {
		// If bc_npc flag is set, use NPC as broadcast source
		bl = map->id2bl(st->oid);
	} else {
		struct map_session_data *sd = script->rid2sd(st);
		if (sd != NULL)
			bl = &sd->bl;
	}
	if (bl == NULL)
		return true;

	if (flag & BC_EBG) {
		target = ANNOUNCE_EBG;
	} else if (flag & BC_TEAMEBG) {
		target = CLIENT_EBG;
	} else if (flag & BC_SELFEBG) {
		target = SELF_EBG;
	}

	if (fontColor)
		ebg_broadcast2(bl, mes, (int)strlen(mes)+1, (unsigned int)strtoul(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY, target);
	else
		ebg_broadcast2(bl, mes, (int)strlen(mes)+1, 0xFF0000, 0x190, 20, 0, 0, target);
#undef scrpt_data
	return true;
}

BUILDIN(bg_script_timer) {
	int uid = script_getnum(st, 2), add_op = 0;
	const char *bonus_script = script_getstr(st, 3);
	
	struct bg_timerdb_struct* tdb = bg_timer_search(uid);
	
	if (script_hasdata(st, 4) && script_isinttype(st, 4))
		add_op = script_getnum(st, 4);
		
	if (tdb == NULL){
		ShowError("bg_script_timer: Timer with UID(%d) Not Found.\n", uid);
		return true;
	}
	
	if (tdb->operations&BG_T_O_SCRIPT){
		ShowError("bg_script_timer: Timer with UID(%d) Already have Condition.\n", uid);
		return true;
	}
	
	if (add_op == 0)
		tdb->operations |= BG_T_O_SCRIPT;

	if (tdb->script != NULL) {
		ShowWarning("Free'd Current Script of Timer UID(%d)\n", uid);
		script->free_code(tdb->script);
	}
	tdb->script = script->parse(bonus_script, "", 0, 0, NULL);	
	
	return true;
}

/**
 * bg_get_guild_id(<GuildID>);
 * Converts Original Guild ID to eBG Guild ID.
 */
BUILDIN(bg_get_guild_id) {
	int gid = script_getnum(st, 2);
	script_pushint(st, GET_EBG_GUILD_ID(gid));
	return true;
}

/**
 * checkwall("wall_name");
 * Returns 1 if wall exist, else returns 0
 */
BUILDIN(checkwall) {
	const char *name = script_getstr(st, 2);

	if (strdb_get(map->iwall_db, name) == NULL) {
		script_pushint(st, 0);
		return true;
	}

	script_pushint(st, 1);
	return true;
}

/**
 * getebgitem(<nameid>, <quantity>, <type>)
 * Gets BG/WoE Bounded Items
 * type: MAP_IS_BG/MAP_IS_WOE
 */
BUILDIN(getebgitem)
{
	int nameid = script_getnum(st, 2),
		quantity = script_getnum(st, 3),
		type = script_getnum(st, 4);
	int id;
	struct item item_tmp;
	struct map_session_data *sd;

	sd = script->rid2sd(st);
	if (sd == NULL) // Player not attached!
		return true;

	if (!itemdb->exists(nameid)) {
		script_pushint(st,0);
		return true;
	}

	switch(type) {
		case MAP_IS_BG:
			id = bg_reserved_char_id;
			break;
		case MAP_IS_WOE:
			id = woe_reserved_char_id;
			break;
		case -1:	// Special for NonExclusive Items
			id = 0;
			break;
		default:
			script_pushint(st, 0);
			ShowError("getebgitem: Invalid Type(%d) For Item(%dx %d).\n", type, quantity, nameid);
			return true;
	}

	
	memset(&item_tmp, 0, sizeof(item_tmp));
	item_tmp.nameid = nameid;
	item_tmp.amount = quantity;
	item_tmp.identify = 1;
	if (id != 0) {
		item_tmp.card[0] = CARD0_CREATE;
		item_tmp.card[1] = 0;
		item_tmp.card[2] = id;
		item_tmp.card[3] = id >> 16;
	}
	if (pc->additem(sd, &item_tmp, quantity, LOG_TYPE_SCRIPT)) {
		// Failed to add item
		script_pushint(st,0);
		return true;
	}

	script_pushint(st,1);
	return true;
}

/**
 * getbgfame(fameType, top)
 * fameType:
 *  1 = Ranked BG
 *  2 = Regular BG
 * top: Top X Players
 */
BUILDIN(getbgfame)
{
	int fameType = script_getnum(st, 2),
		top = script_getnum(st, 3);
	int c = 0, i;
	if (fameType == 1) {
		for (i = 0; i < MAX_FAME_LIST && bgrank_fame_list[i].id && c < top; i++) {
			mapreg->setreg(reference_uid(script->add_variable("$@bg_fame_id"), c++), bgrank_fame_list[i].id);
		}
	} else if (fameType == 2) {
		for (i = 0; i < MAX_FAME_LIST && bg_fame_list[i].id && c < top; i++) {
			mapreg->setreg(reference_uid(script->add_variable("$@bg_fame_id"), c++), bg_fame_list[i].id);
		}
	}

	script_pushint(st, c);
	return true;
}

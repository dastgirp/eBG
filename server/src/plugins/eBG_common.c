#ifndef EBG_MAP
#include "eBG_common.h"
#endif

/**
* Clears All Memory that were allocated to player during his activeness in BG
* @param sd Map_Session_Data
* @param esdb ebg_save_data
* @param sd_data sd_p_data
*/
#ifdef CHAR_SERVER_SAVE
void bg_clear_char_data(struct ebg_save_data *esdb, struct sd_p_data *sd_data)
#else
void bg_clear_char_data(struct map_session_data *sd)
#endif
{
	/**
	* Frees the memory allocated to the structure
	* @param data structure of sd_p_data
	* @param bg_type Name of the structure in 'data'(sd_p_data)
	* @param bg_subtype Name of the structure in 'bg_type'
	* @param bg_subtype2 Name of the structure in 'bg_subtype'
	* @param free Free bg_subtype?
	*/
#define FREE_EBG_DATA(bg_type, bg_struct_type, bg_subtype, bg_subtype2, free) \
		if (bg_type != NULL) { \
			if ((bg_type)->bg_struct_type.bg_subtype != NULL) { \
				if ((bg_type)->bg_struct_type.bg_subtype->bg_subtype2 != NULL) { \
					aFree((((bg_type)->bg_struct_type).bg_subtype)->bg_subtype2); \
					(bg_type)->bg_struct_type.bg_subtype->bg_subtype2 = NULL;	\
				} \
				if (free) { \
					aFree((bg_type)->bg_struct_type.bg_subtype); \
					(bg_type)->bg_struct_type.bg_subtype = NULL; \
				} \
			} \
		}	

#ifndef CHAR_SERVER_SAVE
	struct sd_p_data *sd_data;
	if (sd == NULL) { // Invalid Data
		return;
	}

	sd_data = pdb_search(sd, false);
#endif

	if (
#ifdef CHAR_SERVER_SAVE
		esdb != NULL
#else
		sd_data != NULL
#endif
		)
	{
#ifndef CHAR_SERVER_SAVE
		struct ebg_save_data* esdb = sd_data->esd;
#endif
		if (esdb != NULL) {
			// Free BG Data
			FREE_EBG_DATA(esdb, bg, ctf, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, boss, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, ti, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, eos, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, td, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, sc, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, conq, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, rush, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, dom, bgm_status, true)
			//
			FREE_EBG_DATA(esdb, bg, common, bgm_status, true)
			FREE_EBG_DATA(esdb, bg, leader, bgm_status, true)
#ifdef EBG_RANKING
			// Rankings
			// Free BG Rank Data
			if (esdb->rank.bg != NULL) {
				FREE_EBG_DATA(esdb, rank, bg->rank, damage, false)
				FREE_EBG_DATA(esdb, rank, bg->rank, heal, false)
				FREE_EBG_DATA(esdb, rank, bg->rank, item, false)
				FREE_EBG_DATA(esdb, rank, bg->rank, skill, false)
				FREE_EBG_DATA(esdb, rank, bg->rank, misc, false)
				if (esdb->rank.bg->rank != NULL) {
					aFree(esdb->rank.bg->rank);
					esdb->rank.bg->rank = NULL;
				}
				if (esdb->rank.bg->damage != NULL) {
					aFree(esdb->rank.bg->damage);
					esdb->rank.bg->damage = NULL;
				}
				aFree(esdb->rank.bg);
				esdb->rank.bg = NULL;
			}
			// Free WoE Rank Data
			if (esdb->rank.woe != NULL) {
				FREE_EBG_DATA(esdb, rank, woe->rank, damage, false)
				FREE_EBG_DATA(esdb, rank, woe->rank, heal, false)
				FREE_EBG_DATA(esdb, rank, woe->rank, item, false)
				FREE_EBG_DATA(esdb, rank, woe->rank, skill, false)
				FREE_EBG_DATA(esdb, rank, woe->rank, misc, false)
				if (esdb->rank.woe->damage != NULL) {
					aFree(esdb->rank.woe->damage);
					esdb->rank.woe->damage = NULL;
				}
				if (esdb->rank.woe->misc != NULL) {
					aFree(esdb->rank.woe->misc);
					esdb->rank.woe->misc = NULL;
				}
				aFree(esdb->rank.woe->rank);
				esdb->rank.woe->rank = NULL;
				aFree(esdb->rank.woe);
				esdb->rank.woe = NULL;
			}
#endif
			aFree(esdb);
			esdb = NULL;
		}
		if (sd_data != NULL) {
			///< Save Functions 
			if (sd_data->save.extra != NULL) {
				if (sd_data->save.extra->npcs) {
					aFree(sd_data->save.extra->npcs);
					sd_data->save.extra->npcs = NULL;
				}
				aFree(sd_data->save.extra);
				sd_data->save.extra = NULL;
			}
			if (sd_data->save.sp_loss) {
				aFree(sd_data->save.sp_loss);
				sd_data->save.sp_loss = NULL;
			}
			if (sd_data->save.hp_loss) {
				aFree(sd_data->save.hp_loss);
				sd_data->save.hp_loss = NULL;
			}
		}
#ifndef CHAR_SERVER_SAVE
		// Remove from Memory.
		removeFromMSD(sd, 0);
#endif
	}
#undef FREE_EBG_DATA
	return;
}

/**
 * Sets the Win/Loss/Tie Count into buffer passed.
 * @method ebg_char_set_bgm_status_as_buf
 * @param  bytes                          total bytes, if greater than 8, set tie too.
 * @param  buf                            buffer
 * @param  offset                         current offset counter
 * @param  bgm_status                     address where actual data lies.
 */
void ebg_char_set_bgm_status_as_buf(int bits, unsigned char *buf, int *offset, struct bg_match_result *bgm_status)
{
	if (bgm_status != NULL) {
		WBUFL(buf, *offset+0) = bgm_status->win;
		WBUFL(buf, *offset+4) = bgm_status->loss;
		if (bits > 8) {
			WBUFL(buf, *offset+8) = bgm_status->tie;
		}
	} else {
		WBUFL(buf, *offset+0) = 0;
		WBUFL(buf, *offset+4) = 0;
		if (bits > 8) {
			WBUFL(buf, *offset+8) = 0;
		}
	}
	*offset += bits;
	return;
}

/**
 * Sets the Data into buffer to be passed through socket(Map<->Char)
 * @method set_ebg_buffer
 * @param  buf            buffer
 * @param  offset_        current offset
 * @param  type           save_type
 * @param  esdb           ebg_save_data
 */
void set_ebg_buffer(unsigned char *buf, int *offset_, int64 *type, struct ebg_save_data *esdb)
{
	int offset = *offset_;
	struct ebg_save_data_bg *esdb_bg = &(esdb->bg); ///< BG Data, declared for shorthand

/**
 * Checks if condition is met,
 * i.e it checks whether the data of that type should be passed.
 * @method CHECK_COND
 * @param  type       save_type
 * @param  type_check type to check
 * @param  condition  additional condition which should be satisfied too.
 * @return            [description]
 */
#define CHECK_COND(type, type_check, condition) \
					if (!(((type)&(type_check)) && condition)) { \
						type &= ~type_check; \
					} else 

#ifdef EBG_RANKING
#define CHECK_PTR(bg_type, bg_subtype) (esdb != NULL && esdb->rank.bg_type != NULL && esdb->rank.bg_type->rank != NULL && esdb->rank.bg_type->rank->bg_subtype != NULL)
#define CHECK_PTR2(bg_type, bg_subtype) (esdb != NULL && esdb->rank.bg_type != NULL && esdb->rank.bg_type->bg_subtype != NULL)
#define GET_PTR(bg_type, bg_subtype, bg_variable) esdb->rank.bg_type->rank->bg_subtype->bg_variable
#define GET_PTR2(bg_type, bg_subtype, bg_variable) esdb->rank.bg_type->bg_subtype->bg_variable

	///< Battleground Syncing Starts.
	CHECK_COND(*type, EBG_SAVE_BG_RANK_DMG, CHECK_PTR(bg, damage)) {
		WBUFQ(buf, offset+0) = GET_PTR(bg, damage, max_damage);
		WBUFQ(buf, offset+8) = GET_PTR(bg, damage, damage);
		WBUFQ(buf, offset+16) = GET_PTR(bg, damage, damage_received);
		offset += 24;
	}
	CHECK_COND(*type, EBG_SAVE_BG_RANK_MISC, CHECK_PTR(bg, misc)) {
		WBUFL(buf, offset+0) = GET_PTR(bg, misc, kills);
		WBUFL(buf, offset+4) = GET_PTR(bg, misc, deaths);
		WBUFL(buf, offset+8) = GET_PTR(bg, misc, ammo);
		WBUFL(buf, offset+12) = GET_PTR(bg, misc, score);
		offset += 16;
	}
	CHECK_COND(*type, EBG_SAVE_BG_RANK_HEAL, CHECK_PTR(bg, heal)) {
		WBUFQ(buf, offset+0) = GET_PTR(bg, heal, healing);
		WBUFQ(buf, offset+8) = GET_PTR(bg, heal, healing_fail);
		WBUFL(buf, offset+16) = GET_PTR(bg, heal, sp);
		WBUFL(buf, offset+20) = GET_PTR(bg, heal, hp_potions);
		WBUFL(buf, offset+24) = GET_PTR(bg, heal, sp_potions);
		offset += 28;
	}
	CHECK_COND(*type, EBG_SAVE_BG_RANK_ITEM, CHECK_PTR(bg, item)) {
		WBUFL(buf, offset+0) = GET_PTR(bg, item, red_gemstones);
		WBUFL(buf, offset+4) = GET_PTR(bg, item, blue_gemstones);
		WBUFL(buf, offset+8) = GET_PTR(bg, item, yellow_gemstones);
		WBUFL(buf, offset+12) = GET_PTR(bg, item, poison_bottles);
		WBUFL(buf, offset+16) = GET_PTR(bg, item, acid_demostration);
		WBUFL(buf, offset+20) = GET_PTR(bg, item, acid_demostration_fail);
		offset += 24;
	}
	CHECK_COND(*type, EBG_SAVE_BG_RANK_SKILL, CHECK_PTR(bg, skill)) {
		WBUFL(buf, offset+0) = GET_PTR(bg, skill, support_skills);
		WBUFL(buf, offset+4) = GET_PTR(bg, skill, support_skills_fail);
		WBUFL(buf, offset+8) = GET_PTR(bg, skill, spirit);
		WBUFL(buf, offset+12) = GET_PTR(bg, skill, zeny);
		offset += 16;
	}
	CHECK_COND(*type, EBG_SAVE_BG_RANK_DMG2, CHECK_PTR2(bg, damage)) {
		WBUFQ(buf, offset+0) = GET_PTR2(bg, damage, boss_dmg);
		offset += 8;
	}
	///< WoE Syncing Starts
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_DMG, CHECK_PTR(woe, damage)) {
		WBUFQ(buf, offset+0) = GET_PTR(woe, damage, max_damage);
		WBUFQ(buf, offset+8) = GET_PTR(woe, damage, damage);
		WBUFQ(buf, offset+16) = GET_PTR(woe, damage, damage_received);
		offset += 24;
	}
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_MISC, CHECK_PTR(woe, misc)) {
		WBUFL(buf, offset+0) = GET_PTR(woe, misc, kills);
		WBUFL(buf, offset+4) = GET_PTR(woe, misc, deaths);
		WBUFL(buf, offset+8) = GET_PTR(woe, misc, ammo);
		WBUFL(buf, offset+12) = GET_PTR(woe, misc, score);
		offset += 16;
	}
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_HEAL, CHECK_PTR(woe, heal)) {
		WBUFQ(buf, offset+0) = GET_PTR(woe, heal, healing);
		WBUFQ(buf, offset+8) = GET_PTR(woe, heal, healing_fail);
		WBUFL(buf, offset+16) = GET_PTR(woe, heal, sp);
		WBUFL(buf, offset+20) = GET_PTR(woe, heal, hp_potions);
		WBUFL(buf, offset+24) = GET_PTR(woe, heal, sp_potions);
		offset += 28;
	}
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_ITEM, CHECK_PTR(woe, item)) {
		WBUFL(buf, offset+0) = GET_PTR(woe, item, red_gemstones);
		WBUFL(buf, offset+4) = GET_PTR(woe, item, blue_gemstones);
		WBUFL(buf, offset+8) = GET_PTR(woe, item, yellow_gemstones);
		WBUFL(buf, offset+12) = GET_PTR(woe, item, poison_bottles);
		WBUFL(buf, offset+16) = GET_PTR(woe, item, acid_demostration);
		WBUFL(buf, offset+20) = GET_PTR(woe, item, acid_demostration_fail);
		offset += 24;
	}
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_SKILL, CHECK_PTR(woe, skill)) {
		WBUFL(buf, offset+0) = GET_PTR(woe, skill, support_skills);
		WBUFL(buf, offset+4) = GET_PTR(woe, skill, support_skills_fail);
		WBUFL(buf, offset+8) = GET_PTR(woe, skill, spirit);
		WBUFL(buf, offset+12) = GET_PTR(woe, skill, zeny);
		offset += 16;
	}
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_MISC2, CHECK_PTR2(woe, misc)) {
		WBUFL(buf, offset+0) = GET_PTR2(woe, misc, emperium);
		WBUFL(buf, offset+4) = GET_PTR2(woe, misc, barricade);
		WBUFL(buf, offset+8) = GET_PTR2(woe, misc, guardian);
		WBUFL(buf, offset+12) = GET_PTR2(woe, misc, gstone);
		offset += 16;
	}
	CHECK_COND(*type, EBG_SAVE_WOE_RANK_DMG2, CHECK_PTR2(woe, damage)) {
		WBUFQ(buf, offset+0) = GET_PTR2(woe, damage, emperium_dmg);
		WBUFQ(buf, offset+8) = GET_PTR2(woe, damage, barricade_dmg);
		WBUFQ(buf, offset+16) = GET_PTR2(woe, damage, guardian_dmg);
		WBUFQ(buf, offset+24) = GET_PTR2(woe, damage, gstone_dmg);
		offset += 32;
	}
#undef GET_PTR
#undef GET_PTR2
#undef CHECK_PTR
#undef CHECK_PTR2
#endif
	/** Battleground Modes */
	CHECK_COND(*type, EBG_SAVE_COMMON, esdb_bg->common != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->common->points;
		WBUFL(buf, offset+4) = esdb_bg->common->rank_points;
		WBUFL(buf, offset+8) = esdb_bg->ranked_games;
		WBUFL(buf, offset+12) = esdb_bg->common->rank_games;
		WBUFL(buf, offset+16) = esdb_bg->common->daymonth;
		offset += 20;
	}
	CHECK_COND(*type, EBG_SAVE_COMMON_STATS, esdb_bg->common != NULL) {
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->common->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_CTF, esdb_bg->ctf != NULL) {
		WBUFL(buf, offset + 0) = esdb_bg->ctf->captured;
		WBUFL(buf, offset + 4) = esdb_bg->ctf->onhand;
		WBUFL(buf, offset + 8) = esdb_bg->ctf->dropped;
		offset += 12;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->ctf->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_BOSS, esdb_bg->boss != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->boss->killed;
		WBUFL(buf, offset+4) = esdb_bg->boss->flags;
		offset += 8;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->boss->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_TI, esdb_bg->ti != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->ti->skulls;
		offset += 4;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->ti->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_EOS, esdb_bg->eos != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->eos->flags;
		WBUFL(buf, offset+4) = esdb_bg->eos->bases;
		offset += 8;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->eos->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_TD, esdb_bg->td != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->td->kills;
		WBUFL(buf, offset+4) = esdb_bg->td->deaths;
		offset += 8;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->td->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_SC, esdb_bg->sc != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->sc->steals;
		WBUFL(buf, offset+4) = esdb_bg->sc->captures;
		WBUFL(buf, offset+8) = esdb_bg->sc->drops;
		offset += 12;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->sc->bgm_status);
	}
	CHECK_COND(*type, EBG_SAVE_CONQ, esdb_bg->conq != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->conq->emperium;
		WBUFL(buf, offset+4) = esdb_bg->conq->barricade;
		WBUFL(buf, offset+8) = esdb_bg->conq->guardian;
		offset += 12;
		ebg_char_set_bgm_status_as_buf(8, buf, &offset, esdb_bg->conq->bgm_status);	// No Tie
	}
	CHECK_COND(*type, EBG_SAVE_RUSH, esdb_bg->rush != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->rush->emp_captured;
		offset += 4;
		ebg_char_set_bgm_status_as_buf(8, buf, &offset, esdb_bg->rush->bgm_status);	// No Tie
	}
	CHECK_COND(*type, EBG_SAVE_DOM, esdb_bg->dom != NULL) {
		WBUFL(buf, offset+0) = esdb_bg->dom->offKill;
		WBUFL(buf, offset+4) = esdb_bg->dom->defKill;
		WBUFL(buf, offset+8) = esdb_bg->dom->bases;
		offset += 12;
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->dom->bgm_status);
	}
	//CHECK_COND(*type, EBG_SAVE_TOD, esdb_bg->tod != NULL) {}
	CHECK_COND(*type, EBG_SAVE_LEADER, esdb_bg->leader != NULL) {
		ebg_char_set_bgm_status_as_buf(12, buf, &offset, esdb_bg->leader->bgm_status);
	}
	// Remove Free Type.
	*type &= ~(EBG_FREE1 | EBG_FREE2 | EBG_FREE3 | EBG_FREE4 | EBG_FREE5 | EBG_FREE6);
	*offset_ = offset;
#undef CHECK_COND
}

/**
 * Get's First 16 bit and Last 16 bit of Variable
 * @param val 32 bit variable
 * @param idx 0=Gets First 16 bit value, 1=Gets Last 16bit value
 **/
uint16 GetWord(uint32 val, int idx)
{
	switch( idx ) {
	case 0:
		return (uint16)( (val & 0x0000FFFF)         );
	case 1:
		return (uint16)( (val & 0xFFFF0000) >> 0x10 );
	default:
#if defined(DEBUG)
		ShowDebug("GetWord: invalid index (idx=%d)\n", idx);
#endif
		return 0;
	}
}

/**
 * Creates 16 bit variable with 2 8bit variables
 * @param byte0 First 8 bit variable
 * @param byte1 Second 8 bit variable
 **/
uint16 MakeWord(uint8 byte0, uint8 byte1)
{
	return byte0 | (byte1 << 0x08);
}

/**
 * Creates 32 bit variable with 2 16bit variables
 * @param word0 First 8 bit variable
 * @param word1 Second 8 bit variable
 **/
uint32 MakeDWord(uint16 word0, uint16 word1)
{
	return
		( (uint32)(word0        ) )|
		( (uint32)(word1 << 0x10) );
}

#ifdef EBG_RANKING
/**
 * Checks the Data and Creates it, if create is set to true.
 * Also returns the address of the variable
 * @method bg_get_variable_common
 * @param  esdb                   ebg_save_data
 * @param  type                   type of data to be retrieved
 * @param  create                 should memory be created if not yet?
 * @param  save_flag              save_flag variable
 * @param  is_bg                  map type (MAP_IS_NONE/MAP_IS_BG/MAP_IS_WOE)
 * @return                        address of variable.
 */
void *bg_get_variable_common(struct ebg_save_data *esdb, int type, bool create, int64 *save_flag, int is_bg)
#else
void *bg_get_variable_common(struct ebg_save_data *esdb, int type, bool create, int64 *save_flag)
#endif
{
#define SET_SAVE_FLAG(var, type) if (var != NULL) \
									*var |= type
	struct ebg_save_data_bg *bgd = &(esdb->bg);

#ifdef EBG_RANKING
	uint16 type2;
	/**
	 * 0 : Check Mapflag
	 * 1 : Force BG
	 * 2 : Force WoE
	 **/
	short force_bg_woe = MAP_IS_NONE;
	if (type > PLAYER_DATA_MAX) {
		type2 = GetWord(type, 1);
		type = GetWord(type, 0);
		eShowDebug("get_variable: Accessing type: %d, type2: %d\n", type, type2);
		if (type2 == RANKING_BG_FORCE) {
			force_bg_woe = MAP_IS_BG;
		} else if (type2 == RANKING_WOE_FORCE) {
			force_bg_woe = MAP_IS_WOE;
		}
	}
#endif
	switch (type) {
	/// Common
	case BG_RANKED_GAMES:
		SET_SAVE_FLAG(save_flag, EBG_SAVE_COMMON);
		return (&bgd->ranked_games);
	/// CTF 
	case CTF_ONHAND:
	case CTF_DROPPED:
	case CTF_CAPTURED:
		CHECK_CREATE(bgd->ctf)
		BG_CREATE_DATA_CHAR(bgd, ctf, ctf_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_CTF);
		if (type == CTF_ONHAND) {
			return (&bgd->ctf->onhand);
		} else if (type == CTF_DROPPED) {
			return (&bgd->ctf->dropped);
		} else if (type == CTF_CAPTURED) {
			return (&bgd->ctf->captured);
		}
		break;
	case BG_CTF_WON:
	case BG_CTF_LOSS:
	case BG_CTF_TIE:
		CHECK_CREATE(bgd->ctf)
		BG_CREATE_DATA_CHAR(bgd, ctf, ctf_bg_data, 1)
		CHECK_CREATE(bgd->ctf->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, ctf, ctf_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_CTF);
		if (type == BG_CTF_WON) {
			return (&bgd->ctf->bgm_status->win);
		} else if (type == BG_CTF_LOSS) {
			return (&bgd->ctf->bgm_status->loss);
		} else if (type == BG_CTF_TIE) {
			return (&bgd->ctf->bgm_status->tie);
		}
		break;
	/// Common Stats
	case BG_POINTS:
	case BG_RANKED_POINTS:
	case BG_TOTAL_RANKED_GAMES:
	case BG_TEAM:
		CHECK_CREATE(bgd->common)
		BG_CREATE_DATA_CHAR(bgd, common, bg_data_main, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_COMMON);
		if (type == BG_POINTS) {
			return (&bgd->common->points);
		} else if (type == BG_RANKED_POINTS) {
			return (&bgd->common->rank_points);
		} else if (type == BG_TOTAL_RANKED_GAMES) {
			return (&bgd->common->rank_games);
		} else if (type == BG_TEAM) {
			return (&bgd->common->team);
		}
		break;
	case BG_WON:
	case BG_LOSS:
	case BG_TIE:
		CHECK_CREATE(bgd->common)
		BG_CREATE_DATA_CHAR(bgd, common, bg_data_main, 1)
		CHECK_CREATE(bgd->common->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, common, bg_data_main, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_COMMON_STATS);
		if (type == BG_WON) {
			return (&bgd->common->bgm_status->win);
		} else if (type == BG_LOSS) {
			return (&bgd->common->bgm_status->loss);
		} else if (type == BG_TIE) {
			return (&bgd->common->bgm_status->tie);
		}
		break;
	/// BOSS 
	case BG_BOSS_WON:
	case BG_BOSS_LOSS:
	case BG_BOSS_TIE:
		CHECK_CREATE(bgd->boss)
		BG_CREATE_DATA_CHAR(bgd, boss, boss_bg_data, 1)
		CHECK_CREATE(bgd->boss->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, boss, boss_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_BOSS);
		if (type == BG_BOSS_WON) {
			return (&bgd->boss->bgm_status->win);
		} else if (type == BG_BOSS_LOSS) {
			return (&bgd->boss->bgm_status->loss);
		} else if (type == BG_BOSS_TIE) {
			return (&bgd->boss->bgm_status->tie);
		}
		break;
	case BOSS_KILLED:
	case BOSS_NEUT_FLAG:
		CHECK_CREATE(bgd->boss)
		BG_CREATE_DATA_CHAR(bgd, boss, boss_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_BOSS);
		if (type == BOSS_KILLED) {
			return (&bgd->boss->killed);
		} else if (type == BOSS_NEUT_FLAG) {
			return (&bgd->boss->flags);
		}
		break;
	/// TI 
	case BG_TI_WON:
	case BG_TI_LOSS:
	case BG_TI_TIE:
		CHECK_CREATE(bgd->ti)
		BG_CREATE_DATA_CHAR(bgd, ti, ti_bg_data, 1)
		CHECK_CREATE(bgd->ti->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, ti, ti_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_TI);
		if (type == BG_TI_WON) {
			return (&bgd->ti->bgm_status->win);
		} else if (type == BG_TI_LOSS) {
			return (&bgd->ti->bgm_status->loss);
		} else if (type == BG_TI_TIE) {
			return (&bgd->ti->bgm_status->tie);
		}
		break;
	case BG_TI_SKULL:
		CHECK_CREATE(bgd->ti)
		BG_CREATE_DATA_CHAR(bgd, ti, ti_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_TI);
		return &bgd->ti->skulls;
	/// EOS 
	case BG_EOS_WON:
	case BG_EOS_LOSS:
	case BG_EOS_TIE:
		CHECK_CREATE(bgd->eos)
		BG_CREATE_DATA_CHAR(bgd, eos, eos_bg_data, 1)
		CHECK_CREATE(bgd->eos->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, eos, eos_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_EOS);
		if (type == BG_EOS_WON) {
			return (&bgd->eos->bgm_status->win);
		} else if (type == BG_EOS_LOSS) {
			return (&bgd->eos->bgm_status->loss);
		} else if (type == BG_EOS_TIE) {
			return (&bgd->eos->bgm_status->tie);
		}
		break;
	case BG_EOS_BASE:
	case BG_EOS_FLAG:
		CHECK_CREATE(bgd->eos)
		BG_CREATE_DATA_CHAR(bgd, eos, eos_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_EOS);
		if (type == BG_EOS_BASE) {
			return (&bgd->eos->bases);
		} else if (type == BG_EOS_FLAG) {
			return (&bgd->eos->flags);
		}
		break;
	/// TD 
	case BG_TD_WON:
	case BG_TD_LOSS:
	case BG_TD_TIE:
		CHECK_CREATE(bgd->td)
		BG_CREATE_DATA_CHAR(bgd, td, tdm_bg_data, 1)
		CHECK_CREATE(bgd->td->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, td, tdm_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_TD);
		if (type == BG_TD_WON) {
			return (&bgd->td->bgm_status->win);
		} else if (type == BG_TD_LOSS) {
			return (&bgd->td->bgm_status->loss);
		} else if (type == BG_TD_TIE) {
			return (&bgd->td->bgm_status->tie);
		}
		break;
	case BG_TD_KILL:
	case BG_TD_DIE:
		CHECK_CREATE(bgd->td)
		BG_CREATE_DATA_CHAR(bgd, td, tdm_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_TD);
		if (type == BG_TD_KILL) {
			return (&bgd->td->kills);
		} else if (type == BG_TD_DIE) {
			return (&bgd->td->deaths);
		}
		break;
	/// SC 
	case BG_SC_CAPTURED:
	case BG_SC_DROPPED:
	case BG_SC_STOLE:
		CHECK_CREATE(bgd->sc)
		BG_CREATE_DATA_CHAR(bgd, sc, sc_bg_data, 1);
		SET_SAVE_FLAG(save_flag, EBG_SAVE_SC);
		if (type == BG_SC_CAPTURED) {
			return (&bgd->sc->captures);
		} else if (type == BG_SC_DROPPED) {
			return (&bgd->sc->drops);
		} else if (type == BG_SC_STOLE) {
			return (&bgd->sc->steals);
		}
		break;
	case BG_SC_WON:
	case BG_SC_LOSS:
	case BG_SC_TIE:
		CHECK_CREATE(bgd->sc)
		BG_CREATE_DATA_CHAR(bgd, sc, sc_bg_data, 1)
		CHECK_CREATE(bgd->sc->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, sc, sc_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_SC);
		if (type == BG_SC_WON) {
			return (&bgd->sc->bgm_status->win);
		} else if (type == BG_SC_LOSS) {
			return (&bgd->sc->bgm_status->loss);
		} else if (type == BG_SC_TIE) {
			return (&bgd->sc->bgm_status->tie);
		}
		break;
	/* CONQ - No TIE*/
	case BG_CONQ_WON:
	case BG_CONQ_LOSS:
		CHECK_CREATE(bgd->conq)
		BG_CREATE_DATA_CHAR(bgd, conq, conq_bg_data, 1)
		CHECK_CREATE(bgd->conq->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, conq, conq_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_CONQ);
		if (type == BG_CONQ_WON) {
			return (&bgd->conq->bgm_status->win);
		} else if (type == BG_CONQ_LOSS) {
			return (&bgd->conq->bgm_status->loss);
		}
		break;
	case BG_CONQ_EMPERIUM:
	case BG_CONQ_BARRICADE:
	case BG_CONQ_GUARDIAN_S:
		CHECK_CREATE(bgd->conq)
		BG_CREATE_DATA_CHAR(bgd, conq, conq_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_CONQ);
		if (type == BG_CONQ_EMPERIUM) {
			return (&bgd->conq->emperium);
		} else if (type == BG_CONQ_BARRICADE) {
			return (&bgd->conq->barricade);
		} else if (type == BG_CONQ_GUARDIAN_S) {
			return (&bgd->conq->guardian);
		}
		break;
	/* RUSH - No TIE **/
	case BG_RUSH_EMPERIUM:
		CHECK_CREATE(bgd->rush)
		BG_CREATE_DATA_CHAR(bgd, rush, rush_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_RUSH);
		return (&bgd->rush->emp_captured);
	case BG_RUSH_WON:
	case BG_RUSH_LOSS:
		CHECK_CREATE(bgd->rush)
		BG_CREATE_DATA_CHAR(bgd, rush, rush_bg_data, 1)
		CHECK_CREATE(bgd->rush->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, rush, rush_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_RUSH);
		if (type == BG_RUSH_WON) {
			return (&bgd->rush->bgm_status->win);
		} else if (type == BG_RUSH_LOSS) {
			return (&bgd->rush->bgm_status->loss);
		}
		break;
	/// Domination
	case BG_DOM_WON:
	case BG_DOM_LOSS:
	case BG_DOM_TIE:
		CHECK_CREATE(bgd->dom)
		BG_CREATE_DATA_CHAR(bgd, dom, dom_bg_data, 1)
		CHECK_CREATE(bgd->dom->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, dom, dom_bg_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_DOM);
		if (type == BG_DOM_WON) {
			return (&bgd->dom->bgm_status->win);
		} else if (type == BG_DOM_LOSS) {
			return (&bgd->dom->bgm_status->loss);
		} else if (type == BG_DOM_TIE) {
			return (&bgd->dom->bgm_status->tie);
		}
		break;
	case BG_DOM_BASE:
	case BG_DOM_OFF_KILL:
	case BG_DOM_DEF_KILL:
		CHECK_CREATE(bgd->dom)
		BG_CREATE_DATA_CHAR(bgd, dom, dom_bg_data, 1)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_DOM);
		if (type == BG_DOM_BASE) {
			return (&bgd->dom->bases);
		} else if (type == BG_DOM_OFF_KILL) {
			return (&bgd->dom->offKill);
		} else if (type == BG_DOM_DEF_KILL) {
			return (&bgd->dom->defKill);
		}
		break;
	/// Leader Stats 
	case BG_LEADER_WON:
	case BG_LEADER_LOSS:
	case BG_LEADER_TIE:
		CHECK_CREATE(bgd->leader)
		BG_CREATE_DATA_CHAR(bgd, leader, bg_status_data, 1)
		CHECK_CREATE(bgd->leader->bgm_status)
		BG_CREATE_DATA_CHAR(bgd, leader, bg_status_data, 2)
		SET_SAVE_FLAG(save_flag, EBG_SAVE_LEADER);
		if (type == BG_LEADER_WON) {
			return (&bgd->leader->bgm_status->win);
		} else if (type == BG_LEADER_LOSS) {
			return (&bgd->leader->bgm_status->loss);
		} else if (type == BG_LEADER_TIE) {
			return (&bgd->leader->bgm_status->tie);
		}
		break;
		/** Ranks */

/**
 * Creates Memory of required structure and return it's address.
 * (Used for Common Rankings)
 * @param type variable containing enum from Player_Data
 * @param type2 sd_p_data structure
 * @param bg_struct Structure name (used for allocation of memory)
 * @param is_bg If map is BG or WoE
 * @param bg_type Pointer name to the structure
 * @param bg_variable Variable in the structure to be referred to.
 * @param sv_type Enum to be added to save variable (Trimmed, EBG_SAVE_BG_ is not to be included)
 * @see Player_Data
 * @see ebg_save_data
 * @see ranking_data
 * @see ranking_data_bg
 * @see ranking_data_woe
 * @see bg_save_flag
 **/
#define RANKING_RETURN_POINTER(type, type2, bg_struct, is_bg, bg_type, bg_variable, sv_type) \
			if (type == type2) { \
				if (force_bg_woe == MAP_IS_NONE && is_bg == MAP_IS_NONE) { \
					return NULL; \
				} else if ((force_bg_woe == MAP_IS_NONE && is_bg == MAP_IS_BG) || force_bg_woe == MAP_IS_BG) { \
					CHECK_CREATE(esdb->rank.bg) \
					BG_CREATE_DATA_SUB(&(esdb->rank), bg, ranking_data_bg) \
					CHECK_CREATE(esdb->rank.bg->rank) \
					BG_CREATE_DATA_SUB(esdb->rank.bg, rank, ranking_data) \
					CHECK_CREATE(esdb->rank.bg->rank->bg_type) \
					BG_CREATE_DATA_SUB(esdb->rank.bg->rank, bg_type, bg_struct) \
					SET_SAVE_FLAG(save_flag, EBG_SAVE_BG_##sv_type); \
					return (&esdb->rank.bg->rank->bg_type->bg_variable); \
				} else if ((force_bg_woe == MAP_IS_NONE && is_bg == MAP_IS_WOE) || force_bg_woe == MAP_IS_WOE) { \
					CHECK_CREATE(esdb->rank.woe) \
					BG_CREATE_DATA_SUB(&(esdb->rank), woe, ranking_data_woe) \
					CHECK_CREATE(esdb->rank.woe->rank) \
					BG_CREATE_DATA_SUB(esdb->rank.woe, rank, ranking_data) \
					CHECK_CREATE(esdb->rank.woe->rank->bg_type) \
					BG_CREATE_DATA_SUB(esdb->rank.woe->rank, bg_type, bg_struct) \
					SET_SAVE_FLAG(save_flag, EBG_SAVE_WOE_##sv_type); \
					return (&esdb->rank.woe->rank->bg_type->bg_variable); \
				} else { \
					return NULL; \
				} \
			}
 /**
  * Creates Memory of required structure and return it's address.
  * (Used for Special BG Rankings)
  * @param type variable containing enum from Player_Data
  * @param type2 sd_p_data structure
  * @param bg_struct Structure name (used for allocation of memory)
  * @param is_bg If map is BG or WoE
  * @param bg_type Pointer name to the structure
  * @param bg_variable Variable in the structure to be referred to.
  * @param sv_type Enum to be added to save variable (Trimmed, EBG_SAVE_BG_ is not to be included)
  * @see RANKING_RETURN_POINTER
  * @see ebg_save_data
  * @see Player_Data
  * @see ranking_data
  * @see ranking_data_bg
  * @see bg_save_flag
  **/
#define RANKING_BG_RETURN_POINTER(type, type2, bg_struct, is_bg, bg_type, bg_variable, sv_type) \
			if (type == type2 && ((force_bg_woe == MAP_IS_NONE && is_bg == MAP_IS_BG) || force_bg_woe == MAP_IS_BG)) { \
				CHECK_CREATE(esdb->rank.bg) \
				BG_CREATE_DATA_SUB(&(esdb->rank), bg, ranking_data_bg) \
				CHECK_CREATE(esdb->rank.bg->bg_type) \
				BG_CREATE_DATA_SUB(esdb->rank.bg, bg_type, bg_struct) \
				SET_SAVE_FLAG(save_flag, EBG_SAVE_BG_##sv_type); \
				return (&esdb->rank.bg->bg_type->bg_variable); \
			}
  /**
   * Creates Memory of required structure and return it's address.
   * (Used for Special WoE Rankings)
   * @param type variable containing enum from Player_Data
   * @param type2 sd_p_data structure
   * @param woe_struct Structure name (used for allocation of memory)
   * @param is_bg If map is BG or WoE
   * @param woe_type Pointer name to the structure
   * @param woe_variable Variable in the structure to be referred to.
   * @param sv_type Enum to be added to save variable (Trimmed, EBG_SAVE_BG_ is not to be included)
   * @see RANKING_RETURN_POINTER
   * @see ebg_save_data
   * @see Player_Data
   * @see ranking_data
   * @see ranking_data_woe
   * @see bg_save_flag
   **/
#define RANKING_WOE_RETURN_POINTER(type, type2, woe_struct, is_bg, woe_type, woe_variable, sv_type) \
			if (type == type2 && ((force_bg_woe == MAP_IS_NONE && is_bg == MAP_IS_WOE) || force_bg_woe == MAP_IS_WOE)) { \
				CHECK_CREATE(esdb->rank.woe) \
				BG_CREATE_DATA_SUB(&(esdb->rank), woe, ranking_data_woe) \
				CHECK_CREATE(esdb->rank.woe->woe_type) \
				BG_CREATE_DATA_SUB(esdb->rank.woe, woe_type, woe_struct) \
				SET_SAVE_FLAG(save_flag, EBG_SAVE_WOE_##sv_type); \
				return (&esdb->rank.woe->woe_type->woe_variable); \
			}

	// RANK_DMG
	case RANKING_MAX_DAMAGE:
	case RANKING_DAMAGE_DEALT:
	case RANKING_DAMAGE_RECEIVED:
		RANKING_RETURN_POINTER(type, RANKING_MAX_DAMAGE, ranking_sub_damage, is_bg, damage, max_damage, RANK_DMG)
		RANKING_RETURN_POINTER(type, RANKING_DAMAGE_DEALT, ranking_sub_damage, is_bg, damage, damage, RANK_DMG)
		RANKING_RETURN_POINTER(type, RANKING_DAMAGE_RECEIVED, ranking_sub_damage, is_bg, damage, damage_received, RANK_DMG)
		break;

	// RANK_HEAL
	case RANKING_HEAL_TEAM:
	case RANKING_HEAL_ENEMY:
	case RANKING_SP_USED:
	case RANKING_HP_POTION:
	case RANKING_SP_POTION:
		RANKING_RETURN_POINTER(type, RANKING_HEAL_TEAM, ranking_sub_heal, is_bg, heal, healing, RANK_HEAL)
		RANKING_RETURN_POINTER(type, RANKING_HEAL_ENEMY, ranking_sub_heal, is_bg, heal, healing_fail, RANK_HEAL)
		RANKING_RETURN_POINTER(type, RANKING_SP_USED, ranking_sub_heal, is_bg, heal, sp, RANK_HEAL)
		RANKING_RETURN_POINTER(type, RANKING_HP_POTION, ranking_sub_heal, is_bg, heal, hp_potions, RANK_HEAL)
		RANKING_RETURN_POINTER(type, RANKING_SP_POTION, ranking_sub_heal, is_bg, heal, sp_potions, RANK_HEAL)
		break;

	// RANK_ITEM
	case RANKING_RED_GEMSTONE:
	case RANKING_BLUE_GEMSTONE:
	case RANKING_YELLOW_GEMSTONE:
	case RANKING_POISON_BOTTLE:
	case RANKING_ACID_DEMONSTRATION:
	case RANKING_ACID_DEMONSTRATION_F:
		RANKING_RETURN_POINTER(type, RANKING_RED_GEMSTONE, ranking_sub_item, is_bg, item, red_gemstones, RANK_ITEM)
		RANKING_RETURN_POINTER(type, RANKING_BLUE_GEMSTONE, ranking_sub_item, is_bg, item, blue_gemstones, RANK_ITEM)
		RANKING_RETURN_POINTER(type, RANKING_YELLOW_GEMSTONE, ranking_sub_item, is_bg, item, yellow_gemstones, RANK_ITEM)
		RANKING_RETURN_POINTER(type, RANKING_POISON_BOTTLE, ranking_sub_item, is_bg, item, poison_bottles, RANK_ITEM)
		RANKING_RETURN_POINTER(type, RANKING_ACID_DEMONSTRATION, ranking_sub_item, is_bg, item, acid_demostration, RANK_ITEM)
		RANKING_RETURN_POINTER(type, RANKING_ACID_DEMONSTRATION_F, ranking_sub_item, is_bg, item, acid_demostration_fail, RANK_ITEM)
		break;

	// RANK_SKILL
	case RANKING_SUPPORT_SKILL_TEAM:
	case RANKING_SUPPORT_SKILL_ENEMY:
	case RANKING_SPIRITB:
	case RANKING_ZENY:
		RANKING_RETURN_POINTER(type, RANKING_SUPPORT_SKILL_TEAM, ranking_sub_skill, is_bg, skill, support_skills, RANK_SKILL)
		RANKING_RETURN_POINTER(type, RANKING_SUPPORT_SKILL_ENEMY, ranking_sub_skill, is_bg, skill, support_skills_fail, RANK_SKILL)
		RANKING_RETURN_POINTER(type, RANKING_SPIRITB, ranking_sub_skill, is_bg, skill, spirit, RANK_SKILL)
		RANKING_RETURN_POINTER(type, RANKING_ZENY, ranking_sub_skill, is_bg, skill, zeny, RANK_SKILL)
		break;

	// RANK_MISC
	case RANKING_KILLS:
	case RANKING_DEATHS:
	case RANKING_AMMOS:
	case RANKING_SCORE:
		RANKING_RETURN_POINTER(type, RANKING_KILLS, ranking_sub_misc, is_bg, misc, kills, RANK_MISC)
		RANKING_RETURN_POINTER(type, RANKING_DEATHS, ranking_sub_misc, is_bg, misc, deaths, RANK_MISC)
		RANKING_RETURN_POINTER(type, RANKING_AMMOS, ranking_sub_misc, is_bg, misc, ammo, RANK_MISC)
		RANKING_RETURN_POINTER(type, RANKING_SCORE, ranking_sub_misc, is_bg, misc, score, RANK_MISC)
		break;

	// BG_RANK_DMG2
	case RANKING_BG_BOSS_DMG:
		RANKING_BG_RETURN_POINTER(type, RANKING_BG_BOSS_DMG, ranking_bg_sub_damage, is_bg, damage, boss_dmg, RANK_DMG2)
		break;

	// WOE_RANK_MISC2
	case RANKING_WOE_EMPERIUM:
	case RANKING_WOE_BARRICADE:
	case RANKING_WOE_GUARDIAN:
	case RANKING_WOE_GSTONE:
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_EMPERIUM, ranking_woe_sub_misc, is_bg, misc, emperium, RANK_MISC2)
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_BARRICADE, ranking_woe_sub_misc, is_bg, misc, barricade, RANK_MISC2)
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_GUARDIAN, ranking_woe_sub_misc, is_bg, misc, guardian, RANK_MISC2)
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_GSTONE, ranking_woe_sub_misc, is_bg, misc, gstone, RANK_MISC2)
		break;

	// WOE_RANK_DMG2
	case RANKING_WOE_EMPERIUM_DAMAGE:
	case RANKING_WOE_BARRICADE_DAMAGE:
	case RANKING_WOE_GUARDIAN_DAMAGE:
	case RANKING_WOE_GSTONE_DAMAGE:
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_EMPERIUM_DAMAGE, ranking_woe_sub_damage, is_bg, damage, emperium_dmg, RANK_DMG2)
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_BARRICADE_DAMAGE, ranking_woe_sub_damage, is_bg, damage, barricade_dmg, RANK_DMG2)
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_GUARDIAN_DAMAGE, ranking_woe_sub_damage, is_bg, damage, guardian_dmg, RANK_DMG2)
		RANKING_WOE_RETURN_POINTER(type, RANKING_WOE_GSTONE_DAMAGE, ranking_woe_sub_damage, is_bg, damage, gstone_dmg, RANK_DMG2)
		break;
	}
#undef RANKING_WOE_RETURN_POINTER
#undef RANKING_BG_RETURN_POINTER
#undef RANKING_RETURN_POINTER
	// No Data Found.
	return NULL;
}

/**
 * Free's the Result of sql
 * @method ebg_free_sql_query
 * @param  sql_handle         sql handle which is to be freed.
 */
void ebg_free_sql_query(struct Sql *sql_handle)
{
#if (defined(EBG_CHAR) || !defined(CHAR_SERVER_SAVE))
	SQL->FreeResult(sql_handle);
#endif
}

/**
 * Executes SQL query if use_sql is set to true.
 * else, reads from packet data that is received to Map/Char Server.
 * @method ebg_execute_sql_query
 * @param  temp                  pointer to integer variable
 * @param  temp64                pointer to int64 variable
 * @param  use_sql               true, if SQL is to be used.
 * @param  save                  pointer to save variable, used for primary data.
 * @param  save_                 pointer to save_ variable, used for secondary data(bgm_status)
 * @param  type                  save_type.
 * @param  sql_handle            sql_handle (used with use_sql=true)
 * @param  query                 query to be fired.
 * @param  char_id               character ID (offset, in case of map-server)
 * @param  fd                    File Descriptor (used for sending data)
 * @return                       true, if query is fired successfully or if data is received, else false.
 */
bool ebg_execute_sql_query(int *temp, int64 *temp64, bool use_sql, bool *save, bool *save_, int64 type, struct Sql *sql_handle, const char *query, int *char_id_offset, int fd)
{
/**
 * Saves bgm_status data.
 * @method SYNC_SAVE__COMMON_DATA
 * @param  end_value              max packet length to fetch.
 */
#define SYNC_SAVE__COMMON_DATA(end_value) \
				if (save_ == NULL) { \
					ShowError("Called SYNC_SAVE__COMMON_DATA, but save_ is NULL.\n"); \
				} else { \
					*save_ = true; \
					eShowDebug("COMMON_DATA_START\n"); \
					SYNC_VALUE_BULK(end_value, 4); \
					eShowDebug("COMMON_DATA_END\n"); \
				}

/**
 * Loops end_value/increment times, and saves the data in
 * pointer
 * @method SYNC_VALUE_BULK
 * @param  end_value       max packet length to fetch
 * @param  increment       increment (short = 2, int = 4, qword = 8)
 */
#define SYNC_VALUE_BULK(end_value, increment) \
				eShowDebug("SYNC_VALUE: FD:%d TYPE:%"PRId64"\n", fd, type); \
				for (i = 0; i < end_value; i = i + increment) { \
					if (increment == 2) { \
						SYNC_VALUE(offset + i, RFIFOW, temp); \
						eShowDebug("SYNC_VALUE_POSTW: RIDX:%d TI:%d Val:%d\n", offset + i, temp_index, (int)RFIFOW(fd, offset + i)); \
					} else if (increment == 4) { \
						SYNC_VALUE(offset + i, RFIFOL, temp); \
						eShowDebug("SYNC_VALUE_POSTL: RIDX:%d TI:%d Val:%u\n", offset + i, temp_index, RFIFOL(fd, offset + i)); \
					} else if (increment == 8) { \
						SYNC_VALUE(offset + i, RFIFOQ, temp64); \
						eShowDebug("SYNC_VALUE_POSTQ: RIDX:%d TI:%d Val:%"PRIu64"\n", offset + i, temp_index, RFIFOQ(fd, offset + i)); \
					} \
				} \
				if (save == NULL) { \
					ShowError("Called SYNC_VALUE_BULK, but save is NULL.\n"); \
				} else { \
					*save = true; \
					offset += i; \
				}
/**
 * Saves the Value to pointer
 * @method SYNC_VALUE
 * @param  value      offset of packet
 * @param  rfifo_chr  function to use
 * @param  ptr        pointer name
 */
#define SYNC_VALUE(value, rfifo_chr, ptr)  \
				*(ptr + temp_index) = rfifo_chr(fd, value); \
				temp_index++

	if (save != NULL) {
		*save = false;
	}
	if (save_ != NULL) {
		*save_ = false;
	}
	if (use_sql) {
		if (strcmp(query, "") == 0 && save != NULL) {
			*save = true;
			return true;
		}
		if (sql_handle == NULL) {
			return false;
		}
		if (SQL_ERROR == SQL->Query(sql_handle, query, *char_id_offset)) {	// Acts as Char ID here
			Sql_ShowDebug(sql_handle);
			return false;
		}
		
		if (SQL_SUCCESS == SQL->NextRow(sql_handle)) {
			return true;
		}
		return false;
	}
	else {
		int temp_index = 0, i;
		int offset = 0;
		if (type == 0x0) {	// Invalid
			return false;
		}
		if (fd < 0) {
			return false;
		}
#ifdef EBG_MAP
		offset = *char_id_offset;	// Acts as Offset
#endif
		eShowDebug("Remaining Bytes: %d\n", (int)RFIFOREST(fd));
		/** BG Rank Data */
		if (type & EBG_SAVE_BG_RANK_DMG) {
			SYNC_VALUE_BULK(24, 8);
		}
		if (type & EBG_SAVE_BG_RANK_MISC) {
			SYNC_VALUE_BULK(16, 4);
		}
		if (type & EBG_SAVE_BG_RANK_HEAL) {
			SYNC_VALUE_BULK(16, 8);
			SYNC_VALUE_BULK(12, 4);
		}
		if (type & EBG_SAVE_BG_RANK_ITEM) {
			SYNC_VALUE_BULK(24, 4);
		}
		if (type & EBG_SAVE_BG_RANK_SKILL) {
			SYNC_VALUE_BULK(16, 4);
		}
		if (type & EBG_SAVE_BG_RANK_DMG2) {
			SYNC_VALUE_BULK(8, 8);
		}
		/** WoE Rank Data */
		if (type & EBG_SAVE_WOE_RANK_DMG) {
			SYNC_VALUE_BULK(24, 8);
		}
		if (type & EBG_SAVE_WOE_RANK_MISC) {
			SYNC_VALUE_BULK(16, 4);
		}
		if (type & EBG_SAVE_WOE_RANK_HEAL) {
			SYNC_VALUE_BULK(16, 8);
			SYNC_VALUE_BULK(12, 4);
		}
		if (type & EBG_SAVE_WOE_RANK_ITEM) {
			SYNC_VALUE_BULK(24, 4);
		}
		if (type & EBG_SAVE_WOE_RANK_SKILL) {
			SYNC_VALUE_BULK(16, 4);
		}
		if (type & EBG_SAVE_WOE_RANK_MISC2) {
			SYNC_VALUE_BULK(16, 4);
		}
		if (type & EBG_SAVE_WOE_RANK_DMG2) {
			SYNC_VALUE_BULK(32, 8);
		}
		/** BG Data */
		if (type & EBG_SAVE_COMMON) {
			SYNC_VALUE_BULK(20, 4);
		}
		if (type & EBG_SAVE_COMMON_STATS) {
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_CTF) {
			SYNC_VALUE_BULK(12, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_BOSS) {
			SYNC_VALUE_BULK(8, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_TI) {
			SYNC_VALUE_BULK(4, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_EOS) {
			SYNC_VALUE_BULK(8, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_TD) {
			SYNC_VALUE_BULK(8, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_SC) {
			SYNC_VALUE_BULK(12, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		if (type & EBG_SAVE_CONQ) {
			SYNC_VALUE_BULK(12, 4);
			SYNC_SAVE__COMMON_DATA(8);
		}
		if (type & EBG_SAVE_RUSH) {
			SYNC_VALUE_BULK(4, 4);
			SYNC_SAVE__COMMON_DATA(8);
		}
		if (type & EBG_SAVE_DOM) {
			SYNC_VALUE_BULK(12, 4);
			SYNC_SAVE__COMMON_DATA(12);
		}
		// ToD have no extra data.
		if (type & EBG_SAVE_LEADER) {
			SYNC_SAVE__COMMON_DATA(12);
		}
	#undef SYNC_SAVE__COMMON_DATA
	#undef SYNC_VALUE_BULK
	#undef SYNC_VALUE
		if (offset > 0) {
#ifdef EBG_MAP
			*char_id_offset = offset;	// MapServer would take care of skipping packets
#else
			eShowDebug("Skipping Some Bytes:%d:%d\n", fd, offset);
			RFIFOSKIP(fd, offset);
#endif
			return true;
		}
	}
	return false;	// No Data to Return
}

// Used to Save data in case of Char_SERVER_SAVE is enabled.
#ifdef EBG_MAP
void bg_load_char_data_sub(struct map_session_data *sd, struct Sql *sql_handle, struct ebg_save_data *esdb, int offset, int fd, bool use_sql, int64 save_type)
#else
void bg_load_char_data_sub(struct Sql *sql_handle, struct ebg_save_data *esdb, int char_id, int fd, bool use_sql, int64 save_type)
#endif
{
/**
 * Only saves it if the value is not 0
 * @param len Total number of data
 * @param len_ Index after which which bgm_status data begins
 */
#define GET_SAVE_SQL_DATA_(len, len_, arr_name)	\
	if (use_sql) { \
		save = false; \
		save_ = false; \
		for (i = 0; i < len; i++) { \
			arr_name[i] = 0; \
			SQL->GetData(sql_handle, i, &data, NULL); arr_name[i] = atoi(data); \
			if (arr_name[i] > 0) { \
				if (i <= len_) { \
					save = true; \
				} else { \
					save_ = true; \
				} \
			} \
		} \
	}
#ifdef EBG_RANKING
/**
 * Only saves it if the value is not 0
 * @param start Starting Index
 * @param len Ending Index
 * @param local_save Variable to change to true, if data is found.
 */
#define GET_SAVE_SQL_DATA(start, len, local_save, arr_name) \
	if (use_sql) { \
		local_save = false; \
		for (i = start; i <= len; i++) { \
			arr_name[i] = 0; \
			SQL->GetData(sql_handle, i, &data, NULL); arr_name[i] = atoi(data); \
			if (arr_name[i] > 0) { \
				local_save = true; \
			} \
		} \
	}
#endif
		
	char* data;
#ifdef EBG_RANKING
	int temp[32];
	int64 temp64[32];
#else
	int temp[6];
	int64 temp64[6];
#endif
	int i;
	int temp_index = 0;
	bool save = false;  ///< To determine if normal data will be saved
	bool save_ = false; ///< To determine if bgm_status data will be saved
	bool create = true;
	int *data_temp; ///< Used for saving data to main structure.
	unsigned int *udata_temp; ///< Used for saving data to main structure.
	struct ebg_save_data_bg *bgd = &(esdb->bg); ///< For easy creating data, this is reffered,
#ifdef EBG_MAP
	int char_id = offset;	// No need to parse offset seperately.
	eShowDebug("Map(LCDS): CID:%d OFF:%d FD:%d US:%d SaveType:'%"PRId64"'\n", char_id, offset, fd, use_sql?1:0, save_type);
#else
	eShowDebug("Char(LCDS): CID:%d FD:%d US:%d SaveType:'%"PRId64"'\n", char_id, fd, use_sql?1:0, save_type);
#endif

#ifdef EBG_RANKING
	int64 save_type2 = 0;
	/** eBG Save Dmg Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_BG_RANK_DMG, sql_handle, "SELECT `max_damage`, `damage`, `damage_received` FROM `eBG_bg_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(3, 2, temp64);
		if (save) {
			uint64 *damage_ptr;
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_MAX_DAMAGE, RANKING_BG_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_DAMAGE_DEALT, RANKING_BG_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_DAMAGE_RECEIVED, RANKING_BG_FORCE), temp64[temp_index], temp_index, uint64);
			save_type2 |= EBG_SAVE_BG_RANK_DMG;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** eBG Save Misc Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_BG_RANK_MISC, sql_handle, "SELECT `kills`, `deaths`, `ammo_used`, `score` FROM `eBG_bg_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(4, 3, temp);
		SET_VARIABLE_REPLACE_(udata_temp, MakeDWord(RANKING_SCORE, RANKING_BG_FORCE), 2000, unsigned int);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_KILLS, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_DEATHS, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_AMMOS, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SCORE, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_BG_RANK_MISC;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** eBG Save Heal Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_BG_RANK_HEAL, sql_handle, "SELECT `healing`, `healing_fail`, `sp_used`, `hp_potions`, `sp_potions` FROM `eBG_bg_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(5, 4, temp64);
		if (save) {
			uint64 *heal_ptr;
			SET_VARIABLE_REPLACE(heal_ptr, MakeDWord(RANKING_HEAL_TEAM, RANKING_BG_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(heal_ptr, MakeDWord(RANKING_HEAL_ENEMY, RANKING_BG_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SP_USED, RANKING_BG_FORCE), (int)temp64[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_HP_POTION, RANKING_BG_FORCE), (int)temp64[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SP_POTION, RANKING_BG_FORCE), (int)temp64[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_BG_RANK_HEAL;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** eBG Save Item Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_BG_RANK_ITEM, sql_handle, "SELECT `red_gemstones`, `blue_gemstones`, `yellow_gemstones`, `poison_bottles`, `acid_demostration`, `acid_demostration_fail` FROM `eBG_bg_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(6, 5, temp);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_RED_GEMSTONE, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_BLUE_GEMSTONE, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_YELLOW_GEMSTONE, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_POISON_BOTTLE, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_ACID_DEMONSTRATION, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_ACID_DEMONSTRATION_F, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_BG_RANK_ITEM;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** eBG Save Skill Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_BG_RANK_SKILL, sql_handle, "SELECT `support_skills`, `support_skills_fail`, `spiritb_used`, `zeny_used` FROM `eBG_bg_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(4, 3, temp);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SUPPORT_SKILL_TEAM, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SUPPORT_SKILL_ENEMY, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SPIRITB, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_ZENY, RANKING_BG_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_BG_RANK_SKILL;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** eBG Save Dmg2 Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_BG_RANK_DMG2, sql_handle, "SELECT `boss_damage` FROM `eBG_bg_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(1, 0, temp);
		if (save) {
			uint64 *dmg_ptr;
			SET_VARIABLE_REPLACE(dmg_ptr, MakeDWord(RANKING_BG_BOSS_DMG, RANKING_BG_FORCE), temp64[temp_index], temp_index, uint64);
			save_type2 |= EBG_SAVE_BG_RANK_DMG2;
		}
	}
	ebg_free_sql_query(sql_handle);
#ifndef EBG_MAP
	if (!use_sql && save_type2 > 0) {
		bg_save_ranking(esdb, sql_handle, char_id, NULL, save_type2);
		save_type2 = 0;
	}
#endif
	
	/// WoE
	/** WoE Save Dmg Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_DMG, sql_handle, "SELECT `max_damage`, `damage`, `damage_received` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(3, 2, temp64);
		if (save) {
			uint64 *damage_ptr;
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_MAX_DAMAGE, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_DAMAGE_DEALT, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_DAMAGE_RECEIVED, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			save_type2 |= EBG_SAVE_WOE_RANK_DMG;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** WoE Save Misc Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_MISC, sql_handle, "SELECT `kills`, `deaths`, `ammo_used`, `score` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(4, 3, temp);
		SET_VARIABLE_REPLACE_(udata_temp, MakeDWord(RANKING_SCORE, RANKING_WOE_FORCE), 2000, unsigned int);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_KILLS, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_DEATHS, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_AMMOS, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SCORE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_WOE_RANK_MISC;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** WoE Save Heal Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_HEAL, sql_handle, "SELECT `healing`, `healing_fail`, `sp_used`, `hp_potions`, `sp_potions` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(5, 4, temp64);
		if (save) {
			uint64 *heal_ptr;
			SET_VARIABLE_REPLACE(heal_ptr, MakeDWord(RANKING_HEAL_TEAM, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(heal_ptr, MakeDWord(RANKING_HEAL_ENEMY, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SP_USED, RANKING_WOE_FORCE), (int)temp64[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_HP_POTION, RANKING_WOE_FORCE), (int)temp64[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SP_POTION, RANKING_WOE_FORCE), (int)temp64[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_WOE_RANK_HEAL;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** WoE Save Item Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_ITEM, sql_handle, "SELECT `red_gemstones`, `blue_gemstones`, `yellow_gemstones`, `poison_bottles`, `acid_demostration`, `acid_demostration_fail` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(6, 5, temp);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_RED_GEMSTONE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_BLUE_GEMSTONE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_YELLOW_GEMSTONE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_POISON_BOTTLE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_ACID_DEMONSTRATION, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_ACID_DEMONSTRATION_F, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_WOE_RANK_ITEM;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** WoE Save Skill Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_SKILL, sql_handle, "SELECT `support_skills`, `support_skills_fail`, `spiritb_used`, `zeny_used` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(4, 3, temp);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SUPPORT_SKILL_TEAM, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SUPPORT_SKILL_ENEMY, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_SPIRITB, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_ZENY, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_WOE_RANK_SKILL;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** WoE Save Misc2 Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_MISC2, sql_handle, "SELECT `emperium`, `barricade`, `guardian`, `gstone` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(4, 3, temp);
		if (save) {
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_WOE_EMPERIUM, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_WOE_BARRICADE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_WOE_GUARDIAN, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			SET_VARIABLE_REPLACE(udata_temp, MakeDWord(RANKING_WOE_GSTONE, RANKING_WOE_FORCE), temp[temp_index], temp_index, unsigned int);
			save_type2 |= EBG_SAVE_WOE_RANK_MISC2;
		}
	}
	ebg_free_sql_query(sql_handle);
	/** WoE Save Dmg2 Rank */
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, NULL, save_type & EBG_SAVE_WOE_RANK_DMG2, sql_handle, "SELECT `emperium_dmg`, `barricade_dmg`, `guardian_dmg`, `gstone_dmg` FROM `eBG_woe_rankings` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(4, 3, temp64);
		if (save) {
			uint64 *damage_ptr;
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_WOE_EMPERIUM_DAMAGE, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_WOE_BARRICADE_DAMAGE, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_WOE_GUARDIAN_DAMAGE, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			SET_VARIABLE_REPLACE(damage_ptr, MakeDWord(RANKING_WOE_GSTONE_DAMAGE, RANKING_WOE_FORCE), temp64[temp_index], temp_index, uint64);
			save_type2 |= EBG_SAVE_WOE_RANK_DMG2;
		}
	}
	ebg_free_sql_query(sql_handle);
#ifndef EBG_MAP
	if (!use_sql && save_type2 > 0) {
		bg_save_ranking(esdb, sql_handle, char_id, NULL, save_type2);
	}
#endif
#endif
	///< Common 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_COMMON)), sql_handle, "SELECT `points`,`rank_points`,`rank_games`,`total_rank_games`,`daymonth` FROM `eBG_main` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(5, 4, temp)
		if (save == true) {
			time_t clock;
			struct tm *t;
			int today;
			
			time(&clock);
			t = localtime(&clock);
			today = ((t->tm_mon+1)*100)+t->tm_mday;
			
			BG_CREATE_DATA_CHAR(bgd, common, bg_data_main, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_POINTS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_RANKED_POINTS, temp[temp_index], temp_index, int);
			if (temp[4] != today) {
				SET_VARIABLE_REPLACE(data_temp, BG_RANKED_GAMES, temp[temp_index], temp_index, int);
			} else {
				SET_VARIABLE_REPLACE(data_temp, BG_RANKED_GAMES, 0, temp_index, int);
			}
			temp_index = 3;
			SET_VARIABLE_REPLACE(data_temp, BG_TOTAL_RANKED_GAMES, temp[temp_index], temp_index, int);

#ifndef EBG_MAP
			if (!use_sql) {
				bg_save_common(esdb, sql_handle, char_id, NULL, save_type & EBG_SAVE_COMMON);
			}
#endif
		}
	}	
	ebg_free_sql_query(sql_handle);

	// Other Common Data
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_COMMON_STATS)), sql_handle, "SELECT `wins`,`loss`,`tie` FROM `eBG_main` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(3, -1, temp)
		if (save_ == true) {
			BG_CREATE_DATA_CHAR(bgd, common, bg_data_main, 3)
			SET_VARIABLE_REPLACE(data_temp, BG_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_TIE, temp[temp_index], temp_index, int);
#ifndef EBG_MAP
			if (!use_sql) {
				bg_save_common(esdb, sql_handle, char_id, NULL, save_type & EBG_SAVE_COMMON_STATS);
			}
#endif
		}
	}	
	ebg_free_sql_query(sql_handle);

	///< Ctf
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_CTF)), sql_handle, "SELECT `taken`,`onhand`,`drops`,`wins`,`loss`,`tie` FROM `eBG_ctf` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(6, 2, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, ctf, ctf_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, CTF_CAPTURED, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, CTF_ONHAND, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, CTF_DROPPED, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 3;
			BG_CREATE_DATA_CHAR(bgd, ctf, ctf_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_CTF_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_CTF_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_CTF_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_ctf(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< Boss 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_BOSS)), sql_handle, "SELECT `killed`,`flag`,`wins`,`loss`,`tie` FROM `eBG_boss` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(5, 1, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, boss, boss_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BOSS_KILLED, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BOSS_NEUT_FLAG, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 2;
			BG_CREATE_DATA_CHAR(bgd, boss, boss_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_BOSS_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_BOSS_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_BOSS_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_boss(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< TI 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_TI)), sql_handle, "SELECT `skulls`,`wins`,`loss`,`tie` FROM `eBG_ti` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(4, 0, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, ti, ti_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_TI_SKULL, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 1;
			BG_CREATE_DATA_CHAR(bgd, ti, ti_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_TI_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_TI_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_TI_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_ti(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< EoS 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_EOS)), sql_handle, "SELECT `flags`,`bases`,`wins`,`loss`,`tie` FROM `eBG_eos` WHERE char_id='%d'", &char_id, fd)) {
		temp_index = 0;
		GET_SAVE_SQL_DATA_(5, 1, temp)
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, eos, eos_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_EOS_FLAG, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_EOS_BASE, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 2;
			BG_CREATE_DATA_CHAR(bgd, eos, eos_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_EOS_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_EOS_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_EOS_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_eos(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< TDM 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_TD)), sql_handle, "SELECT `kills`,`deaths`,`wins`,`loss`,`tie` FROM `eBG_tdm` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(5, 1, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, td, tdm_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_TD_KILL, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_TD_DIE, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 2;
			BG_CREATE_DATA_CHAR(bgd, td, tdm_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_TD_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_TD_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_TD_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_tdm(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< SC 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_SC)), sql_handle, "SELECT `steals`,`captures`,`drops`,`wins`,`loss`,`tie` FROM `eBG_sc` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(6, 2, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, sc, sc_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_SC_STOLE, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_SC_CAPTURED, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_SC_DROPPED, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 3;
			BG_CREATE_DATA_CHAR(bgd, sc, sc_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_SC_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_SC_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_SC_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_sc(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< Conquest 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_CONQ)), sql_handle, "SELECT `emperium`,`barricade`,`guardian`,`wins`,`loss` FROM `eBG_conq` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(5, 2, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, conq, conq_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_CONQ_EMPERIUM, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_CONQ_BARRICADE, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_CONQ_GUARDIAN_S, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 3;
			BG_CREATE_DATA_CHAR(bgd, conq, conq_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_CONQ_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_CONQ_LOSS, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_conq(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< Rush 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_RUSH)), sql_handle, "SELECT `emp_captured`,`wins`,`loss` FROM `eBG_rush` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(3, 0, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, rush, rush_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_RUSH_EMPERIUM, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 1;
			BG_CREATE_DATA_CHAR(bgd, rush, rush_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_RUSH_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_RUSH_LOSS, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_rush(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
		
	}
	ebg_free_sql_query(sql_handle);

	///< DOM 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_DOM)), sql_handle, "SELECT `off_kill`,`def_kill`,`bases`,`wins`,`loss`,`tie` FROM `eBG_dom` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(5, 2, temp)
		temp_index = 0;
		if (save == true) {
			BG_CREATE_DATA_CHAR(bgd, dom, dom_bg_data, 1);
			SET_VARIABLE_REPLACE(data_temp, BG_DOM_OFF_KILL, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_DOM_DEF_KILL, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_DOM_BASE, temp[temp_index], temp_index, int);
		}
		if (save_ == true) {
			temp_index = 3;
			BG_CREATE_DATA_CHAR(bgd, dom, dom_bg_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_DOM_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_DOM_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_DOM_TIE, temp[temp_index], temp_index, int);
		}
#ifndef EBG_MAP
		if (!use_sql && (save || save_)) {
			bg_save_dom(esdb, sql_handle, char_id, NULL, save_type);
		}
#endif
	}
	ebg_free_sql_query(sql_handle);

	///< Leader 
	if (ebg_execute_sql_query(temp, temp64, use_sql, &save, &save_, ((save_type)&(EBG_SAVE_LEADER)), sql_handle, "SELECT `wins`,`loss`,`tie` FROM `eBG_leader` WHERE char_id='%d'", &char_id, fd)) {
		GET_SAVE_SQL_DATA_(3, 2, temp)
		temp_index = 0;
		if (save == true || save_ == true) {	// only save is needed
			BG_CREATE_DATA_CHAR(bgd, leader, bg_status_data, 3);
			SET_VARIABLE_REPLACE(data_temp, BG_LEADER_WON, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_LEADER_LOSS, temp[temp_index], temp_index, int);
			SET_VARIABLE_REPLACE(data_temp, BG_LEADER_TIE, temp[temp_index], temp_index, int);
#ifndef EBG_MAP
			if (!use_sql) {
				bg_save_leader(esdb, sql_handle, char_id, NULL, save_type);
			}
#endif
		}
	}
	ebg_free_sql_query(sql_handle);
#undef GET_SAVE_SQL_DATA_
#ifdef EBG_RANKING
	#undef GET_SAVE_SQL_DATA
#endif
	eShowDebug("Ended");
}

#define CONST_TO_VAL(high_const) values[high_const-sub_with]

/**
 * Saves Common Data of Player(BG)
 * @param sd Map_Session_Data
 * @param data sd_p_data
 **/
#ifdef EBG_MAP
void bg_save_common(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_common(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	int i;
	int values[6];	///< RankPoints, Points, Total Ranked Games, 2 UnRequired..., Ranked Games (Also Leader Win/Loss/Tie)
	int sub_with = BG_POINTS;
	if (save_type & EBG_SAVE_COMMON) {
		time_t clock;
		struct tm *t;
		int today;
		
		time(&clock);
		t = localtime(&clock);
		today = ((t->tm_mon+1)*100)+t->tm_mday;

		for (i = BG_POINTS; i <= BG_RANKED_GAMES; i++) {
			if (i > BG_TOTAL_RANKED_GAMES && i <= COMMON_MAX)
				continue;
#ifdef EBG_MAP
			values[i-BG_POINTS] = get_variable_(sd, i, false, 0);
#else
			values[i-BG_POINTS] = get_variable_(esdb, i, false, 0);
#endif
		}
		
		/// Save Points Data
		if (SQL_ERROR == SQL->Query(mysql_handle,
				"INSERT INTO `eBG_main` (`char_id`,`points`,`rank_points`,`rank_games`,`total_rank_games`,`daymonth`) VALUES ('%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `points`=VALUES(`points`), `rank_points`=VALUES(`rank_points`), `rank_games`=VALUES(`rank_games`), `total_rank_games`=VALUES(`total_rank_games`), `daymonth`=VALUES(`daymonth`)",
				char_id, CONST_TO_VAL(BG_POINTS), CONST_TO_VAL(BG_RANKED_POINTS), CONST_TO_VAL(BG_RANKED_GAMES), CONST_TO_VAL(BG_TOTAL_RANKED_GAMES), today))
			Sql_ShowDebug(mysql_handle);
	}
	if (save_type & EBG_SAVE_COMMON_STATS) {
		sub_with = BG_WON;
		for (i = BG_WON; i <= BG_LOSS; i++) {
#ifdef EBG_MAP
			values[i-BG_WON] = get_variable_(sd, i, false, 0);
#else
			values[i-BG_WON] = get_variable_(esdb, i, false, 0);
#endif
		}
		
		/// Save Points Data
		if (SQL_ERROR == SQL->Query(mysql_handle,
				"INSERT INTO `eBG_main` (`char_id`,`wins`,`loss`,`tie`) VALUES ('%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
				char_id, CONST_TO_VAL(BG_WON), CONST_TO_VAL(BG_LOSS), CONST_TO_VAL(BG_TIE)))
			Sql_ShowDebug(mysql_handle);
	}
}

/**
 * Saves Leader Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_leader(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_leader(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	int i;
	int values[3];	///< Leader Won/Loss/Tie
	int sub_with = BG_LEADER_WON;
	
	/// Save Leader Stats 
	for (i = BG_LEADER_WON; i <= BG_LEADER_TIE; i++) {
#ifdef EBG_MAP
		values[i-BG_LEADER_WON] = get_variable_(sd, i, false, 0);
#else
		values[i-BG_LEADER_WON] = get_variable_(esdb, i, false, 0);
#endif
	}
	if (SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_leader` VALUES ('%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id, CONST_TO_VAL(BG_LEADER_WON), CONST_TO_VAL(BG_LEADER_LOSS), CONST_TO_VAL(BG_LEADER_TIE)))
		Sql_ShowDebug(mysql_handle);
}

/**
 * Saves CaptureTheFlag Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_ctf(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_ctf(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
#if defined(EBG_MAP) && defined(CHAR_SERVER_SAVE)
	ShowError("bg_save_ctf: EBG_MAP and CHAR_SERVER_SAVE, both defined.\n");
	return;
#else
	EBG_SAVE_DATA_INIT(CTF_CAPTURED, BG_CTF_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_ctf` VALUES ('%d','%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `taken`=VALUES(`taken`), `onhand`=VALUES(`onhand`), `drops`=VALUES(`drops`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id, CONST_TO_VAL(BG_CTF_WON), CONST_TO_VAL(BG_CTF_LOSS), CONST_TO_VAL(BG_CTF_TIE), CONST_TO_VAL(CTF_CAPTURED), CONST_TO_VAL(CTF_ONHAND),  CONST_TO_VAL(CTF_DROPPED)))
		Sql_ShowDebug(mysql_handle);
	return;
#endif
}

/**
 * Saves Tierra Boss Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_boss(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_boss(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BOSS_NEUT_FLAG, BG_BOSS_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_boss` VALUES ('%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `flag`=VALUES(`flag`), `killed`=VALUES(`killed`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id,
			CONST_TO_VAL(BG_BOSS_WON), CONST_TO_VAL(BG_BOSS_LOSS), CONST_TO_VAL(BG_BOSS_TIE), CONST_TO_VAL(BOSS_NEUT_FLAG), CONST_TO_VAL(BOSS_KILLED)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves Tierra Triple Infierno Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_ti(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_ti(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_TI_SKULL, BG_TI_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_ti` VALUES ('%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `skulls`=VALUES(`skulls`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id,
			CONST_TO_VAL(BG_TI_WON), CONST_TO_VAL(BG_TI_LOSS), CONST_TO_VAL(BG_TI_TIE), CONST_TO_VAL(BG_TI_SKULL)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves Tierra EyeOfStorm Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_eos(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_eos(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_EOS_FLAG, BG_EOS_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_eos` VALUES ('%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `flags`=VALUES(`flags`), `bases`=VALUES(`bases`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id,
			CONST_TO_VAL(BG_EOS_WON), CONST_TO_VAL(BG_EOS_LOSS), CONST_TO_VAL(BG_EOS_TIE), CONST_TO_VAL(BG_EOS_FLAG), CONST_TO_VAL(BG_EOS_BASE)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves Tierra TeamDeathMatch Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_tdm(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_tdm(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_TD_DIE, BG_TD_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_tdm` VALUES ('%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `kills`=VALUES(`kills`), `deaths`=VALUES(`deaths`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id,
			CONST_TO_VAL(BG_TD_WON), CONST_TO_VAL(BG_TD_LOSS), CONST_TO_VAL(BG_TD_TIE), CONST_TO_VAL(BG_TD_KILL), CONST_TO_VAL(BG_TD_DIE)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves Flavius StoneControl Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_sc(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_sc(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_SC_CAPTURED, BG_SC_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_sc` VALUES ('%d','%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `steals`=VALUES(`steals`), `captures`=VALUES(`captures`), `drops`=VALUES(`drops`) , `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id,
			CONST_TO_VAL(BG_SC_WON), CONST_TO_VAL(BG_SC_LOSS), CONST_TO_VAL(BG_SC_TIE), CONST_TO_VAL(BG_SC_STOLE), CONST_TO_VAL(BG_SC_CAPTURED), CONST_TO_VAL(BG_SC_DROPPED)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves Conquest Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_conq(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_conq(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_CONQ_GUARDIAN_S, BG_CONQ_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_conq` VALUES ('%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `emperium`=VALUES(`emperium`), `barricade`=VALUES(`barricade`), `guardian`=VALUES(`guardian`)",
			char_id,
			CONST_TO_VAL(BG_CONQ_WON), CONST_TO_VAL(BG_CONQ_LOSS), CONST_TO_VAL(BG_CONQ_EMPERIUM), CONST_TO_VAL(BG_CONQ_BARRICADE), CONST_TO_VAL(BG_CONQ_GUARDIAN_S)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves Rush Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_rush(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_rush(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_RUSH_EMPERIUM, BG_RUSH_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_rush` VALUES ('%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `emp_captured`=VALUES(`emp_captured`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`)",
			char_id,
			CONST_TO_VAL(BG_RUSH_WON), CONST_TO_VAL(BG_RUSH_LOSS), CONST_TO_VAL(BG_RUSH_EMPERIUM)))
		Sql_ShowDebug(mysql_handle);
	return;
}


/**
 * Saves Tierra Domination Data of Player(BG)
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_dom(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_dom(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	EBG_SAVE_DATA_INIT(BG_DOM_DEF_KILL, BG_DOM_WON, data, int, MAP_IS_NONE);
	if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `eBG_dom` VALUES ('%d','%d','%d','%d','%d','%d','%d') ON DUPLICATE KEY UPDATE `off_kill`=VALUES(`off_kill`), `def_kill`=VALUES(`def_kill`), `bases`=VALUES(`bases`), `wins`=VALUES(`wins`), `loss`=VALUES(`loss`), `tie`=VALUES(`tie`)",
			char_id,
			CONST_TO_VAL(BG_DOM_WON), CONST_TO_VAL(BG_DOM_LOSS), CONST_TO_VAL(BG_DOM_TIE), CONST_TO_VAL(BG_DOM_OFF_KILL), CONST_TO_VAL(BG_DOM_DEF_KILL), CONST_TO_VAL(BG_DOM_BASE)))
		Sql_ShowDebug(mysql_handle);
	return;
}

/**
 * Saves BG Ranking of Player
 * @param char_id CharacterID
 * @param data sd_p_data
 * @param save_type Save Constant
 **/
#ifdef EBG_MAP
void bg_save_ranking(struct map_session_data *sd, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#else
void bg_save_ranking(struct ebg_save_data *esdb, struct Sql *mysql_handle, int char_id, struct sd_p_data *data, int64 save_type)
#endif
{
	int map_type = MAP_IS_NONE;
	const char* tbl_name;
	if (save_type & EBG_SAVE_BG_RANK_ALL) {
		map_type = MAP_IS_BG;
		tbl_name = "eBG_bg_rankings";
	} else {
		map_type = MAP_IS_WOE;
		tbl_name = "eBG_woe_rankings";
	}
	/// Common Data
	if (save_type&(EBG_SAVE_BG_RANK_DMG|EBG_SAVE_WOE_RANK_DMG)) {
		EBG_SAVE_DATA_INIT(RANKING_DAMAGE_RECEIVED, RANKING_MAX_DAMAGE, data, uint64, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `max_damage`, `damage`, `damage_received`) VALUES ('%d','%"PRIu64"','%"PRIu64"','%"PRIu64"') ON DUPLICATE KEY UPDATE `max_damage` = VALUES(`max_damage`), `damage` = VALUES(`damage`), `damage_received` = VALUES(`damage_received`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_MAX_DAMAGE), CONST_TO_VAL(RANKING_DAMAGE_DEALT), CONST_TO_VAL(RANKING_DAMAGE_RECEIVED)))
			Sql_ShowDebug(mysql_handle);
	}
	if (save_type&(EBG_SAVE_BG_RANK_HEAL|EBG_SAVE_WOE_RANK_HEAL)) {
		int loop_ = 0;
		for (loop_ = 0; loop_ <= 1; loop_++) {
			switch(loop_) {
				case 0: { /// Heal(uint64)
					EBG_SAVE_DATA_INIT(RANKING_HEAL_ENEMY, RANKING_HEAL_TEAM, data, uint64, map_type);
					if (save && SQL_ERROR == SQL->Query(mysql_handle,
						"INSERT INTO `%s` (`char_id`, `healing`, `healing_fail`) VALUES ('%d','%"PRIu64"','%"PRIu64"') ON DUPLICATE KEY UPDATE `healing` = VALUES(`healing`), `healing_fail` = VALUES(`healing_fail`)",
						tbl_name, char_id,
						CONST_TO_VAL(RANKING_HEAL_TEAM), CONST_TO_VAL(RANKING_HEAL_ENEMY)))
						Sql_ShowDebug(mysql_handle);
					}
					break;
				case 1: { /// Other Healing(unsigned int)
					EBG_SAVE_DATA_INIT(RANKING_SP_POTION, RANKING_SP_USED, data, unsigned int, map_type);
					if (save && SQL_ERROR == SQL->Query(mysql_handle,
						"INSERT INTO `%s` (`char_id`, `sp_used`, `hp_potions`, `sp_potions`) VALUES ('%d','%u','%u','%u') ON DUPLICATE KEY UPDATE `sp_used` = VALUES(`sp_used`), `hp_potions` = VALUES(`hp_potions`), `sp_potions` = VALUES(`sp_potions`)",
						tbl_name, char_id,
						CONST_TO_VAL(RANKING_SP_USED), CONST_TO_VAL(RANKING_HP_POTION), CONST_TO_VAL(RANKING_SP_POTION)))
						Sql_ShowDebug(mysql_handle);
					}
					break;
			}
		}
	}
	if (save_type&(EBG_SAVE_BG_RANK_ITEM|EBG_SAVE_WOE_RANK_ITEM)) {
		EBG_SAVE_DATA_INIT(RANKING_ACID_DEMONSTRATION_F, RANKING_RED_GEMSTONE, data, unsigned int, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `red_gemstones`, `blue_gemstones`, `yellow_gemstones`, `poison_bottles`, `acid_demostration`, `acid_demostration_fail`) VALUES ('%d','%u','%u','%u','%u','%u', '%u') ON DUPLICATE KEY UPDATE `red_gemstones` = VALUES(`red_gemstones`), `blue_gemstones` = VALUES(`blue_gemstones`), `yellow_gemstones` = VALUES(`yellow_gemstones`), `poison_bottles` = VALUES(`poison_bottles`), `acid_demostration` = VALUES(`acid_demostration`), `acid_demostration_fail` = VALUES(`acid_demostration_fail`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_RED_GEMSTONE), CONST_TO_VAL(RANKING_BLUE_GEMSTONE), CONST_TO_VAL(RANKING_YELLOW_GEMSTONE), CONST_TO_VAL(RANKING_POISON_BOTTLE), CONST_TO_VAL(RANKING_ACID_DEMONSTRATION), CONST_TO_VAL(RANKING_ACID_DEMONSTRATION_F)))
			Sql_ShowDebug(mysql_handle);
	}
	if (save_type&(EBG_SAVE_BG_RANK_SKILL|EBG_SAVE_WOE_RANK_SKILL)) {
		EBG_SAVE_DATA_INIT(RANKING_ZENY, RANKING_SUPPORT_SKILL_TEAM, data, unsigned int, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `support_skills`, `support_skills_fail`, `spiritb_used`, `zeny_used`) VALUES ('%d','%u','%u','%u','%u') ON DUPLICATE KEY UPDATE `support_skills` = VALUES(`support_skills`), `support_skills_fail` = VALUES(`support_skills_fail`), `spiritb_used` = VALUES(`spiritb_used`), `zeny_used` = VALUES(`zeny_used`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_SUPPORT_SKILL_TEAM), CONST_TO_VAL(RANKING_SUPPORT_SKILL_ENEMY), CONST_TO_VAL(RANKING_SPIRITB), CONST_TO_VAL(RANKING_ZENY)))
			Sql_ShowDebug(mysql_handle);
	}
	if (save_type&(EBG_SAVE_BG_RANK_MISC|EBG_SAVE_WOE_RANK_MISC)) {
		EBG_SAVE_DATA_INIT(RANKING_SCORE, RANKING_KILLS, data, unsigned int, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `ammo_used`, `kills`, `deaths`, `score`) VALUES ('%d','%u','%u','%u','%u') ON DUPLICATE KEY UPDATE `ammo_used` = VALUES(`ammo_used`), `kills` = VALUES(`kills`), `deaths` = VALUES(`deaths`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_AMMOS), CONST_TO_VAL(RANKING_KILLS), CONST_TO_VAL(RANKING_DEATHS), CONST_TO_VAL(RANKING_SCORE)))
			Sql_ShowDebug(mysql_handle);
	}
	/// BG Data
	if (save_type&(EBG_SAVE_BG_RANK_DMG2)) {
		EBG_SAVE_DATA_INIT(RANKING_BG_BOSS_DMG, RANKING_BG_BOSS_DMG, data, uint64, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `boss_damage`) VALUES ('%d','%"PRIu64"') ON DUPLICATE KEY UPDATE `boss_damage` = VALUES(`boss_damage`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_BG_BOSS_DMG)))
			Sql_ShowDebug(mysql_handle);
	}
	/// WoE Data
	if (save_type&(EBG_SAVE_WOE_RANK_DMG2)) {
		EBG_SAVE_DATA_INIT(RANKING_WOE_GSTONE_DAMAGE, RANKING_WOE_EMPERIUM_DAMAGE, data, uint64, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `emperium_dmg`, `barricade_dmg`, `guardian_dmg`, `gstone_dmg`) VALUES ('%d','%"PRIu64"','%"PRIu64"','%"PRIu64"','%"PRIu64"') ON DUPLICATE KEY UPDATE `emperium_dmg` = VALUES(`emperium_dmg`), `barricade_dmg` = VALUES(`barricade_dmg`), `guardian_dmg` = VALUES(`guardian_dmg`), `gstone_dmg` = VALUES(`gstone_dmg`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_WOE_EMPERIUM_DAMAGE), CONST_TO_VAL(RANKING_WOE_BARRICADE_DAMAGE), CONST_TO_VAL(RANKING_WOE_GUARDIAN_DAMAGE), CONST_TO_VAL(RANKING_WOE_GSTONE_DAMAGE)))
			Sql_ShowDebug(mysql_handle);
	}
	if (save_type&(EBG_SAVE_WOE_RANK_MISC2)) {
		EBG_SAVE_DATA_INIT(RANKING_WOE_GSTONE, RANKING_WOE_EMPERIUM, data, unsigned int, map_type);
		if (save && SQL_ERROR == SQL->Query(mysql_handle,
			"INSERT INTO `%s` (`char_id`, `emperium`, `barricade`, `guardian`, `gstone`) VALUES ('%d','%u','%u','%u','%u') ON DUPLICATE KEY UPDATE `emperium` = VALUES(`emperium`), `barricade` = VALUES(`barricade`), `guardian` = VALUES(`guardian`), `gstone` = VALUES(`gstone`)",
			tbl_name, char_id,
			CONST_TO_VAL(RANKING_WOE_EMPERIUM), CONST_TO_VAL(RANKING_WOE_BARRICADE), CONST_TO_VAL(RANKING_WOE_GUARDIAN), CONST_TO_VAL(RANKING_WOE_GSTONE)))
			Sql_ShowDebug(mysql_handle);
	}
	return;
}

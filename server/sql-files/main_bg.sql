/*
DROP TABLE IF EXISTS `char_bg`;
CREATE TABLE `char_bg` (
  `char_id` INT(11) NOT NULL,
  `win` INT(11) NOT NULL DEFAULT '0',
  `lost` INT(11) NOT NULL DEFAULT '0',
  `tie` INT(11) NOT NULL DEFAULT '0',
  `deserter` INT(11) NOT NULL DEFAULT '0',
  `score` INT(11) NOT NULL DEFAULT '0',
  `rank_games` INT(11) NOT NULL DEFAULT '0',
PRIMARY KEY  (`char_id`)
) ENGINE=InnoDB;
*/

DROP TABLE IF EXISTS `char_kill_log`;
DROP TABLE IF EXISTS `eBG_bg_rankings`;
DROP TABLE IF EXISTS `eBG_woe_rankings`;
DROP TABLE IF EXISTS `eBG_dom`;
DROP TABLE IF EXISTS `eBG_rush`;
DROP TABLE IF EXISTS `eBG_conq`;
DROP TABLE IF EXISTS `eBG_sc`;
DROP TABLE IF EXISTS `eBG_tdm`;
DROP TABLE IF EXISTS `eBG_eos`;
DROP TABLE IF EXISTS `eBG_ti`;
DROP TABLE IF EXISTS `eBG_boss`;
DROP TABLE IF EXISTS `eBG_ctf`;
DROP TABLE IF EXISTS `eBG_leader`;
DROP TABLE IF EXISTS `eBG_main`;

CREATE TABLE `char_kill_log` (
	`id` INT(11) NOT NULL AUTO_INCREMENT,
	`time` DATETIME NOT NULL DEFAULT '0000-00-00 00:00:00',
	`killer_name` VARCHAR(25) NOT NULL,
	`killer_id` INT(11) NOT NULL,
	`killed_name` VARCHAR(25) NOT NULL,
	`killed_id` INT(11) NOT NULL,
	`map` VARCHAR(16) NOT NULL DEFAULT '',
	`skill` INT(11) NOT NULL DEFAULT '0',
	`map_type` TINYINT(2) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`id`),
	INDEX `killer_id` (`killer_id`),
	INDEX `killed_id` (`killed_id`)
) ENGINE=MyISAM;

CREATE TABLE IF NOT EXISTS `eBG_bg_rankings` (
	`char_id` INT(11) NOT NULL,
	`score` INT(11) UNSIGNED NOT NULL,
	`max_damage` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`damage` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`damage_received` BIGINT(20) NOT NULL DEFAULT '0',
	`boss_damage` BIGINT(20) NOT NULL DEFAULT '0',
	`kills` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`deaths` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`healing` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`healing_fail` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`sp_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`hp_potions` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`sp_potions` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`red_gemstones` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`blue_gemstones` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`yellow_gemstones` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`poison_bottles` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`acid_demostration` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`acid_demostration_fail` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`support_skills` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`support_skills_fail` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`spiritb_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`ammo_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`zeny_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_woe_rankings` (
	`char_id` INT(11) NOT NULL,
	`score` INT(11) UNSIGNED NOT NULL,
	`max_damage` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`damage` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`damage_received` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`emperium` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`barricade` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`guardian` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`gstone` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`emperium_dmg` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`barricade_dmg` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`guardian_dmg` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`gstone_dmg` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`kills` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`deaths` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`healing` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`healing_fail` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`sp_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`hp_potions` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`sp_potions` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`red_gemstones` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`blue_gemstones` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`yellow_gemstones` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`poison_bottles` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`acid_demostration` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`acid_demostration_fail` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`support_skills` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`support_skills_fail` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`spiritb_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`ammo_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	`zeny_used` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_dom` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`off_kill` INT(11) NOT NULL DEFAULT '0',
	`def_kill` INT(11) NOT NULL DEFAULT '0',
	`bases` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);


CREATE TABLE IF NOT EXISTS `eBG_rush` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`emp_captured` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_conq` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`emperium` INT(11) NOT NULL DEFAULT '0',
	`barricade` INT(11) NOT NULL DEFAULT '0',
	`guardian` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_sc` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`steals` INT(11) NOT NULL DEFAULT '0',
	`captures` INT(11) NOT NULL DEFAULT '0',
	`drops` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_tdm` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`kills` INT(11) NOT NULL DEFAULT '0',
	`deaths` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_eos` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`flags` INT(11) NOT NULL DEFAULT '0',
	`bases` INT(11) NOT NULL DEFAULT '0',	
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_ti` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`skulls` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_boss` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`flag` INT(11) NOT NULL DEFAULT '0',
	`killed` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_ctf` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`taken` INT(11) NOT NULL DEFAULT '0',
	`onhand` INT(11) NOT NULL DEFAULT '0',
	`drops` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_leader` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_main` (
	`char_id` INT(11) NOT NULL,
	`wins` INT(11) NOT NULL DEFAULT '0',
	`loss` INT(11) NOT NULL DEFAULT '0',
	`tie` INT(11) NOT NULL DEFAULT '0',
	`points` INT(11) NOT NULL DEFAULT '0',
	`rank_points` INT(11) NOT NULL DEFAULT '0',
	`rank_games` INT(11) NOT NULL DEFAULT '0',
	`total_rank_games` INT(11) NOT NULL DEFAULT '0',
	`daymonth` MEDIUMINT(5) NOT NULL DEFAULT '0',
	PRIMARY KEY  (`char_id`)
);

CREATE TABLE IF NOT EXISTS `eBG_member` (
  `guild_id` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  `account_id` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  `char_id` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  `hair` TINYINT(6) UNSIGNED NOT NULL DEFAULT '0',
  `hair_color` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',
  `gender` TINYINT(6) UNSIGNED NOT NULL DEFAULT '0',
  `class` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',
  `lv` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',
  `exp` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
  `exp_payper` TINYINT(11) UNSIGNED NOT NULL DEFAULT '0',
  `online` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',
  `position` TINYINT(6) UNSIGNED NOT NULL DEFAULT '0',
  `name` VARCHAR(24) NOT NULL DEFAULT '',
  PRIMARY KEY (`guild_id`,`char_id`),
  KEY `char_id` (`char_id`)
) ENGINE=MyISAM;

REPLACE INTO `guild` VALUES (901,'Blue Team',150000,'General Guillaume',1,0,16,1,1,1,1,'Blue Team - eBG','Blue Team - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (902,'Red Team',150000,'Prince Croix',1,0,16,1,1,1,1,'Red Team - eBG','Red Team - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (903,'Green Team',150000,'Mercenary',1,0,16,1,1,1,1,'Green Team - eBG','Green Team - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (904,'Team 1',0,'Team 1',1,0,16,1,1,1,1,'Team 1 - eBG','Team 1 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (905,'Team 2',0,'Team 2',1,0,16,1,1,1,1,'Team 2 - eBG','Team 2 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (906,'Team 3',0,'Team 3',1,0,16,1,1,1,1,'Team 3 - eBG','Team 3 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (907,'Team 4',0,'Team 4',1,0,16,1,1,1,1,'Team 4 - eBG','Team 4 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (908,'Team 5',0,'Team 5',1,0,16,1,1,1,1,'Team 5 - eBG','Team 5 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (909,'Team 6',0,'Team 6',1,0,16,1,1,1,1,'Team 6 - eBG','Team 6 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (910,'Team 7',0,'Team 7',1,0,16,1,1,1,1,'Team 7 - eBG','Team 7 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (911,'Team 8',0,'Team 8',1,0,16,1,1,1,1,'Team 8 - eBG','Team 8 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (912,'Team 9',0,'Team 9',1,0,16,1,1,1,1,'Team 9 - eBG','Team 9 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (913,'Team 10',0,'Team 10',1,0,16,1,1,1,1,'Team 10 - eBG','Team 10 - eBG',0,0,NULL);

REPLACE INTO `guild_position` VALUES (901,0,'Blue Team Leader',0,0);
REPLACE INTO `guild_position` VALUES (901,1,'Blue Team',0,0);
REPLACE INTO `guild_position` VALUES (902,0,'Red Team Leader',0,0);
REPLACE INTO `guild_position` VALUES (902,1,'Red Team',0,0);
REPLACE INTO `guild_position` VALUES (903,0,'Green Team Leader',0,0);
REPLACE INTO `guild_position` VALUES (903,1,'Green Team',0,0);
REPLACE INTO `guild_position` VALUES (904,0,'Team 1 Leader',0,0);
REPLACE INTO `guild_position` VALUES (904,1,'Team 1',0,0);
REPLACE INTO `guild_position` VALUES (905,0,'Team 2 Leader',0,0);
REPLACE INTO `guild_position` VALUES (905,1,'Team 2',0,0);
REPLACE INTO `guild_position` VALUES (906,0,'Team 3 Leader',0,0);
REPLACE INTO `guild_position` VALUES (906,1,'Team 3',0,0);
REPLACE INTO `guild_position` VALUES (907,0,'Team 4 Leader',0,0);
REPLACE INTO `guild_position` VALUES (907,1,'Team 4',0,0);
REPLACE INTO `guild_position` VALUES (908,0,'Team 5 Leader',0,0);
REPLACE INTO `guild_position` VALUES (908,1,'Team 5',0,0);
REPLACE INTO `guild_position` VALUES (909,0,'Team 6 Leader',0,0);
REPLACE INTO `guild_position` VALUES (909,1,'Team 6',0,0);
REPLACE INTO `guild_position` VALUES (910,0,'Team 7 Leader',0,0);
REPLACE INTO `guild_position` VALUES (910,1,'Team 7',0,0);
REPLACE INTO `guild_position` VALUES (911,0,'Team 8 Leader',0,0);
REPLACE INTO `guild_position` VALUES (911,1,'Team 8',0,0);
REPLACE INTO `guild_position` VALUES (912,0,'Team 9 Leader',0,0);
REPLACE INTO `guild_position` VALUES (912,1,'Team 9',0,0);
REPLACE INTO `guild_position` VALUES (913,0,'Team 10 Leader',0,0);
REPLACE INTO `guild_position` VALUES (913,1,'Team 10',0,0);

REPLACE INTO `guild_skill` VALUES (901,10000,1);
REPLACE INTO `guild_skill` VALUES (901,10001,1);
REPLACE INTO `guild_skill` VALUES (901,10002,1);
REPLACE INTO `guild_skill` VALUES (901,10003,3);
REPLACE INTO `guild_skill` VALUES (901,10004,10);
REPLACE INTO `guild_skill` VALUES (901,10005,0);
REPLACE INTO `guild_skill` VALUES (901,10006,5);
REPLACE INTO `guild_skill` VALUES (901,10007,5);
REPLACE INTO `guild_skill` VALUES (901,10008,5);
REPLACE INTO `guild_skill` VALUES (901,10009,5);
REPLACE INTO `guild_skill` VALUES (901,10010,1);
REPLACE INTO `guild_skill` VALUES (901,10011,3);
REPLACE INTO `guild_skill` VALUES (901,10012,1);
REPLACE INTO `guild_skill` VALUES (901,10013,1);
REPLACE INTO `guild_skill` VALUES (901,10014,1);

REPLACE INTO `guild_skill` VALUES (902,10000,1);
REPLACE INTO `guild_skill` VALUES (902,10001,1);
REPLACE INTO `guild_skill` VALUES (902,10002,1);
REPLACE INTO `guild_skill` VALUES (902,10003,3);
REPLACE INTO `guild_skill` VALUES (902,10004,10);
REPLACE INTO `guild_skill` VALUES (902,10005,0);
REPLACE INTO `guild_skill` VALUES (902,10006,5);
REPLACE INTO `guild_skill` VALUES (902,10007,5);
REPLACE INTO `guild_skill` VALUES (902,10008,5);
REPLACE INTO `guild_skill` VALUES (902,10009,5);
REPLACE INTO `guild_skill` VALUES (902,10010,1);
REPLACE INTO `guild_skill` VALUES (902,10011,3);
REPLACE INTO `guild_skill` VALUES (902,10012,1);
REPLACE INTO `guild_skill` VALUES (902,10013,1);
REPLACE INTO `guild_skill` VALUES (902,10014,1);

REPLACE INTO `guild_skill` VALUES (903,10000,1);
REPLACE INTO `guild_skill` VALUES (903,10001,1);
REPLACE INTO `guild_skill` VALUES (903,10002,1);
REPLACE INTO `guild_skill` VALUES (903,10003,3);
REPLACE INTO `guild_skill` VALUES (903,10004,10);
REPLACE INTO `guild_skill` VALUES (903,10005,0);
REPLACE INTO `guild_skill` VALUES (903,10006,5);
REPLACE INTO `guild_skill` VALUES (903,10007,5);
REPLACE INTO `guild_skill` VALUES (903,10008,5);
REPLACE INTO `guild_skill` VALUES (903,10009,5);
REPLACE INTO `guild_skill` VALUES (903,10010,1);
REPLACE INTO `guild_skill` VALUES (903,10011,3);
REPLACE INTO `guild_skill` VALUES (903,10012,1);
REPLACE INTO `guild_skill` VALUES (903,10013,1);
REPLACE INTO `guild_skill` VALUES (903,10014,1);
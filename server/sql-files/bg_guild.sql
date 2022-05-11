# SQL
# To be executed only if VIRT_GUILD is commented(in eBG_common.h)
# CAUTION: The script creates some permanent GUILD, change GuildID if they already exist
#          If guild ID is changed, also change following variable from eBG_common.h
#          `#define EBG_GUILDSTART 901`

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

REPLACE INTO `guild` VALUES (901,'Blue Team',150000,'General Guillaume',1,0,16,1,1,1,1,0,'Blue Team - eBG','Blue Team - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (902,'Red Team',150000,'Prince Croix',1,0,16,1,1,1,1,0,'Red Team - eBG','Red Team - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (903,'Green Team',150000,'Mercenary',1,0,16,1,1,1,1,0,'Green Team - eBG','Green Team - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (904,'Team 1',0,'Team 1',1,0,16,1,1,1,1,0,'Team 1 - eBG','Team 1 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (905,'Team 2',0,'Team 2',1,0,16,1,1,1,1,0,'Team 2 - eBG','Team 2 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (906,'Team 3',0,'Team 3',1,0,16,1,1,1,1,0,'Team 3 - eBG','Team 3 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (907,'Team 4',0,'Team 4',1,0,16,1,1,1,1,0,'Team 4 - eBG','Team 4 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (908,'Team 5',0,'Team 5',1,0,16,1,1,1,1,0,'Team 5 - eBG','Team 5 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (909,'Team 6',0,'Team 6',1,0,16,1,1,1,1,0,'Team 6 - eBG','Team 6 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (910,'Team 7',0,'Team 7',1,0,16,1,1,1,1,0,'Team 7 - eBG','Team 7 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (911,'Team 8',0,'Team 8',1,0,16,1,1,1,1,0,'Team 8 - eBG','Team 8 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (912,'Team 9',0,'Team 9',1,0,16,1,1,1,1,0,'Team 9 - eBG','Team 9 - eBG',0,0,NULL);
REPLACE INTO `guild` VALUES (913,'Team 10',0,'Team 10',1,0,16,1,1,1,1,0,'Team 10 - eBG','Team 10 - eBG',0,0,NULL);

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

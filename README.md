Build Status: [![Build Status](https://travis-ci.org/dastgirp/eBG.svg?branch=master)](https://travis-ci.org/dastgirp/eBG)

# Extended Battlegrounds for Hercules - PLUGIN #

Plugin for Extended Battleground for Hercules

Support Forum: http://herc.ws/board/topic/8814-release-extendedbg-for-hercules-with-eamod-bg-modes/

### Contributed By ###

* Dastgir/Hercules.
* jaBote/Hercules.

# Installation Steps #

### Client:
* Put the contents of resnametable.txt to GRF
* Put the contents of mapnametable.txt to GRF
* Optional:
    * To be done only if .bg_common[6] have 1st bit set and .bg_common[7] is custom Item 
    * Put contents of System/itemInfo.lua to your itemInfo.lua/lub

### Server:
* Conf:
    * Copy contents of conf/map/maps.conf at end of your maps.conf
    * Copy contents of conf/import/battle.conf in your herc/conf/import/battle.conf depending on your configuration
    * Copy conf/import/eBG.conf file in your herc/conf/import/ folder
* DB:
    * Copy db/emblems folder to your hercules/db/emblems folder
    * Copy contents of db/map_index.txt at end of your map_index.txt
    * Optional:
        * To be done only if .bg_common[6] have 1st bit set and .bg_common[7] is custom Item
            * Copy the contents of item_db.conf to your item_db2.conf
            * Copy the contents of item_group.conf to your (pre-)re/item_group.conf
* MapCache:
    * Copy all files in maps/ folder to "maps/re" or "maps/pre-re" depending on your configuration
* NPC:
    * Copy all files in npc/ folder to your herc/npc/ folder
    * Add this line at end of scripts_custom.conf:
        * @include "npc/scripts_eBG.conf"
* Plugins:
    * Copy all files in plugin folder to your herc/src/plugins folder
    * compile and enable 2 plugins:
        * ExtendedBG
        * ExtendedBG-char
    * (All other files are just includes)
* SQL:
    * Execute the file sql-files/bg_main.sql on your SQL Server
    * Optional (If VIRT_GUILD is commented from eBG_Common.h):
        * To be executed only IF VIRT_GUILD is commented from eBG_common.h
        * Execute the file sql-files/bg_guild.sql


Build Status: [![Build Status](https://travis-ci.org/dastgirp/eBG.svg?branch=master)](https://travis-ci.org/dastgirp/eBG)

# Extended Battlegrounds for Hercules - PLUGIN #

Plugin for Extended Battleground for Hercules

Support Forum: http://herc.ws/board/topic/8814-release-extendedbg-for-hercules-with-eamod-bg-modes/

### Contributed By ###

* Dastgir/Hercules.
* jaBote/Hercules.

# Installation Steps #

Client:
    1) Put the contents of resnametable.txt to GRF
    2) Put the contents of mapnametable.txt to GRF
    Optional:
    1) To be done only if .bg_common[6] have 1st bit set and .bg_common[7] is custom Item
        a) Put contents of System/itemInfo.lua to your itemInfo.lua/lub

Server:
    Conf:
        1) Copy contents of conf/map/maps.conf at end of your maps.conf
        2) Copy contents of conf/import/battle.conf in your herc/conf/import/battle.conf depending on your configuration
        3) Copy conf/import/eBG.conf file in your herc/conf/import/ folder
    DB:
        1) Copy db/emblems folder to your hercules/db/emblems folder
        2) Copy contents of db/map_index.txt at end of your map_index.txt
        Optional:
        1) To be done only if .bg_common[6] have 1st bit set and .bg_common[7] is custom Item
            a) Copy the contents of item_db.conf to your item_db2.conf
            b) Copy the contents of item_group.conf to your (pre-)re/item_group.conf
    MapCache:
        1) Copy all files in maps/ folder to "maps/re" or "maps/pre-re" depending on your configuration
    NPC:
        1) Copy all files in npc/ folder to your herc/npc/ folder
        2) Add this line at end of scripts_custom.conf:
            @include "npc/scripts_eBG.conf"
    Plugins:
        1) Copy all files in plugin folder to your herc/src/plugins folder
        2) compile and enable 2 plugins:
            ExtendedBG
            ExtendedBG-char
        (All other files are just includes)
    SQL:
        1) Execute the file sql-files/bg_main.sql on your SQL Server
        Optional(If VIRT_GUILD is commented from eBG_Common.h):
        1) To be executed only IF VIRT_GUILD is commented from eBG_common.h,
            a) Execute the file sql-files/bg_guild.sql


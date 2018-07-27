# Extended Battlegrounds for Hercules - PLUGIN #

Plugin for Extended Battleground for Hercules :D

### Currently Working On ###
* Optimization, BugFixing and 

### Contribution Guidelines ###

* Initial Script: jaBote
* Plugin: Dastgir
* Scripts LaterOn: Dastgir
* Optimization/ErrorFix/Test: Dastgir

### Contributed By ###

* Dastgir/Hercules.
* jaBote/Hercules.

# Installation Steps #

Client:
    Put the contents of resnametable.txt to GRF

Server:
    Conf:
        1) Copy contents of conf/map/maps.conf at end of your maps.conf
    DB:
        1) Copy db/emblems folder to your hercules/db/emblems folder
        2) Copy contents of db/map_index.txt at end of your map_index.txt
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
        1) Execute the file sql-files/main.sql on your SQL Server


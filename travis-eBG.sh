#!/bin/bash

# This file is part of Hercules.
# http://herc.ws - http://github.com/HerculesWS/Hercules
#
# Copyright (C) 2014-2015  Hercules Dev Team
#
# Hercules is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

MODE="$1"
script_dir="server/npc/eBG"
shift

function foo {
	for i in "$@"; do
		echo "> $i"
	done
}

function usage {
	echo "usage:"
	echo "    $0 createdb <dbname> [dbuser] [dbpassword]"
	echo "    $0 importdb <dbname> [dbuser] [dbpassword]"
	echo "    $0 build [configure args]"
	echo "    $0 test <dbname> [dbuser] [dbpassword]"
	echo "    $0 getrepo"
	exit 1
}

function aborterror {
	echo $@
	exit 1
}

function run_server {
	echo "Running: $1 --run-once $2"
	$1 --run-once $2 2>runlog.txt
	export errcode=$?
	export teststr=$(cat runlog.txt)
	if [[ -n "${teststr}" ]]; then
		echo "Errors found in running server $1."
		cat runlog.txt
		aborterror "Errors found in running server $1."
	else
		echo "No errors found for server $1."
	fi
	if [ ${errcode} -ne 0 ]; then
		echo "server $1 terminated with exit code ${errcode}"
		aborterror "Test failed"
	fi
}

case "$MODE" in
	createdb|importdb|test)
		DBNAME="$1"
		DBUSER="$2"
		DBPASS="$3"
		if [ -z "$DBNAME" ]; then
			usage
		fi
		if [ "$MODE" != "test" ]; then
			if [ -n "$DBUSER" ]; then
				DBUSER="-u $DBUSER"
			fi
			if [ -n "$DBPASS" ]; then
				DBPASS="-p$DBPASS"
			fi
		fi
		;;
esac

case "$MODE" in
	createdb)
		echo "Creating database $DBNAME..."
		mysql $DBUSER $DBPASS -e "create database $DBNAME;" || aborterror "Unable to create database."
		;;
	importdb)
		echo "Importing tables into $DBNAME..."
		mysql $DBUSER $DBPASS $DBNAME < sql-files/main.sql || aborterror "Unable to import main database."
		mysql $DBUSER $DBPASS $DBNAME < sql-files/logs.sql || aborterror "Unable to import logs database."
		mysql $DBUSER $DBPASS $DBNAME < server/sql-files/bg_main.sql || aborterror "Unable to import Main eBG database."
		mysql $DBUSER $DBPASS $DBNAME < server/sql-files/bg_guild.sql || aborterror "Unable to import Guild eBG database."
		;;
	build)
		(cd tools && ./validateinterfaces.py silent) || aborterror "Interface validation error."
		./configure $@ || (cat config.log && aborterror "Configure error, aborting build.")
		make -j3 || aborterror "Build failed."
		make plugins -j3 || aborterror "Build failed."
		make plugin.script_mapquit -j3 || aborterror "Build failed."
		make plugin.ExtendedBG -j3 || aborterror "Build failed."
		make plugin.ExtendedBG-char -j3 || aborterror "Build failed."
		;;
	test)
		cat > conf/travis_sql_connection.conf << EOF
sql_connection: {
	//default_codepage: ""
	//case_sensitive: false
	db_hostname: "localhost"
	db_username: "$DBUSER"
	db_password: "$DBPASS"
	db_database: "$DBNAME"
	//codepage:""
}
EOF
		[ $? -eq 0 ] || aborterror "Unable to write database configuration, aborting tests."
		cat > conf/import/login-server.conf << EOF
login_configuration: {
	account: {
		@include "conf/travis_sql_connection.conf"
		ipban: {
			@include "conf/travis_sql_connection.conf"
		}
	}
}
EOF
		[ $? -eq 0 ] || aborterror "Unable to override login-server configuration, aborting tests."
		cat > conf/import/char-server.conf << EOF
char_configuration: {
	@include "conf/travis_sql_connection.conf"
}
EOF
		[ $? -eq 0 ] || aborterror "Unable to override char-server configuration, aborting tests."
		cat > conf/import/map-server.conf << EOF
map_configuration: {
	@include "conf/travis_sql_connection.conf"
}
EOF
		[ $? -eq 0 ] || aborterror "Unable to override map-server configuration, aborting tests."
		cat > conf/import/inter-server.conf << EOF
inter_configuration: {
	log: {
		@include "conf/travis_sql_connection.conf"
	}
}
EOF
		[ $? -eq 0 ] || aborterror "Unable to override inter-server configuration, aborting tests."
		# Replace maps
		sed -i '/"alb_ship",/r server/conf/map/maps.conf' conf/map/maps.conf
		sed -i '/alb2trea/r server/db/map_index.txt' db/map_index.txt

		ARGS="--load-script npc/dev/test.txt "
		ARGS="--load-plugin script_mapquit $ARGS --load-script npc/dev/ci_test.txt --load-script server/npc/BGQueue.txt"
		PLUGINS="--load-plugin HPMHooking --load-plugin sample --load-plugin ExtendedBG --load-plugin ExtendedBG-char"
		# Other modes
		SCRIPTS="--load-script $script_dir/bg_conquest.txt --load-script $script_dir/bg_rush.txt"
		# Tierra
		SCRIPTS="--load-script $script_dir/bg_tierra_boss.txt --load-script $script_dir/bg_tierra_ti.txt --load-script $script_dir/bg_tierra_eos.txt --load-script $script_dir/bg_tierra_dom.txt $SCRIPTS"
		# Flavius
		SCRIPTS="--load-script $script_dir/bg_flavius_ctf.txt --load-script $script_dir/bg_flavius_td.txt --load-script $script_dir/bg_flavius_sc.txt $SCRIPTS"
		# Queue
		SCRIPTS="--load-script server/npc/BGQueue.txt $SCRIPTS"
		# Misc Scripts
		SCRIPTS="--load-script $script_dir/bg_shop.txt --load-script $script_dir/bg_ranking.txt $SCRIPTS"
		# Common functions
		SCRIPTS="--load-script $script_dir/bg_functions.txt --load-script $script_dir/bg_common.txt $SCRIPTS"
		# Run with only HPM
		echo "run all servers with HPM"
		run_server ./login-server "$PLUGINS"
		run_server ./char-server "$PLUGINS"
		run_server ./map-server "$ARGS $PLUGINS $SCRIPTS"
		;;
	getrepo)
		echo "Cloning Hercules repository..."
		# Clone Hercules Repository
		git clone https://github.com/HerculesWS/Hercules.git tmp || aborterror "Unable to fetch Hercules repository"
		echo "Moving tmp to root directory"
		yes | cp -a tmp/* .
		# Move Required files for eBG to Hercules Folder
		echo "Moving eBG files to root directory"
		yes | cp -a server/src/plugins/* src/plugins/
		mkdir db/emblems
		yes | cp -a server/db/emblems/* db/emblems/
		yes | cp -a server/maps/* maps/re/
		yes | cp -a server/maps/* maps/pre-re/
		yes | cp -a server/conf/import/* conf/import/
		rm -rf tmp
		;;
	*)
		usage
		;;
esac

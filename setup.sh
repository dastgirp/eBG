uname -a
chmod a+x travis-eBG.sh
./travis-eBG.sh getrepo || true
./travis-eBG.sh build

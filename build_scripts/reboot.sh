./mkgcc.sh 3
cd bin
tar -xzf ../backup/recent/genfiles_*.tgz
cd ..
make clean_build clean all
#./mksrc.sh

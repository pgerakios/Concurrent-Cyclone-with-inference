#./mkgcc.sh 4
cd bin
tar -czf genfiles_backup.tgz genfiles
cd ..
make cfiles && cp -r build/boot/* bin/genfiles && make clean_nogc all

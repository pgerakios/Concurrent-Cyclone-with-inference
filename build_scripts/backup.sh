S=`date +"%d%m%Y_%k-%M-%S" | tr ' ' '_'`
mkdir -p backup/$S && tar -czf src_$S.tgz src &&  mv src_$S.tgz backup/$S  && cd bin && tar -czf genfiles_$S.tgz genfiles && mv genfiles_$S.tgz ../backup/$S && cd ../backup && ./mkrecent.sh


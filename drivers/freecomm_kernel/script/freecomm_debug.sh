#!/system/bin/sh
cd /system/bin/
#. freecomm_init.sh
#export DEBUG_STUB="/mnt/sdcard/debug/debug_stub.sh"
#time_last=0
#time_now=0
echo "log_debug test start........" > /data/log_debug.txt
sdcard0=$EXTERNAL_STORAGE
sdcard1=$SECONDARY_STORAGE
echo $sdcard0 >/data/log_debug.txt
echo $sdcard1 >/data/log_debug.txt


log_started=0
while [ 1 ]
do
echo "log_debug test" >> /data/log_debug.txt
#sdcard_mounted=$(busybox mount|busybox grep "external_sd type vfat" |cut -d ' ' -f5)	#if mount got "vfat"
#in_sdcard_mounted=$(busybox mount|busybox grep "emmc type vfat" |busybox cut -d ' ' -f5)	#if mount got "vfat"
#ex_sdcard_mounted=$(busybox mount|busybox grep "sdcard type vfat" |busybox cut -d ' ' -f5)	#if mount got "vfat"
in_sdcard_mounted=$(busybox mount|busybox grep "$sdcard0 type vfat" |busybox cut -d ' ' -f5)	#if mount got "vfat"
in_sdcard_mounted1=$(busybox mount|busybox grep "$sdcard0 type fuse" |busybox cut -d ' ' -f5)	#if mount got "fuse"
ex_sdcard_mounted=$(busybox mount|busybox grep "$sdcard1 type vfat" |busybox cut -d ' ' -f5)	#if mount got "vfat"
ex_sdcard_mounted1=$(busybox mount|busybox grep "$sdcard1 type fuse" |busybox cut -d ' ' -f5)	#if mount got "fuse"
log_debug_enable=$(getprop persist.sys.log.debug.enable)

#if [ "$log_started" == "0" -a "$log_debug_enable" == "1" -a "$sdcard_mounted" == "vfat" ];then
if [ "$log_started" == "0" -a "$log_debug_enable" == "1" ] && [ "$in_sdcard_mounted1" == "fuse" -o "$in_sdcard_mounted" == "vfat" ];then
	. /system/bin/log_debug.sh 
	log_started=1
elif [ "$log_started" == "0" -a "$log_debug_enable" == "2" ] && [ "$ex_sdcard_mounted1" == "fuse" -o "$ex_sdcard_mounted" == "vfat" ];then
	. /system/bin/log_debug.sh 
	log_started=1
else 
	if [ "$log_started" == "1" -a "$log_debug_enable" == "0" ];then
	#echo "stop log debug" >> /data/debug-err-log.txt
		busybox killall log_debug_logcat
		busybox killall cat
		rm /data/log_debug_logcat
		rm /data/cat
		log_started=0
	#echo "log debug not enabled"
	#else
	#	echo "log_enable = \"$log_debug_enable\"" >> /data/debug-err-log.txt
	#	echo "log_started= \"$log_started\"" >> /data/debug-err-log.txt
	fi
fi

#if [ -e $DEBUG_STUB ]; then
#	cd /mnt/sdcard/debug
#	time_now=$(stat debug_stub.sh |grep Modify | cut -c 9-27)
#	if [ "$time_last" != "$time_now" ];
#	then
#	date >> /mnt/sdcard/debug/log.txt
#	chmod 777 debug_stub.sh
#	. debug_stub.sh
#	time_last=$time_now	
#	cat debug_stub.sh >> /mnt/sdcard/debug/log.txt
#	echo -e "\r\nExecute End\r\n" >> /mnt/sdcard/debug/log.txt
#	fi
#fi
sleep 10
done


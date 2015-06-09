#!/system/bin/sh

rm /data/log_debug_logcat
rm /data/log_debug_cat
ln -s /system/bin/logcat /data/log_debug_logcat
ln -s /system/bin/toolbox /data/cat
sdcard0=$EXTERNAL_STORAGE
sdcard1=$SECONDARY_STORAGE
log_storage=$(getprop persist.sys.log.debug.enable)
if [ "$log_storage" == "1" ];then
	#internal
	cur_storage_dir=$sdcard0

elif [ "$log_storage" == "2" ];then
	#external
	cur_storage_dir=$sdcard1
fi
cur_log_dir=$(date +%Y%m%d-%H%M%S)
#mkdir -p /sdcard/external_sd/phicomm_log/$cur_log_dir
mkdir -p $cur_storage_dir/phicomm_log/$cur_log_dir

#/data/cat /proc/kmsg > /sdcard/external_sd/phicomm_log/$cur_log_dir/kernel-$(date +%Y%m%d-%H%M%S).log &
#/data/log_debug_logcat -v time > /sdcard/external_sd/phicomm_log/$cur_log_dir/logcat-$(date +%Y%m%d-%H%M%S).log &
#/data/log_debug_logcat -b radio -v time > /sdcard/external_sd/phicomm_log/$cur_log_dir/radio-$(date +%Y%m%d-%H%M%S).log &

/data/cat /proc/kmsg > $cur_storage_dir/phicomm_log/$cur_log_dir/kernel-$(date +%Y%m%d-%H%M%S).log &
/data/log_debug_logcat -v time > $cur_storage_dir/phicomm_log/$cur_log_dir/logcat-$(date +%Y%m%d-%H%M%S).log &
/data/log_debug_logcat -b radio -v time > $cur_storage_dir/phicomm_log/$cur_log_dir/radio-$(date +%Y%m%d-%H%M%S).log &

if [ -e /data/anr/traces.txt ];then
	#cp /data/anr/traces.txt /sdcard/external_sd/phicomm_log/$cur_log_dir/
	busybox cp /data/anr/traces.txt $cur_storage_dir/phicomm_log/$cur_log_dir/
fi

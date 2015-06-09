#!/system/bin/sh
echo "test start!!!"
freq_set="false"
while [ 1 ]
do
new_state=$(ps |busybox grep templerun)
echo "$new_state"
   if [ "$new_state" != "" -a "$freq_set" == "false" ];then
      echo "templerun is running!!!"
      echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
      echo 700800 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
      freq_set="true"
   elif [ "$new_state" == "" -a "$freq_set" == "true" ];then
      echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
		  echo "templerun is not running"
		  freq_set="false"
	 else
	    echo "templerun state not change!!!"
	 fi
sleep 5
done

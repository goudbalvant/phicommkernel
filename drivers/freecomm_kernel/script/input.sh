#!/system/bin/sh
cd /sys/class/input
for dir in $(ls)
do
	name=$(cat $dir/name)
	if [ "$name" == "proximity" ]; then
		chown system $dir/*
		echo ok
	else
		echo error
	fi
        if [ "$name" == "light" ]; then
                chown system $dir/*
                echo ok
       else
                echo error
       fi
done 
    

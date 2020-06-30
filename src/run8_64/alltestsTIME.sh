#!/bin/sh
date > alltestsTIME.txt
#--------------------------------------------------------------
#This script uses the following programs to obtain information! 
#"cat, echo, sed, uname, free, whoami, logname, uptime, and df"
#--------------------------------------------------------------
#---------------
#Current Version
#---------------
aver=v1.03
#---------------------------------------------------------------------------------
#Delcare some variables.
#---------------------------------------------------------------------------------
name=$(uname -s)
version=$(uname -v)
hardware=$(uname -m)
release=$(uname -r)
disk=$(df -lh)
hname=$(uname -n)
cpu=$(cat /proc/cpuinfo | grep 'cpu MHz' | sed 's/cpu MHz//')
cpumodel=$(cat /proc/cpuinfo | grep 'model name' | sed 's/model name//')
who=$(whoami)
lwho=$(logname)
memory=$(cat /proc/meminfo | grep MemTotal)
#Display information to user...
#------------------------------
echo "information about your box. "  >> alltestsTIME.txt
echo "OS Type                 : $name" >> alltestsTIME.txt
echo "Hostname                : $hname" >> alltestsTIME.txt
echo "Currently logged in as  : $who" >> alltestsTIME.txt
echo "Originally logged in as : $lwho" >> alltestsTIME.txt
echo "Hardware Architecture   : $hardware" >> alltestsTIME.txt
echo "CPU Model         $cpumodel" >> alltestsTIME.txt
echo "CPU Speed   $cpu MHz" >> alltestsTIME.txt
echo "Kernel Version          : $version"  >> alltestsTIME.txt
echo "System Memory in MB"    : $memory >> alltestsTIME.txt

./version.exe >> alltestsTIME.txt
echo -------- cpTIME >> alltestsTIME.txt
date > cptime.txt
./cpTIME.bat
echo -------- cciTIME >> alltestsTIME.txt
date > ccitime.txt
./cciTIME.bat
echo -------- RecordAccessTIME.bat >> alltestsTIME.txt
date > RandomAccessTime.txt
./RecordAccessTIME.bat
echo -------- SynchStatsTIME >> alltestsTIME.txt
date > SyncStatsTime.txt
./SynchStatsTIME.bat
echo -------- ThreeStageTIME >> alltestsTIME.txt
date > ThreeStageTime.txt
./ThreeStageTIME.bat
echo -------- Combine all the results into a single file >> alltestsTIME.txt
cat alltestsTIME.txt cptime.txt ccitime.txt RandomAccessTime.txt SyncStatsTime.txt ThreeStageTime.txt > alltestsTIME.$hname.txt
echo -------- zip the results file >> alltestsTIME.$hname.txt
gzip alltestsTIME.$hname.txt
echo ========================================

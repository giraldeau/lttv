#!/bin/bash
#  Copyright (C) 2006 Eric Clément
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
# purpose : automatic generation of traces with LTT in a network.
# usage : 
# 
# you need SSH connection without password for all computers that you want
# trace.  You might put le name of those computers (uname -n) in a file
# (liste.txt by default).  LTT might be installed in the same way for those
# computer.  
# You can customize your path.  This script can also generate traffic TCP and
# UDP.  A UDP monitor is use to validate the result (this is the first computer
# in the file (liste.txt)).
# 
# usage: ./lttclt_distributed.sh time freq mode_generator {options}
# 
# time (seconds) : duration a the tracing
# freq (nb packet/second or nb packet/ms)  : communication frequency when TCP
#                                            generator is used, 0 otherwise
# mode_generator : 1 : traffic generator off
#                  2 : traffic generator TCP on (nb packet/second)
#                  3 : traffic generator TCP on (nb packet/ms)
# 
# options (optional): 1 : enable the UDP monitor (1packet/second is generated
#                         by all UDP client)
# 
# you need : ssh, scp and zip

TCP_SERVER=tcpserver
TCP_CLIENT=tcpclient

UDP_SERVER=udpserver
UDP_CLIENT=udpclient  

PATH_TRACE=/root/trace-ltt/
PATH_DEBUGFS=/debugfs/ltt/
SET_LTT_FACILITIES="export LTT_FACILITIES=/home/ercle/NEW_GENERATION_LTTV/share/LinuxTraceToolkitViewer/facilities/"
SET_LTT_DAEMON="export LTT_DAEMON=/home/ercle/NEW_GENERATION_LTTV/bin/lttd"

START_DAEMON="/home/ercle/NEW_GENERATION_LTTV/bin/lttctl -d -n \
trace1 -t $PATH_TRACE  -l $PATH_DEBUGFS >/dev/null"

STOP_DAEMON="/home/ercle/NEW_GENERATION_LTTV/bin/lttctl  -n trace1 -q >/dev/null"
REMOVE_DAEMON="/home/ercle/NEW_GENERATION_LTTV/bin/lttctl  -n trace1 -r >/dev/null"
REMOVE_TRACE="rm -rf $PATH_TRACE"

E_FNEXIST=100

FILE_LISTE=liste.txt
TRACE_DAEMON=/tmp/daemon-

if [ $# -lt 3 ]
then
  echo "usage: $0 time freq mode_generator {options}"
  exit 1
elif [ $# -gt 4 ]
then
  echo "usage: $0 time freq mode_generator {options}"
  exit 1
fi

if [ -e $FILE_LISTE ]
then
  
  time=$1
  freq=$2
  mode_generator=$3
  if [ $# -eq 4 ]
  then
    mode_monitor=$4
  else
    mode_monitor=0
  fi

#create script generator
  FILE_OUT=daemon-

  if [ $mode_generator -eq 3 ]
  then
    TCP_CLIENT=tcpclient_ms
  fi

  nb_node=0
  for line in $( cat  $FILE_LISTE );
  do
    let nb_node+=1
  
    echo $REMOVE_TRACE         > "$FILE_OUT$line.sh"
    echo mkdir $PATH_TRACE    >> "$FILE_OUT$line.sh"
    echo $SET_LTT_FACILITIES  >> "$FILE_OUT$line.sh"
    echo $SET_LTT_DAEMON      >> "$FILE_OUT$line.sh"
    echo $START_DAEMON        >> "$FILE_OUT$line.sh"

    chmod +x "$FILE_OUT$line.sh"
  
    if [ $mode_generator -ge 2 ]  #if generator de trafic enable (2 or 3)
    then
      if [ $nb_node -eq 1 -a $mode_monitor -eq 1 ]
      then
        monitor=$line
        echo "/tmp/$UDP_SERVER >/dev/null &" >> "$FILE_OUT$line.sh"
	scp "$UDP_SERVER" $line:/tmp/

        echo "sleep $time" >> "$FILE_OUT$line.sh"
        echo "kill \`ps -A |grep $UDP_SERVER | awk '{ print \$1 }'\`" >> "$FILE_OUT$line.sh"
      else
        if [ $mode_monitor -eq 1 ]
        then
          echo "/tmp/$UDP_CLIENT $monitor $time 1 >/dev/null & " >> "$FILE_OUT$line.sh"
	  scp "$UDP_CLIENT" $line:/tmp/
        fi
      
        echo "/tmp/$TCP_SERVER >/dev/null &" >> "$FILE_OUT$line.sh"
	scp "$TCP_SERVER" $line:/tmp/
        compteur=0 
        for line2 in $( cat  $FILE_LISTE );
        do
          let compteur+=1
          if [ $compteur -gt $nb_node ]
          then
            echo "/tmp/$TCP_CLIENT $line2 $time $freq >/dev/null &" >> "$FILE_OUT$line.sh"
	    scp "$TCP_CLIENT" $line:/tmp/
          fi
        done  
        
	echo "sleep $time" >> "$FILE_OUT$line.sh"
        echo "kill \`ps -A |grep $TCP_SERVER | awk '{ print \$1 }'\`" >> "$FILE_OUT$line.sh" 
      fi
    fi
  
    echo $STOP_DAEMON	>> "$FILE_OUT$line.sh"
    echo $REMOVE_DAEMON	>> "$FILE_OUT$line.sh"
  
    #script for get node information
    ENDIAN=endian
    echo 'FILE_OUT=`uname -n`.info'                               >> "$FILE_OUT$line.sh"
    echo 'ENDIAN=endian'                                          >> "$FILE_OUT$line.sh"
    echo '  exec 6>&1'                                            >> "$FILE_OUT$line.sh"
    echo '  exec > /tmp/$FILE_OUT'                                >> "$FILE_OUT$line.sh"
    echo '  echo `uname -n`'                                      >> "$FILE_OUT$line.sh"
    echo ''                                                       >> "$FILE_OUT$line.sh"
    echo '  /tmp/$ENDIAN || ( echo >&6 && echo "** ERROR **: problem occur with /tmp/$ENDIAN" >&6 && echo >&6)'                               >> "$FILE_OUT$line.sh"
    echo ''                                                       >> "$FILE_OUT$line.sh"
    echo -e '  /sbin/ifconfig | grep addr: | awk \047{print $2}\047 | sed /127.0.0.1/d | sed s/addr:// | sed /^$/d	#english'                               >> "$FILE_OUT$line.sh"
    echo -e '  /sbin/ifconfig | grep adr: | awk \047{print $2}\047 | sed /127.0.0.1/d | sed s/adr:// | sed /^$/d 	#french'                               >> "$FILE_OUT$line.sh"
    echo '  echo END'                                             >> "$FILE_OUT$line.sh"
    echo ''                                                       >> "$FILE_OUT$line.sh"
    echo '  exec 1>&6 6>&-'                                       >> "$FILE_OUT$line.sh"
    echo ''                                                       >> "$FILE_OUT$line.sh"
    echo '  echo "$FILE_OUT done"'                                >> "$FILE_OUT$line.sh"
    echo ''                                                       >> "$FILE_OUT$line.sh"
    
  #send files
      if [ $mode_generator -ge 2 -a $nb_node -eq 1 -a $mode_monitor -eq 1 ]
      then
        echo mv /tmp/'`uname -n`'.info /tmp/'`uname -n`'.monitor >> "$FILE_OUT$line.sh"
	echo scp /tmp/'`uname -n`'.monitor `uname -n`:`pwd`/   >> "$FILE_OUT$line.sh"
      else
        echo scp /tmp/'`uname -n`'.info `uname -n`:`pwd`/   >> "$FILE_OUT$line.sh"
      fi
  
    echo '  exit 0'                                               >> "$FILE_OUT$line.sh"
  
    scp "$FILE_OUT$line.sh" $line:/tmp/
    scp "$ENDIAN" $line:/tmp/
    rm "$FILE_OUT$line.sh" 
  done
#end script generator   
  
#start traces !!    
  sleep 1

  for line in $( cat  $FILE_LISTE );
    do
      echo ssh -f "$line $TRACE_DAEMON$line.sh "
      ssh -f $line "$TRACE_DAEMON$line.sh" 
    done
 
else
  echo "error: file $FILE_LISTE doesn't exist"
  exit E_FNEXIST
fi

date

sleep $time

# is all daemon stop ?
for line in $( cat  $FILE_LISTE );
  do

      daemon_present="true"
      wait_time=1
      while [ $daemon_present == "true" ]
      do
         daemon_present="false"
         (ssh $line ps -A |grep lttd) && daemon_present="true"
         sleep $wait_time
         let wait_time+=1
      done
  done

#get all traces
nb_computer=0
zip_path=""
for line in $( cat  $FILE_LISTE );
  do
   mkdir `pwd`/$line 2>/dev/null
   scp -q -r $line:$PATH_TRACE/ `pwd`/$line
   zip_path="$zip_path $line"
   let nb_computer+=1
 done

#get network informatioin
FILE_TMP=ls.tmp
FILE_OUT=network.trace

exec 3> $FILE_TMP		#open FILE_TMP (Write)

ls *.monitor >&3 2>/dev/null

#get the list of .info file to parse
ls *.info >&3 || (echo EMPTY >&3)
echo END >&3

exec 3>&-       		#close FILE_TMP

exec 3< $FILE_TMP		#open FILE_TMP (Read)
read line <&3
if [ "$line" = "EMPTY" ]
then
  echo "NO .info file"
  exec 3>&-       		#close FILE_TMP
  rm -rf $FILE_TMP
  exit 1
fi

exec 5> $FILE_OUT		#open FILE_OUT (Write)

echo -e "Nb IP\tName\t\endianness\tIP ..."
echo -e "Nb IP\tName\tendianness\tIP ..." >&5

while [ "$line" != "END" ]
do
  echo "++++++++++++++++++++"
  echo "in file $line"

  output=""
  nb_ip=-2			#1st line => name, 2 line => endianness
  exec 4< $line
  read answer <&4
  
    while [ "$answer" != "END" ]
    do
        nb_ip=`expr $nb_ip + 1`
        output="$output\n$answer"
	read answer <&4
    done
    
  echo -e "$nb_ip$output"
  echo -e "$nb_ip$output" >&5
  exec 4<&-
  mv $line "$line.read"
  read line <&3
  echo "---------------------"
done

exec 3>&-       		#close FILE_TMP
rm -rf $FILE_TMP
echo END >&5
exec 5<&-			#close FILE_OUT

#zip files
  root=`pwd` 
  cd $root && echo $root
  nomfic="trace__nb_computer"$nb_computer"__time"$time"__freq"$freq"__"`uname -n``date '+__%d%b__%H-%M-%S'`$options".zip"

   zip -r $nomfic $zip_path *.info.read *.monitor.read *.trace>/dev/null
  echo -e "zip done $zip_path\n$root/$nomfic"
  
  exec 3<&-

echo -e "\a$0 done!"
exit 0

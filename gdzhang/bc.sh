#!/bin/bash - 
#===============================================================================
#
#          FILE: bc.sh
# 
#         USAGE: ./bc.sh 
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Zhang, Guodong (), gdzhang@linx-info.com
#  ORGANIZATION: 
#       CREATED: 2019年10月25日 09:49
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

input=$1
high=60
low=30
start_speed=15
percent=100
pwm=0;
pwm=`echo "($input-$start_speed-1) * ($percent - $start_speed)/($high - $low-1) - $start_speed-1"| bc -l`
echo "1 pwm :$pwm"

if [ $input -lt $low ] ;then  pwm=$start_speed; fi
if [ $input -gt $high ] ;then  pwm=$percent; fi
#if [ $input -gt 30 ]; then pwm=$start_speed ;fi
#if [ $input -gt 65 ]; then
#   pwm=100;
#if

echo "pwm is $pwm "

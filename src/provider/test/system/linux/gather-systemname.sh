#!/bin/sh


#***********************************************************************#
#                          MetricGatherer                               #
#-----------------------------------------------------------------------#

# SystemName

echo `hostname` | grep `dnsdomainname` >/dev/null\
&& echo `hostname` \
|| echo `hostname`'.'`dnsdomainname`


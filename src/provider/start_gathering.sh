#!/bin/sh
GATHER=`wbemcli ein http://localhost/root/cimv2:Linux_MetricGatherer`
REPOS=`wbemcli ein http://localhost/root/cimv2:Linux_MetricRepositoryService`

if [ -z "$GATHER" -o -z "$REPOS" ]
then
    echo "Could not get infos about gathering services."
    echo "\tProviders installed? Wbemcli available?"
else
    echo "go on"
    wbemcli cm http://"$GATHER" StartService
    wbemcli cm http://"$REPOS" StartService
    wbemcli cm http://"$GATHER" StartSampling
fi
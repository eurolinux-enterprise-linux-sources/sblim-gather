#!/usr/bin/perl

# This sample perl script demonstrates how to convert a reposdump output
# file into a comma-separated value list for import into a spreadsheet
# program.
# It receives a list of dump file names and will produce two files:
# defs.csv, containing the metric definitions and vals.csv containing the
# metric values.

open $defs, ">", "defs.csv";
open $vals, ">", "vals.csv";

my @listtypes = ( "metricdefs", "metricvals" );
my %outfiles = ( "$listtypes[0]" => $defs, "$listtypes[1]" => $vals );
my %headerseen = ( "$listtypes[0]" => 0, "$listtypes[1]" => 0 );


while (<>) {   
    if(/^===(.*) (.*)===/) {
	# Skip delimiters
	$recordtype = $1;
	$pluginname = $2;
	if (!$headerseen{$recordtype}) {
	    # Only write first header
	    $headerseen{$recordtype} = 1;
	    $headermode = 1;
	    $write = 1;
	} else {
	    $write = 0;
	}
    } elsif ($write) {
	# Write header or data
	if ($headermode) {
	    $plugininfo = "Plugin";
	    $headermode=0;
	} else {
	    $plugininfo = $pluginname;
	}
	print { $outfiles{$recordtype} } "$plugininfo,", $_;
    } else {
	# Reenable writing after skipped header
	$write = 1;
    }
}
close $defs;
close $vals;

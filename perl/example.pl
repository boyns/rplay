#!/usr/bin/perl

use RPlay;

$rp = new RPlay;
$rp->connect ("localhost");
$rp->play ("sound" => "bogus.au");
$rp->disconnect ();

exit 0;

# $Id: RPlay.pm,v 1.2 1998/08/13 06:18:06 boyns Exp $	-*-perl-*-
#
# Copyright (C) 1993-98 Mark R. Boyns <boyns@doit.org>                               
#
# This file is part of rplay.                                                        
#
# rplay is free software; you can redistribute it and/or modify                      
# it under the terms of the GNU General Public License as published by               
# the Free Software Foundation; either version 2 of the License, or                  
# (at your option) any later version.                                                
#
# rplay is distributed in the hope that it will be useful,                           
# but WITHOUT ANY WARRANTY; without even the implied warranty of                     
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      
# GNU General Public License for more details.                                       
#
# You should have received a copy of the GNU General Public License                  
# along with rplay; see the file COPYING.  If not, write to the                      
# Free Software Foundation, Inc.,                                                    
# 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.                            
#

package RPlay;

require 5.000;
require "rplay.ph";

use Socket;

$connected = 0;

## Create a new RPlay object.
sub new
{
    my $self = {};
    bless $self;
    return $self;
}

## Connect to a rplay server.
sub connect
{
    my $self = shift;
    my ($that_host, $that_port) = @_;
    my ($pat, $name, $aliases, $proto, $port, $udp);
    my (@bytes, $addrtype, $length, $old);
    my ($that, $that_addr, $that_addr);
    my ($this, $this_addr, $this_addr);

    $sockaddr = 'S n a4 x8';

    ($name, $aliases, $proto) = getprotobyname ('udp');
    $udp = $proto;
    ($name, $aliases, $port, $proto) = getservbyname ('rplay', 'udp');

    if (!$that_port)
    {
	$that_port = $name ? $port : &RPLAY_PORT;
    }

    chop ($this_host = `hostname`);
    ($name, $aliases, $addrtype, $length, $this_addr) = gethostbyname ($this_host);
    die "$this_host: unknown host\n" unless $name;
    ($name, $aliases, $addrtype, $length, $that_addr) = gethostbyname ($that_host);
    die "$that_host: unknown host\n" unless $name;

    $this = pack ($sockaddr, AF_INET, 0, $this_addr);
    $that = pack ($sockaddr, AF_INET, $that_port, $that_addr);

    socket (RPLAY, AF_INET, SOCK_DGRAM, $udp) || die "socket: $!";
    ## bind (RPLAY, $this) || die "bind: $!";
    connect (RPLAY, $that) || die "connect: $!";

    $old = select (RPLAY);
    $| = 1;
    select ($old);

    $connected = 1;
}

## Close the rplay server connection.
sub disconnect
{
    my $self = shift;
    
    close (RPLAY);
    $connected = 0;
}

## Convert different types arguments to a list of hash references.
sub parse
{
    my $self = shift;
    my ($r, @refs);

    $r = ref ($_[0]);
    if ($r eq "HASH")
    {
    	@refs = @_;
    }
    elsif ($r eq "ARRAY")
    {
    	foreach (@_)
    	{
    	    my (%attr, $i, @list);
    	    @list = @$_;
	    for ($i = 0; $i < $#list; $i+=2)
	    {
	        $attr{$list[$i]} = $list[$i+1];
	    }
	    push (@refs, \%attr);
    	}
    }
    else
    {
    	my (%attr, $i);
    	for ($i = 0; $i < $#_; $i+=2)
    	{
    	    $attr{$_[$i]} = $_[$i+1];
    	}
	push (@refs, \%attr);
    }

    return @refs;
}

## Create the appropriate RPLAY packet and send it to the server.
sub doit
{
    my $self = shift;
    my $command = shift;
    my @refs = $self->parse (@_);
    my (%attrs, $name, $packet);

    die "Not connected - use connect () first." unless $connected;

    ## Packet header.
    $packet = pack ("C", &RPLAY_PACKET_ID);
    $packet .= pack ("C", $command);

    ## Convert name-value hash pairs to RPLAY attributes.
    foreach (@refs)
    {
    	%attrs = %$_;

	if (!$attrs{'sound'})
	{
	    die "Missing `sound' attribute.";
	}
	foreach (keys %attrs)
	{
	    if (/sound/)
	    {
		$packet .= pack ("C", &RPLAY_SOUND);
		$packet .= "$attrs{$_}\0";
	    }
	    elsif (/volume/)
	    {
		$packet .= pack ("C", &RPLAY_VOLUME);
		$packet .= pack ("C", $attrs{$_});
	    }
	    elsif (/list_count/)
	    {
		$packet .= pack ("C", &RPLAY_LIST_COUNT);
		$packet .= pack ("C", $attrs{$_});
	    }
	    elsif (/priority/)
	    {
		$packet .= pack ("C", &RPLAY_PRIORITY);
		$packet .= pack ("C", $attrs{$_});
	    }
	    elsif (/sample_rate/)
	    {
		$packet .= pack ("C", &RPLAY_SAMPLE_RATE);
		$packet .= pack ("C", $attrs{$_});
	    }
	    elsif (/list_name/)
	    {
		$packet .= pack ("C", &RPLAY_CLIENT_DATA);
		$packet .= "$attrs{$_}\0";
	    }
	    else
	    {
		#warn "Uknown attribute `$_'";
	    }
	}
        $packet .= pack ("C", &RPLAY_NULL);
    }
    $packet .= pack ("C", &RPLAY_NULL);

    send (RPLAY, $packet, 0) || die "send: $!";
}

## Play sounds.
sub play
{
    my $self = shift;
    $self->doit (&RPLAY_PLAY, @_);
}

## Pause sounds.
sub pause
{
    my $self = shift;
    $self->doit (&RPLAY_PAUSE, @_);
}

## Continue sounds.
sub continue
{
    my $self = shift;
    $self->doit (&RPLAY_CONTINUE, @_);
}

## Stop sounds.
sub stop
{
    my $self = shift;
    $self->doit (&RPLAY_STOP, @_);
}

## Done sounds.
sub done
{
    my $self = shift;
    $self->doit (&RPLAY_DONE, @_);
}

1;

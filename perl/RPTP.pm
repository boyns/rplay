# $Id: RPTP.pm,v 1.1 1998/10/12 16:07:05 boyns Exp $	-*-perl-*-
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

package RPTP;

require 5.000;
require "rplay.ph";
require "shellwords.pl";
use FileHandle;
use Socket;
use strict;

## Create a new RPlay object.
sub new
{
    my $self = {};
    bless $self;
    $self->{debug} = 0;
    $self->{server} = ();
    $self->{callbacks} = ();
    $self->{socket} = undef;
    $self;
}

sub debug
{
    my $self = shift;
    $self->{debug} = @_[0];
}

## Connect to a rplay server.
sub connect
{
    my $self = shift;
    my ($that_host, $that_port) = @_;
    my ($pat, $name, $aliases, $proto, $port, $udp);
    my (@bytes, $addrtype, $length, $old);
    my ($that, $that_addr);
    my ($this, $this_addr, $this_host);

    my $sockaddr = 'S n a4 x8';

    ($name, $aliases, $proto) = getprotobyname ('tcp');
    my $tcp = $proto;
    #($name, $aliases, $port, $proto) = getservbyname ('rplay', 'udp');

    $that_port = &RPTP_PORT;

    chop ($this_host = `hostname`);
    ($name, $aliases, $addrtype, $length, $this_addr) =
         gethostbyname ($this_host);
    die "$this_host: unknown host\n" unless $name;
    ($name, $aliases, $addrtype, $length, $that_addr) =
         gethostbyname ($that_host);
    die "$that_host: unknown host\n" unless $name;

    $this = pack ($sockaddr, AF_INET, 0, $this_addr);
    $that = pack ($sockaddr, AF_INET, $that_port, $that_addr);

    $self->{socket} = new FileHandle;
    socket ($self->{socket}, AF_INET, SOCK_STREAM, $udp) || die "socket: $!";
    connect ($self->{socket}, $that) || die "connect: $!";

    $old = select ($self->{socket});
    $| = 1;
    select ($old);

    my %hash = $self->readline();
    foreach (keys %hash)
    {
	$self->{"server_$_"} = $hash{$_};
    }
}

## Close the rplay server connection.
sub disconnect
{
    my $self = shift;
    close ($self->{socket});
}

sub server_info
{
    my $self = shift;
    $self->{"server_".@_[0]};
}

##
sub readline
{
    my $self = shift;
    my %hash;
    my $sock = $self->{socket};
    chomp(my $line = <$sock>);
    print "readline: $line\n" if $self->{debug};
    my $type = substr($line, 0, 1);
    $hash{'_type'} = $type;
    $line = substr($line, 1);
    foreach (shellwords($line))
    {
	my ($name, $value) = split('=');
	$hash{$name} = $value;
    }
    %hash;
}

##
sub writeline
{
    my $self = shift;
    my $sock = $self->{socket};
    print "writeline: ", join(" ", @_), "\n" if $self->{debug};
    print $sock join(" ", @_), "\n";
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
    my (%attrs, $line);

    #die "Not connected - use connect() first." unless $connected;

    $line = "$command";

    foreach (@refs)
    {
    	%attrs = %$_;
	foreach (keys %attrs)
	{
	    $line .= " $_=\"$attrs{$_}\"";
	}
    }

    die "Missing id or sound." unless $attrs{sound} or $attrs{id};

    $self->writeline($line);
}

## Play sounds.
sub play
{
    my $self = shift;
    $self->doit ("play", @_);
}

## Pause sounds.
sub pause
{
    my $self = shift;
    $self->doit ("pause", @_);
}

## Continue sounds.
sub continue
{
    my $self = shift;
    $self->doit ("continue", @_);
}

## Stop sounds.
sub stop
{
    my $self = shift;
    $self->doit ("stop", @_);
}

## Done sounds.
sub done
{
    my $self = shift;
    $self->doit ("done", @_);
}

#  "continue"
#  "done"
#  "error"
#  "flow"
#  "level"
#  "modify"
#  "ok"
#  "pause"
#  "play"
#  "position"
#  "skip"
#  "stop"
#  "timeout"
#  "volume"
sub notify
{
    my $self = shift;
    my($type, $func) = @_;
    $self->{callbacks}{$type} = $func;
}

my %proto =
(
 "+" => "ok",
 "-" => "error",
 "!" => "timeout",
 "@" => "event"
);
 
sub mainloop
{
    my $self = shift;
    my $emask;
    foreach (keys %{$self->{callbacks}})
    {
	next if /(ok|error|timeout|event)/;
	$emask .= "$_|";
    }
    chop $emask;

    $self->writeline("set notify=$emask");

    for (;;)
    {
	my %hash = $self->readline();
	my $type = $proto{$hash{_type}};
	if ($hash{command} eq "set")
	{
	    next;
	}
	my $func;
	if (exists($self->{callbacks}{all}))
	{
	    $func = $self->{callbacks}{all};
	}
	elsif (exists($self->{callbacks}{$type}))
	{
	    $func = $self->{callbacks}{$type};
	}
	else
	{
	    $func = $self->{callbacks}{$hash{$type}};
	}
	&$func(%hash) if $func;
    }
}

1;

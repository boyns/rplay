/* RPlay.java */

/*
 * Copyright (C) 1998 Mark R. Boyns <boyns@doit.org>
 *
 * This file is part of rplay.
 *
 * rplay is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * rplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rplay; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

package org.doit.rplay;

import org.doit.io.ByteArray;
import java.util.*;
import java.net.*;

public class RPlay
{
    final byte RPLAY_PACKET_ID = 30;
    final byte RPLAY_NULL = 0;
    final byte RPLAY_PLAY = 1;
    final byte RPLAY_STOP = 2;
    final byte RPLAY_PAUSE = 3;
    final byte RPLAY_CONTINUE = 4;
    final byte RPLAY_SOUND = 5;
    final byte RPLAY_VOLUME = 6;
    final byte RPLAY_PRIORITY = 15;
    final byte RPLAY_RANDOM_SOUND = 16;
    final byte RPLAY_SAMPLE_RATE = 22;
    final byte RPLAY_PUT = 27;
    final byte RPLAY_ID = 28;
    final byte RPLAY_SEQUENCE = 29;
    final byte RPLAY_DATA = 30;
    final byte RPLAY_DATA_SIZE = 31;

    final int RPLAY_PORT = 5555;

    private DatagramSocket socket;
    private InetAddress serverAddr;
    private Hashtable attrs = new Hashtable ();
    private byte command;

    public void open (String hostname) throws Exception
    {
	serverAddr = InetAddress.getByName (hostname);
	socket = new DatagramSocket ();
    }

    public void close ()
    {
	socket.close ();
    }

    public void play (String sound)
    {
	command = RPLAY_PLAY;
	attrs.put ("sound", sound);
	doit ();
    }

    public void stop (String sound)
    {
	command = RPLAY_STOP;
	attrs.put ("sound", sound);
	doit ();
    }

    public void pause (String sound)
    {
	command = RPLAY_PAUSE;
	attrs.put ("sound", sound);
	doit ();
    }

    public void resume (String sound)
    {
	command = RPLAY_CONTINUE;
	attrs.put ("sound", sound);
	doit ();
    }

    public void put (int id, int sequence, byte data[])
    {
	command = RPLAY_PUT;
	attrs.put ("id", new Byte (id));
	attrs.put ("sequence", new Integer (sequence));
	attrs.put ("data", new ByteArray (data));
	doit ();
    }

    void doit ()
    {
	ByteArray pack = new ByteArray ();
	pack.append (RPLAY_PACKET_ID);
	pack.append (command);

	Enumeration e = attrs.keys ();
	while (e.hasMoreElements ())
	{
	    String key = (String) e.nextElement ();
	    if (key.equals ("sound"))
	    {
		pack.append (RPLAY_SOUND);
		pack.append ((String) attrs.get (key));
		pack.append ((byte)0);
	    }
	    else if (key.equals ("id"))
	    {
		pack.append (RPLAY_ID);
		pack.append (((Byte) attrs.get (key)).byteValue ());
	    }
	    else if (key.equals ("sequence"))
	    {
		pack.append (RPLAY_ID);
		// need network byte order
		-> pack.append (((Integer) attrs.get (key)).Value ());
	    }
	}
	pack.append (RPLAY_NULL);
	
	DatagramPacket packet = new DatagramPacket (pack.getBytes (),
						    pack.length (),
						    serverAddr,
						    RPLAY_PORT);
	try
	{
	    socket.send (packet);
	}
	catch (Exception ex)
	{
	    System.out.println (ex);
	}
    }
    
    public static void main (String argv[]) throws Exception
    {
	RPlay rp = new RPlay ();
	rp.open ("doit.sdsu.edu");
	rp.play (argv[0]);
	rp.close ();
    }
}

/*
   getcalleraddr.c
   Prints the address of the host that is using this program through a socket.
   Useful in a remote shell or in a ".fingerrc" file...
   [Raphael Quinet, 09 Feb 94]
*/

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

char *getcallername(s)
     int s;
{
  struct sockaddr_in  addr;
  int                 addrlen;
  struct hostent     *hp;

  addrlen = sizeof(struct sockaddr);
  if (getpeername(s, (struct sockaddr *)&addr, &addrlen) > -1)
    {
      hp = gethostbyaddr(addr.sin_addr, addrlen, AF_INET);
      if (hp != NULL)
	return hp->h_name;
      else
	return inet_ntoa(addr.sin_addr);
    }
  return NULL;
}

void main(argc, argv)
     int   argc;
     char *argv[];
{
  char *prog_name;
  int   socket_num;
  char *caller_name;

  prog_name = *argv++;
  socket_num = fileno(stdout);

  for (argc--; argc; argv++, argc--)
    if (!strcmp(*argv, "-stdin"))
      socket_num = fileno(stdin);
    else
      if (!strcmp(*argv, "-stdout"))
	socket_num = fileno(stdout);
      else
	if (!strcmp(*argv, "-stderr"))
	  socket_num = fileno(stderr);
	else
	  {
	    if (strcmp(*argv, "-help") && strcmp(*argv, "-h"))
	      printf("Error: invalid option %s\n", *argv);
	    printf("Usage: rsh <hostname> %s [-stdin | -stdout | -stderr]\n",
		   prog_name);
	    exit(-1);
	  }

  caller_name = getcallername(socket_num);
  if (caller_name)
    {
      printf("%s\n", caller_name);
      exit(0);
    }
  else
    {
      printf("Error: no socket\n");
      exit(1);
    }
}

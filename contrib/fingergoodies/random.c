/*
   random.c
   This program randomly picks one of its command line arguments and prints it.
   [Raphael Quinet, 07 Feb 92, 13 May 93]
*/
main(v,c)char**c;{srandom((int)time(!++c)*getpid());v-->1?printf("%s\n",c[random()%v]):(int)v;}

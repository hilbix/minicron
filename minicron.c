/* $Header$
 *
 * This Works is placed under the terms of the Copyright Less License,
 * see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.
 *
 * $Log$
 * Revision 1.1  2008-01-18 03:23:37  tino
 * First dist
 *
 */

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "minicron_version.h"

char	iobuf[1024];
int	iofill;
int	iofd;

#define	USAGE	"Usage: minicron [options] seconds program [args..] 3>log\r\n"	\
"		Version " MINICRON_VERSION " compiled " __DATE__ "\r\n"		\
"	Start a program in a loop, wait seconds between loops\r\n"		\
"	This program currently has no options.\r\n"

void
write_all(int fd, const void *_s, size_t len)
{ 
  const unsigned char *s=_s;
  int got;

  while (len && (got=write(fd, s, len))!=0)
    if (got>0)
      s+=got, len-=got;
    else if (errno!=EINTR)
      break;
}

void
ioflush(void)
{
  write_all(iofd, iobuf, iofill);
  iofill	= 0;
}

void
ioset(int fd)
{
  if (iofd!=fd)
    ioflush();
  iofd	= fd;
}

void
ioc(char c)
{
  if (iofill>=sizeof iobuf)
    ioflush();
  iobuf[iofill++]	= c;
}

void
ios(const char *s)
{
  while (*s)
    ioc(*s++);
}

void
ionl(void)
{
  ioc('\r');
  ioc('\n');
  ioflush();
}

void
iox(int x)
{
  ioc("0123456789abcdef"[x]);
}

void
ioul(unsigned long u)
{
  if (u>9ull)
    ioul(u/10);
  iox(u%10);
}

void
iouln(unsigned long u, int n)
{
  if (n>1)
    {
      iouln(u/10, n-1);
      u	%= 10;
    }
  ioul(u);
}

void
ioul2(unsigned long u)
{
  iouln(u, 2);
}

void
ioul4(unsigned long u)
{
  iouln(u, 4);
}

void
ioxl(unsigned long u)
{
  if (u>15ull)
    ioul(u>>4);
  iox(u&15);
}

void
err(const char *s)
{
  ioset(2);
  ios(s);
}

void
ex(const char *s)
{
  ioset(2);
  ios(s);
  ionl();
  exit(1);
}

void
oops(const char *s)
{
  int		e=errno;

  ioset(2);
  ios(s);
  ios(": ");
  ios(strerror(e));
  ionl();
}

void
blog(void)
{
  time_t	tim;
  struct tm	*tm;

  ioset(3);
  time(&tim);
  tm	= localtime(&tim);
  ioul4(tm->tm_year+1900);
  ioc('-');
  ioul2(tm->tm_mon+1);
  ioc('-');
  ioul2(tm->tm_mday);
  ioc(' ');
  ioul2(tm->tm_hour);
  ioc(':');
  ioul2(tm->tm_min);
  ioc(':');
  ioul2(tm->tm_sec);
  ioc(' ');
}

unsigned long
getul(const char *s)
{
  char *end;
  unsigned long	ul;

  ul	= strtoul(s, &end, 0);
  if (!end || *end)
    ex("wrong number");
  return ul;
}

void
forker(char **args)
{
  pid_t	pid, got;

  if ((pid=fork())==0)
    {
      execvp(args[0], args);
      oops("exec failed");
      exit(1);
    }
  if (pid==(pid_t)-1)
    {
      oops("fork");
      return;
    }

  blog();
  ios("forked child ");
  ioul((unsigned long)pid);
  ionl();

  do
    {
      int	st;

      got	= waitpid((pid_t)-1, &st, WUNTRACED);
      if (got==(pid_t)-1)
        {
	  if (errno==EINTR)
	    continue;
	  oops("waitpid");
	  return;
	}

      blog();
      ios("child ");
      ioul((unsigned long)pid);
      ios(" ret ");
      ioxl((unsigned long)st);
      ionl();
      
    } while (got!=pid);
}

void
sleeper(unsigned long delay)
{
  blog();
  ios("sleep ");
  ioul(delay);
  ionl();

  sleep(delay);
}

int
main(int argc, char **argv)
{
  unsigned long	delay;

  if (argc<2)
    ex(USAGE);
  delay	= getul(argv[1]);
  for (;;)
    {
      forker(argv+2);
      sleeper(delay);
    }
  return 0;
}

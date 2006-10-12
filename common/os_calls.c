/*
   Copyright (c) 2004-2006 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   generic operating system calls

   put all the os / arch define in here you want
*/

#if defined(_WIN32)
#include <windows.h>
#include <winsock.h>
#else
/* fix for solaris 10 with gcc 3.3.2 problem */
#if defined(sun) || defined(__sun)
#define ctid_t id_t
#endif
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* for clearenv() */
#if defined(_WIN32)
#else
extern char** environ;
#endif

/* for solaris */
#if !defined(PF_LOCAL)
#define PF_LOCAL AF_UNIX
#endif
#if !defined(INADDR_NONE)
#define INADDR_NONE ((unsigned long)-1)
#endif

/*****************************************************************************/
/* allocate memory, returns a pointer to it, size bytes are allocated,
   if zero is non zero, each byte will be set to zero */
void*
g_malloc(int size, int zero)
{
  char* rv;

  rv = (char*)malloc(size);
  if (zero)
  {
    memset(rv, 0, size);
  }
  return rv;
}

/*****************************************************************************/
/* free the memory pointed to by ptr, ptr can be zero */
void
g_free(void* ptr)
{
  if (ptr != 0)
  {
    free(ptr);
  }
}

/*****************************************************************************/
/* output text to stdout, try to use g_write / g_writeln instead to avoid
   linux / windows EOL problems */
void
g_printf(char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void
g_sprintf(char* dest, char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  vsprintf(dest, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void
g_snprintf(char* dest, int len, char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  vsnprintf(dest, len, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void
g_writeln(char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
#if defined(_WIN32)
  g_printf("\r\n");
#else
  g_printf("\n");
#endif
}

/*****************************************************************************/
void
g_write(char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
}

/*****************************************************************************/
/* produce a hex dump */
void
g_hexdump(char* p, int len)
{
  unsigned char* line;
  int i;
  int thisline;
  int offset;

  line = (unsigned char*)p;
  offset = 0;
  while (offset < len)
  {
    g_printf("%04x ", offset);
    thisline = len - offset;
    if (thisline > 16)
    {
      thisline = 16;
    }
    for (i = 0; i < thisline; i++)
    {
      g_printf("%02x ", line[i]);
    }
    for (; i < 16; i++)
    {
      g_printf("   ");
    }
    for (i = 0; i < thisline; i++)
    {
      g_printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');
    }
    g_printf("\r\n");
    offset += thisline;
    line += thisline;
  }
}

/*****************************************************************************/
void
g_memset(void* ptr, int val, int size)
{
  memset(ptr, val, size);
}

/*****************************************************************************/
void
g_memcpy(void* d_ptr, const void* s_ptr, int size)
{
  memcpy(d_ptr, s_ptr, size);
}

/*****************************************************************************/
int
g_getchar(void)
{
  return getchar();
}

/*****************************************************************************/
int
g_tcp_set_no_delay(int sck)
{
  int i;

  i = 1;
#if defined(_WIN32)
  setsockopt(sck, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(i));
#else
  setsockopt(sck, IPPROTO_TCP, TCP_NODELAY, (void*)&i, sizeof(i));
#endif
  return 0;
}

/*****************************************************************************/
int
g_tcp_socket(void)
{
  int rv;
  int i;

  rv = socket(PF_INET, SOCK_STREAM, 0);
#if defined(_WIN32)
  i = 1;
  setsockopt(rv, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(i));
  i = 1;
  setsockopt(rv, SOL_SOCKET, SO_REUSEADDR, (char*)&i, sizeof(i));
  i = 8192 * 2;
  setsockopt(rv, SOL_SOCKET, SO_SNDBUF, (char*)&i, sizeof(i));
#else
  i = 1;
  setsockopt(rv, IPPROTO_TCP, TCP_NODELAY, (void*)&i, sizeof(i));
  i = 1;
  setsockopt(rv, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(i));
  i = 8192 * 2;
  setsockopt(rv, SOL_SOCKET, SO_SNDBUF, (void*)&i, sizeof(i));
#endif
  return rv;
}

/*****************************************************************************/
int
g_tcp_local_socket(void)
{
#if defined(_WIN32)
  return 0;
#else
  return socket(PF_LOCAL, SOCK_STREAM, 0);
#endif
}

/*****************************************************************************/
void
g_tcp_close(int sck)
{
  if (sck == 0)
  {
    return;
  }
  shutdown(sck, 2);
#if defined(_WIN32)
  closesocket(sck);
#else
  close(sck);
#endif
}

/*****************************************************************************/
int
g_tcp_connect(int sck, char* address, char* port)
{
  struct sockaddr_in s;
  struct hostent* h;

  g_memset(&s, 0, sizeof(struct sockaddr_in));
  s.sin_family = AF_INET;
  s.sin_port = htons(atoi(port));
  s.sin_addr.s_addr = inet_addr(address);
  if (s.sin_addr.s_addr == INADDR_NONE)
  {
    h = gethostbyname(address);
    if (h != 0)
    {
      if (h->h_name != 0)
      {
        if (h->h_addr_list != 0)
        {
          if ((*(h->h_addr_list)) != 0)
          {
            s.sin_addr.s_addr = *((int*)(*(h->h_addr_list)));
          }
        }
      }
    }
  }
  return connect(sck, (struct sockaddr*)&s, sizeof(struct sockaddr_in));
}

/*****************************************************************************/
int
g_tcp_set_non_blocking(int sck)
{
  unsigned long i;

#if defined(_WIN32)
  i = 1;
  ioctlsocket(sck, FIONBIO, &i);
#else
  i = fcntl(sck, F_GETFL);
  i = i | O_NONBLOCK;
  fcntl(sck, F_SETFL, i);
#endif
  return 0;
}

/*****************************************************************************/
int
g_tcp_bind(int sck, char* port)
{
  struct sockaddr_in s;

  memset(&s, 0, sizeof(struct sockaddr_in));
  s.sin_family = AF_INET;
  s.sin_port = htons(atoi(port));
  s.sin_addr.s_addr = INADDR_ANY;
  return bind(sck, (struct sockaddr*)&s, sizeof(struct sockaddr_in));
}

/*****************************************************************************/
int
g_tcp_local_bind(int sck, char* port)
{
#if defined(_WIN32)
  return -1;
#else
  struct sockaddr_un s;

  memset(&s, 0, sizeof(struct sockaddr_un));
  s.sun_family = AF_UNIX;
  strcpy(s.sun_path, port);
  return bind(sck, (struct sockaddr*)&s, sizeof(struct sockaddr_un));
#endif
}

/*****************************************************************************/
int
g_tcp_listen(int sck)
{
  return listen(sck, 2);
}

/*****************************************************************************/
int
g_tcp_accept(int sck)
{
  struct sockaddr_in s;
#if defined(_WIN32)
  signed int i;
#else
  unsigned int i;
#endif

  i = sizeof(struct sockaddr_in);
  memset(&s, 0, i);
  return accept(sck, (struct sockaddr*)&s, &i);
}

/*****************************************************************************/
void
g_sleep(int msecs)
{
#if defined(_WIN32)
  Sleep(msecs);
#else
  usleep(msecs * 1000);
#endif
}

/*****************************************************************************/
int
g_tcp_last_error_would_block(int sck)
{
#if defined(_WIN32)
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EWOULDBLOCK;
#endif
}

/*****************************************************************************/
int
g_tcp_recv(int sck, void* ptr, int len, int flags)
{
#if defined(_WIN32)
  return recv(sck, (char*)ptr, len, flags);
#else
  return recv(sck, ptr, len, flags);
#endif
}

/*****************************************************************************/
int
g_tcp_send(int sck, void* ptr, int len, int flags)
{
#if defined(_WIN32)
  return send(sck, (char*)ptr, len, flags);
#else
  return send(sck, ptr, len, flags);
#endif
}

/*****************************************************************************/
int
g_tcp_select(int sck1, int sck2)
{
  fd_set rfds;
  struct timeval time;
  int max;
  int rv;

  time.tv_sec = 0;
  time.tv_usec = 0;
  FD_ZERO(&rfds);
  if (sck1 > 0)
  {
    FD_SET(((unsigned int)sck1), &rfds);
  }
  if (sck2 > 0)
  {
    FD_SET(((unsigned int)sck2), &rfds);
  }
  max = sck1;
  if (sck2 > max)
  {
    max = sck2;
  }
  rv = select(max + 1, &rfds, 0, 0, &time);
  if (rv > 0)
  {
    rv = 0;
    if (FD_ISSET(((unsigned int)sck1), &rfds))
    {
      rv = rv | 1;
    }
    if (FD_ISSET(((unsigned int)sck2), &rfds))
    {
      rv = rv | 2;
    }
  }
  else
  {
    rv = 0;
  }
  return rv;
}

/*****************************************************************************/
void
g_random(char* data, int len)
{
#if defined(_WIN32)
  memset(data, 0x44, len);
#else
  int fd;

  memset(data, 0x44, len);
  fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1)
  {
    fd = open("/dev/random", O_RDONLY);
  }
  if (fd != -1)
  {
    read(fd, data, len);
    close(fd);
  }
#endif
}

/*****************************************************************************/
int
g_abs(int i)
{
  return abs(i);
}

/*****************************************************************************/
int
g_memcmp(void* s1, void* s2, int len)
{
  return memcmp(s1, s2, len);
}

/*****************************************************************************/
/* returns -1 on error, else return handle or file descriptor */
int
g_file_open(char* file_name)
{
#if defined(_WIN32)
  return (int)CreateFile(file_name, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#else
  int rv;

  rv =  open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (rv == -1)
  {
    /* can't open read / write, try to open read only */
    rv =  open(file_name, O_RDONLY);
  }
  return rv;
#endif
}

/*****************************************************************************/
/* returns error, always 0 */
int
g_file_close(int fd)
{
#if defined(_WIN32)
  CloseHandle((HANDLE)fd);
#else
  close(fd);
#endif
  return 0;
}

/*****************************************************************************/
/* read from file, returns the number of bytes read or -1 on error */
int
g_file_read(int fd, char* ptr, int len)
{
#if defined(_WIN32)
  if (ReadFile((HANDLE)fd, (LPVOID)ptr, (DWORD)len, (LPDWORD)&len, 0))
  {
    return len;
  }
  else
  {
    return -1;
  }
#else
  return read(fd, ptr, len);
#endif
}

/*****************************************************************************/
/* write to file, returns the number of bytes writen or -1 on error */
int
g_file_write(int fd, char* ptr, int len)
{
#if defined(_WIN32)
  if (WriteFile((HANDLE)fd, (LPVOID)ptr, (DWORD)len, (LPDWORD)&len, 0))
  {
    return len;
  }
  else
  {
    return -1;
  }
#else
  return write(fd, ptr, len);
#endif
}

/*****************************************************************************/
/* move file pointer, returns offset on success, -1 on failure */
int
g_file_seek(int fd, int offset)
{
#if defined(_WIN32)
  int rv;

  rv = (int)SetFilePointer((HANDLE)fd, offset, 0, FILE_BEGIN);
  if (rv == (int)INVALID_SET_FILE_POINTER)
  {
    return -1;
  }
  else
  {
    return rv;
  }
#else
  return (int)lseek(fd, offset, SEEK_SET);
#endif
}

/*****************************************************************************/
/* do a write lock on a file */
/* return boolean */
int
g_file_lock(int fd, int start, int len)
{
#if defined(_WIN32)
  return LockFile((HANDLE)fd, start, 0, len, 0);
#else
  struct flock lock;

  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = start;
  lock.l_len = len;
  if (fcntl(fd, F_SETLK, &lock) == -1)
  {
    return 0;
  }
  return 1;
#endif
}

/*****************************************************************************/
/* returns error, always zero */
int
g_set_file_rights(char* filename, int read, int write)
{
#if defined(_WIN32)
  return 0;
#else
  int flags;

  flags = read ? S_IRUSR : 0;
  flags |= write ? S_IWUSR : 0;
  chmod(filename, flags);
  return 0;
#endif
}

/*****************************************************************************/
/* returns error, always zero */
int
g_mkdir(char* dirname)
{
#if defined(_WIN32)
  return 0;
#else
  mkdir(dirname, S_IRWXU);
  return 0;
#endif
}

/*****************************************************************************/
/* gets the current working directory and puts up to maxlen chars in
   dirname 
   always returns 0 */
char*
g_get_current_dir(char* dirname, int maxlen)
{
#if defined(_WIN32)
  GetCurrentDirectory(maxlen, dirname);
  return 0;
#else
  getcwd(dirname, maxlen);
  return 0;
#endif
}

/*****************************************************************************/
/* returns error, zero on success and -1 on failure */
int
g_set_current_dir(char* dirname)
{
#if defined(_WIN32)
  if (SetCurrentDirectory(dirname))
  {
    return 0;
  }
  else
  {
    return -1;
  }
#else
  return chdir(dirname);
#endif
}

/*****************************************************************************/
/* returns boolean, non zero if the file exists */
int
g_file_exist(char* filename)
{
#if defined(_WIN32)
  return 0; // use FindFirstFile();
#else
  return access(filename, F_OK) == 0;
#endif
}

/*****************************************************************************/
/* returns non zero if the file was deleted */
int
g_file_delete(char* filename)
{
#if defined(_WIN32)
  return DeleteFile(filename);
#else
  return unlink(filename) != -1;
#endif
}

/*****************************************************************************/
/* returns length of text */
int
g_strlen(char* text)
{
  if (text == 0)
  {
    return 0;
  }
  return strlen(text);
}

/*****************************************************************************/
/* returns dest */
char*
g_strcpy(char* dest, char* src)
{
  if (src == 0 && dest != 0)
  {
    dest[0] = 0;
    return dest;
  }
  if (dest == 0 || src == 0)
  {
    return 0;
  }
  return strcpy(dest, src);
}

/*****************************************************************************/
/* returns dest */
char*
g_strncpy(char* dest, char* src, int len)
{
  char* rv;

  if (src == 0 && dest != 0)
  {
    dest[0] = 0;
    return dest;
  }
  if (dest == 0 || src == 0)
  {
    return 0;
  }
  rv = strncpy(dest, src, len);
  dest[len] = 0;
  return rv;
}

/*****************************************************************************/
/* returns dest */
char*
g_strcat(char* dest, char* src)
{
  if (dest == 0 || src == 0)
  {
    return dest;
  }
  return strcat(dest, src);
}

/*****************************************************************************/
/* if in = 0, return 0 else return newly alloced copy of in */
char*
g_strdup(char* in)
{
  int len;
  char* p;

  if (in == 0)
  {
    return 0;
  }
  len = g_strlen(in);
  p = (char*)g_malloc(len + 1, 0);
  g_strcpy(p, in);
  return p;
}

/*****************************************************************************/
int
g_strcmp(char* c1, char* c2)
{
  return strcmp(c1, c2);
}

/*****************************************************************************/
int
g_strncmp(char* c1, char* c2, int len)
{
  return strncmp(c1, c2, len);
}

/*****************************************************************************/
int
g_strcasecmp(char* c1, char* c2)
{
#if defined(_WIN32)
  return stricmp(c1, c2);
#else
  return strcasecmp(c1, c2);
#endif
}

/*****************************************************************************/
int
g_strncasecmp(char* c1, char* c2, int len)
{
#if defined(_WIN32)
  return strnicmp(c1, c2, len);
#else
  return strncasecmp(c1, c2, len);
#endif
}

/*****************************************************************************/
int
g_atoi(char* str)
{
  return atoi(str);
}

/*****************************************************************************/
int
g_pos(char* str, char* to_find)
{
  char* pp;

  pp = strstr(str, to_find);
  if (pp == 0)
  {
    return -1;
  }
  return (pp - str);
}

/*****************************************************************************/
long
g_load_library(char* in)
{
#if defined(_WIN32)
  return (long)LoadLibrary(in);
#else
  return (long)dlopen(in, RTLD_LOCAL | RTLD_LAZY);
#endif
}

/*****************************************************************************/
int
g_free_library(long lib)
{
  if (lib == 0)
  {
    return 0;
  }
#if defined(_WIN32)
  return FreeLibrary((HMODULE)lib);
#else
  return dlclose((void*)lib);
#endif
}

/*****************************************************************************/
/* returns NULL if not found */
void*
g_get_proc_address(long lib, char* name)
{
  if (lib == 0)
  {
    return 0;
  }
#if defined(_WIN32)
  return GetProcAddress((HMODULE)lib, name);
#else
  return dlsym((void*)lib, name);
#endif
}

/*****************************************************************************/
int
g_system(char* aexec)
{
#if defined(_WIN32)
  return 0;
#else
  return system(aexec);
#endif
}

/*****************************************************************************/
char*
g_get_strerror(void)
{
#if defined(_WIN32)
  return 0;
#else
  return strerror(errno);
#endif
}

/*****************************************************************************/
int
g_execvp(char* p1, char* args[])
{
#if defined(_WIN32)
  return 0;
#else
  return execvp(p1, args);
#endif
}

/*****************************************************************************/
/* takes up to 30 parameters */
int
g_execlp(int num_params, char* param1, ...)
{
#if defined(_WIN32)
  return 0;
#else
  va_list ap;
  char* p[32];
  int index;

  if (num_params > 30)
  {
    return 0;
  }
  memset(p, 0, sizeof(p));
  p[0] = param1;
  va_start(ap, param1);
  for (index = 1; index < num_params; index++)
  {
    p[index] = va_arg(ap, char*);
  }
  va_end(ap);
  return execlp(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15],
                p[16], p[17], p[18], p[19], p[20], p[21], p[22], p[23],
                p[24], p[25], p[26], p[27], p[28], p[29], p[30], p[31],
                (void*)0);
#endif
}

/*****************************************************************************/
int
g_execlp3(char* a1, char* a2, char* a3)
{
#if defined(_WIN32)
  return 0;
#else
  return execlp(a1, a2, a3, (void*)0);
#endif
}

/*****************************************************************************/
int
g_execlp11(char* a1, char* a2, char* a3, char* a4, char* a5, char* a6,
           char* a7, char* a8, char* a9, char* a10, char* a11)
{
#if defined(_WIN32)
  return 0;
#else
  return execlp(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, (void*)0);
#endif
}

/*****************************************************************************/
int
g_execlp13(char* a1, char* a2, char* a3, char* a4, char* a5, char* a6,
           char* a7, char* a8, char* a9, char* a10, char* a11,
           char* a12, char* a13)
{
#if defined(_WIN32)
  return 0;
#else
  return execlp(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
                a12, a13, (void*)0);
#endif
}

/*****************************************************************************/
void
g_signal(int sig_num, void (*func)(int))
{
#if defined(_WIN32)
#else
  signal(sig_num, func);
#endif
}

/*****************************************************************************/
void
g_signal_child_stop(void (*func)(int))
{
#if defined(_WIN32)
#else
  signal(SIGCHLD, func);
#endif
}

/*****************************************************************************/
void
g_unset_signals(void)
{
#if defined(_WIN32)
#else
  sigset_t mask;

  sigemptyset(&mask);
  sigprocmask(SIG_SETMASK, &mask, NULL);
#endif
}

/*****************************************************************************/
int
g_fork(void)
{
#if defined(_WIN32)
  return 0;
#else
  return fork();
#endif
}

/*****************************************************************************/
int
g_setgid(int pid)
{
#if defined(_WIN32)
  return 0;
#else
  return setgid(pid);
#endif
}

/*****************************************************************************/
/* returns error, zero is success, non zero is error */
int
g_initgroups(const char* user, int gid)
{
#if defined(_WIN32)
  return 0;
#else
  return initgroups(user ,gid);
#endif
}
  
/*****************************************************************************/
int
g_setuid(int pid)
{
#if defined(_WIN32)
  return 0;
#else
  return setuid(pid);
#endif
}

/*****************************************************************************/
int
g_waitchild(void)
{
#if defined(_WIN32)
  return 0;
#else
  int wstat;

  return waitpid(0, &wstat, WNOHANG);
#endif
}

/*****************************************************************************/
int
g_waitpid(int pid)
{
#if defined(_WIN32)
  return 0;
#else
  return waitpid(pid, 0, 0);
#endif
}

/*****************************************************************************/
void
g_clearenv(void)
{
#if defined(_WIN32)
#else
  environ = 0;
#endif
}

/*****************************************************************************/
int
g_setenv(char* name, char* value, int rewrite)
{
#if defined(_WIN32)
  return 0;
#else
  return setenv(name, value, rewrite);
#endif
}

/*****************************************************************************/
char*
g_getenv(char* name)
{
#if defined(_WIN32)
  return 0;
#else
  return getenv(name);
#endif
}

/*****************************************************************************/
int
g_exit(int exit_code)
{
  _exit(exit_code);
  return 0;
}

/*****************************************************************************/
int
g_getpid(void)
{
#if defined(_WIN32)
  return 0;
#else
  return getpid();
#endif
}

/*****************************************************************************/
int
g_sigterm(int pid)
{
#if defined(_WIN32)
  return 0;
#else
  return kill(pid, SIGTERM);
#endif
}

/*****************************************************************************/
/* returns 0 if ok */
int
g_getuser_info(char* username, int* gid, int* uid, char* shell, char* dir,
               char* gecos)
{
#if defined(_WIN32)
  return 1;
#else
  struct passwd* pwd_1;

  pwd_1 = getpwnam(username);
  if (pwd_1 != 0)
  {
    if (gid != 0)
    {
      *gid = pwd_1->pw_gid;
    }
    if (uid != 0)
    {
      *uid = pwd_1->pw_uid;
    }
    if (dir != 0)
    {
      g_strcpy(dir, pwd_1->pw_dir);
    }
    if (shell != 0)
    {
      g_strcpy(shell, pwd_1->pw_shell);
    }
    if (gecos != 0)
    {
      g_strcpy(gecos, pwd_1->pw_gecos);
    }
    return 0;
  }
  return 1;
#endif
}

/*****************************************************************************/
/* returns 0 if ok */
int
g_getgroup_info(char* groupname, int* gid)
{
#if defined(_WIN32)
  return 1;
#else
  struct group* g;

  g = getgrnam(groupname);
  if (g != 0)
  {
    if (gid != 0)
    {
      *gid = g->gr_gid;
    }
    return 0;
  }
  return 1;
#endif
}

/*****************************************************************************/
/* returns error */
/* if zero is returned, then ok is set */
int
g_check_user_in_group(char* username, int gid, int* ok)
{
#if defined(_WIN32)
  return 1;
#else
  struct group* groups;
  int i;

  groups = getgrgid(gid);
  if (groups == 0)
  {
    return 1;
  }
  *ok = 0;
  i = 0;
  while (0 != groups->gr_mem[i])
  {
    if (0 == g_strcmp(groups->gr_mem[i], username))
    {
      *ok = 1;
      break;
    }
    i++;
  }
  return 0;
#endif
}

/*****************************************************************************/
/* returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
   measured in seconds. */
int
g_time1(void)
{
#if defined(_WIN32)
  return GetTickCount() / 1000;
#else
  return time(0);
#endif
}

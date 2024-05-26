#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int is_directory(const char *path)
{
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}
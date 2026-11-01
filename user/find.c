#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

static int
find(char *path, char *name)
{
  int fd = open(path, 0), failed = 0;
  struct stat st;
  struct dirent entry;
  char child[512];
  if(fd < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return 1;
  }
  if(fstat(fd, &st) < 0){
    close(fd);
    return 1;
  }
  char *base = path;
  for(char *p = path; *p; p++)
    if(*p == '/')
      base = p + 1;
  if(strcmp(base, name) == 0)
    printf("%s\n", path);
  if(st.type == T_DIR){
    int len = strlen(path);
    if(len + 1 + DIRSIZ + 1 > sizeof child){
      fprintf(2, "find: path too long: %s\n", path);
      close(fd);
      return 1;
    }
    strcpy(child, path);
    child[len++] = '/';
    while(read(fd, &entry, sizeof entry) == sizeof entry){
      if(entry.inum == 0)
        continue;
      memmove(child + len, entry.name, DIRSIZ);
      child[len + DIRSIZ] = 0;
      if(strcmp(child + len, ".") && strcmp(child + len, ".."))
        failed |= find(child, name);
    }
  }
  close(fd);
  return failed;
}

int
main(int argc, char **argv)
{
  if(argc != 3){
    fprintf(2, "usage: find path name\n");
    exit(1);
  }
  exit(find(argv[1], argv[2]));
}

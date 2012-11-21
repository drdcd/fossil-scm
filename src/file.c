/*
** Copyright (c) 2006 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** File utilities.
**
** Functions named file_* are generic functions that always follow symlinks.
**
** Functions named file_wd_* are to be used for files inside working
** directories. They follow symlinks depending on 'allow-symlinks' setting.
*/
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "file.h"

/*
** On Windows, include the Platform SDK header file.
*/
#ifdef _WIN32
# include <direct.h>
# include <windows.h>
# include <sys/utime.h>
#endif

/*
** The file status information from the most recent stat() call.
**
** Use _stati64 rather than stat on windows, in order to handle files
** larger than 2GB.
*/
#if defined(_WIN32) && (defined(__MSVCRT__) || defined(_MSC_VER))
# undef stat
# define stat _stati64
#endif
/*
** On Windows S_ISLNK always returns FALSE.
*/
#if !defined(S_ISLNK)
# define S_ISLNK(x) (0)
#endif
static int fileStatValid = 0;
static struct stat fileStat;

/*
** Fill stat buf with information received from stat() or lstat().
** lstat() is called on Unix if isWd is TRUE and allow-symlinks setting is on.
**
*/
static int fossil_stat(const char *zFilename, struct stat *buf, int isWd){
#if !defined(_WIN32)
  if( isWd && g.allowSymlinks ){
    return lstat(zFilename, buf);
  }else{
    return stat(zFilename, buf);
  }
#else
  int rc = 0;
  wchar_t *zMbcs = fossil_utf8_to_filename(zFilename);
  rc = _wstati64(zMbcs, buf);
  fossil_mbcs_free(zMbcs);
  return rc;
#endif
}

/*
** Fill in the fileStat variable for the file named zFilename.
** If zFilename==0, then use the previous value of fileStat if
** there is a previous value.
**
** If isWd is TRUE, do lstat() instead of stat() if allow-symlinks is on.
**
** Return the number of errors.  No error messages are generated.
*/
static int getStat(const char *zFilename, int isWd){
  int rc = 0;
  if( zFilename==0 ){
    if( fileStatValid==0 ) rc = 1;
  }else{
    if( fossil_stat(zFilename, &fileStat, isWd)!=0 ){
      fileStatValid = 0;
      rc = 1;
    }else{
      fileStatValid = 1;
      rc = 0;
    }
  }
  return rc;
}

/*
** Return the size of a file in bytes.  Return -1 if the file does not
** exist.  If zFilename is NULL, return the size of the most recently
** stat-ed file.
*/
i64 file_size(const char *zFilename){
  return getStat(zFilename, 0) ? -1 : fileStat.st_size;
}

/*
** Same as file_size(), but takes into account symlinks.
*/
i64 file_wd_size(const char *zFilename){
  return getStat(zFilename, 1) ? -1 : fileStat.st_size;
}

/*
** Return the modification time for a file.  Return -1 if the file
** does not exist.  If zFilename is NULL return the size of the most
** recently stat-ed file.
*/
i64 file_mtime(const char *zFilename){
  return getStat(zFilename, 0) ? -1 : fileStat.st_mtime;
}

/*
** Same as file_mtime(), but takes into account symlinks.
*/
i64 file_wd_mtime(const char *zFilename){
  return getStat(zFilename, 1) ? -1 : fileStat.st_mtime;
}

/*
** Return TRUE if the named file is an ordinary file or symlink
** and symlinks are allowed.
** Return false for directories, devices, fifos, etc.
*/
int file_wd_isfile_or_link(const char *zFilename){
  return getStat(zFilename, 1) ? 0 : S_ISREG(fileStat.st_mode) ||
                                     S_ISLNK(fileStat.st_mode);
}

/*
** Return TRUE if the named file is an ordinary file.  Return false
** for directories, devices, fifos, symlinks, etc.
*/
int file_isfile(const char *zFilename){
  return getStat(zFilename, 0) ? 0 : S_ISREG(fileStat.st_mode);
}

/*
** Same as file_isfile(), but takes into account symlinks.
*/
int file_wd_isfile(const char *zFilename){
  return getStat(zFilename, 1) ? 0 : S_ISREG(fileStat.st_mode);
}

/*
** Create symlink to file on Unix, or plain-text file with
** symlink target if "allow-symlinks" is off or we're on Windows.
**
** Arguments: target file (symlink will point to it), link file
**/
void symlink_create(const char *zTargetFile, const char *zLinkFile){
#if !defined(_WIN32)
  if( g.allowSymlinks ){
    int i, nName;
    char *zName, zBuf[1000];

    nName = strlen(zLinkFile);
    if( nName>=sizeof(zBuf) ){
      zName = mprintf("%s", zLinkFile);
    }else{
      zName = zBuf;
      memcpy(zName, zLinkFile, nName+1);
    }
    nName = file_simplify_name(zName, nName, 0);
    for(i=1; i<nName; i++){
      if( zName[i]=='/' ){
        zName[i] = 0;
          if( file_mkdir(zName, 1) ){
            fossil_fatal_recursive("unable to create directory %s", zName);
            return;
          }
        zName[i] = '/';
      }
    }
    if( zName!=zBuf ) free(zName);

    if( symlink(zTargetFile, zName)!=0 ){
      fossil_fatal_recursive("unable to create symlink \"%s\"", zName);
    }
  }else
#endif
  {
    Blob content;
    blob_set(&content, zTargetFile);
    blob_write_to_file(&content, zLinkFile);
    blob_reset(&content);
  }
}

/*
** Copy symbolic link from zFrom to zTo.
*/
void symlink_copy(const char *zFrom, const char *zTo){
  Blob content;
  blob_read_link(&content, zFrom);
  symlink_create(blob_str(&content), zTo);
  blob_reset(&content);
}

/*
** Return file permissions (normal, executable, or symlink):
**   - PERM_EXE if file is executable;
**   - PERM_LNK on Unix if file is symlink and allow-symlinks option is on;
**   - PERM_REG for all other cases (regular file, directory, fifo, etc).
*/
int file_wd_perm(const char *zFilename){
  if( getStat(zFilename, 1) ) return PERM_REG;
#if defined(_WIN32)
#  ifndef S_IXUSR
#    define S_IXUSR  _S_IEXEC
#  endif
  if( S_ISREG(fileStat.st_mode) && ((S_IXUSR)&fileStat.st_mode)!=0 )
    return PERM_EXE;
  else
    return PERM_REG;
#else
  if( S_ISREG(fileStat.st_mode) &&
      ((S_IXUSR|S_IXGRP|S_IXOTH)&fileStat.st_mode)!=0 )
    return PERM_EXE;
  else if( g.allowSymlinks && S_ISLNK(fileStat.st_mode) )
    return PERM_LNK;
  else
    return PERM_REG;
#endif
}

/*
** Return TRUE if the named file is an executable.  Return false
** for directories, devices, fifos, symlinks, etc.
*/
int file_wd_isexe(const char *zFilename){
  return file_wd_perm(zFilename)==PERM_EXE;
}

/*
** Return TRUE if the named file is a symlink and symlinks are allowed.
** Return false for all other cases.
**
** On Windows, always return False.
*/
int file_wd_islink(const char *zFilename){
  return file_wd_perm(zFilename)==PERM_LNK;
}

/*
** Return 1 if zFilename is a directory.  Return 0 if zFilename
** does not exist.  Return 2 if zFilename exists but is something
** other than a directory.
*/
int file_isdir(const char *zFilename){
  int rc;

  if( zFilename ){
    char *zFN = mprintf("%s", zFilename);
    file_simplify_name(zFN, -1, 0);
    rc = getStat(zFN, 0);
    free(zFN);
  }else{
    rc = getStat(0, 0);
  }
  return rc ? 0 : (S_ISDIR(fileStat.st_mode) ? 1 : 2);
}

/*
** Same as file_isdir(), but takes into account symlinks.
*/
int file_wd_isdir(const char *zFilename){
  int rc;

  if( zFilename ){
    char *zFN = mprintf("%s", zFilename);
    file_simplify_name(zFN, -1, 0);
    rc = getStat(zFN, 1);
    free(zFN);
  }else{
    rc = getStat(0, 1);
  }
  return rc ? 0 : (S_ISDIR(fileStat.st_mode) ? 1 : 2);
}


/*
** Wrapper around the access() system call.
*/
int file_access(const char *zFilename, int flags){
#ifdef _WIN32
  wchar_t *zMbcs = fossil_utf8_to_filename(zFilename);
  int rc = _waccess(zMbcs, flags);
  fossil_mbcs_free(zMbcs);
#else
  int rc = access(zFilename, flags);
#endif
  return rc;
}

/*
** Find an unused filename similar to zBase with zSuffix appended.
**
** Make the name relative to the working directory if relFlag is true.
**
** Space to hold the new filename is obtained form mprintf() and should
** be freed by the caller.
*/
char *file_newname(const char *zBase, const char *zSuffix, int relFlag){
  char *z = 0;
  int cnt = 0;
  z = mprintf("%s-%s", zBase, zSuffix);
  while( file_size(z)>=0 ){
    fossil_free(z);
    z = mprintf("%s-%s-%d", zBase, zSuffix, cnt++);
  }
  if( relFlag ){
    Blob x;
    file_relative_name(z, &x, 0);
    fossil_free(z);
    z = blob_str(&x);
  }
  return z;
}

/*
** Return the tail of a file pathname.  The tail is the last component
** of the path.  For example, the tail of "/a/b/c.d" is "c.d".
*/
const char *file_tail(const char *z){
  const char *zTail = z;
  while( z[0] ){
    if( z[0]=='/' ) zTail = &z[1];
    z++;
  }
  return zTail;
}

/*
** Copy the content of a file from one place to another.
*/
void file_copy(const char *zFrom, const char *zTo){
  FILE *in, *out;
  int got;
  char zBuf[8192];
  in = fossil_fopen(zFrom, "rb");
  if( in==0 ) fossil_fatal("cannot open \"%s\" for reading", zFrom);
  out = fossil_fopen(zTo, "wb");
  if( out==0 ) fossil_fatal("cannot open \"%s\" for writing", zTo);
  while( (got=fread(zBuf, 1, sizeof(zBuf), in))>0 ){
    fwrite(zBuf, 1, got, out);
  }
  fclose(in);
  fclose(out);
}

/*
** Set or clear the execute bit on a file.  Return true if a change
** occurred and false if this routine is a no-op.
*/
int file_wd_setexe(const char *zFilename, int onoff){
  int rc = 0;
#if !defined(_WIN32)
  struct stat buf;
  if( fossil_stat(zFilename, &buf, 1)!=0 || S_ISLNK(buf.st_mode) ) return 0;
  if( onoff ){
    int targetMode = (buf.st_mode & 0444)>>2;
    if( (buf.st_mode & 0111)!=targetMode ){
      chmod(zFilename, buf.st_mode | targetMode);
      rc = 1;
    }
  }else{
    if( (buf.st_mode & 0111)!=0 ){
      chmod(zFilename, buf.st_mode & ~0111);
      rc = 1;
    }
  }
#endif /* _WIN32 */
  return rc;
}

/*
** Set the mtime for a file.
*/
void file_set_mtime(const char *zFilename, i64 newMTime){
#if !defined(_WIN32)
  struct timeval tv[2];
  memset(tv, 0, sizeof(tv[0])*2);
  tv[0].tv_sec = newMTime;
  tv[1].tv_sec = newMTime;
  utimes(zFilename, tv);
#else
  struct _utimbuf tb;
  wchar_t *zMbcs = fossil_utf8_to_filename(zFilename);
  tb.actime = newMTime;
  tb.modtime = newMTime;
  _wutime(zMbcs, &tb);
  fossil_mbcs_free(zMbcs);
#endif
}

/*
** COMMAND: test-set-mtime
**
** Usage: %fossil test-set-mtime FILENAME DATE/TIME
**
** Sets the mtime of the named file to the date/time shown.
*/
void test_set_mtime(void){
  const char *zFile;
  char *zDate;
  i64 iMTime;
  if( g.argc!=4 ){
    usage("test-set-mtime FILENAME DATE/TIME");
  }
  db_open_or_attach(":memory:", "mem");
  iMTime = db_int64(0, "SELECT strftime('%%s',%Q)", g.argv[3]);
  zFile = g.argv[2];
  file_set_mtime(zFile, iMTime);
  iMTime = file_wd_mtime(zFile);
  zDate = db_text(0, "SELECT datetime(%lld, 'unixepoch')", iMTime);
  fossil_print("Set mtime of \"%s\" to %s (%lld)\n", zFile, zDate, iMTime);
}

/*
** Delete a file.
*/
void file_delete(const char *zFilename){
#ifdef _WIN32
  wchar_t *z = fossil_utf8_to_filename(zFilename);
  _wunlink(z);
  fossil_mbcs_free(z);
#else
  unlink(zFilename);
#endif
}

/*
** Create the directory named in the argument, if it does not already
** exist.  If forceFlag is 1, delete any prior non-directory object
** with the same name.
**
** Return the number of errors.
*/
int file_mkdir(const char *zName, int forceFlag){
  int rc = file_wd_isdir(zName);
  if( rc==2 ){
    if( !forceFlag ) return 1;
    file_delete(zName);
  }
  if( rc!=1 ){
#if defined(_WIN32)
    int rc;
    wchar_t *zMbcs = fossil_utf8_to_filename(zName);
    rc = _wmkdir(zMbcs);
    fossil_mbcs_free(zMbcs);
    return rc;
#else
    return mkdir(zName, 0755);
#endif
  }
  return 0;
}

/*
** Return true if the filename given is a valid filename for
** a file in a repository.  Valid filenames follow all of the
** following rules:
**
**     *  Does not begin with "/"
**     *  Does not contain any path element named "." or ".."
**     *  Does not contain any of these characters in the path: "\"
**     *  Does not end with "/".
**     *  Does not contain two or more "/" characters in a row.
**     *  Contains at least one character
*/
int file_is_simple_pathname(const char *z){
  int i;
  char c = z[0];
  if( c=='/' || c==0 ) return 0;
  if( c=='.' ){
    if( z[1]=='/' || z[1]==0 ) return 0;
    if( z[1]=='.' && (z[2]=='/' || z[2]==0) ) return 0;
  }
  for(i=0; (c=z[i])!=0; i++){
    if( c=='\\' ){
      return 0;
    }
    if( c=='/' ){
      if( z[i+1]=='/' ) return 0;
      if( z[i+1]=='.' ){
        if( z[i+2]=='/' || z[i+2]==0 ) return 0;
        if( z[i+2]=='.' && (z[i+3]=='/' || z[i+3]==0) ) return 0;
      }
    }
  }
  if( z[i-1]=='/' ) return 0;
  return 1;
}

/*
** If the last component of the pathname in z[0]..z[j-1] is something
** other than ".." then back it out and return true.  If the last
** component is empty or if it is ".." then return false.
*/
static int backup_dir(const char *z, int *pJ){
  int j = *pJ;
  int i;
  if( j<=0 ) return 0;
  for(i=j-1; i>0 && z[i-1]!='/'; i--){}
  if( z[i]=='.' && i==j-2 && z[i+1]=='.' ) return 0;
  *pJ = i-1;
  return 1;
}

/*
** Simplify a filename by
**
**  * Convert all \ into / on windows
**  * removing any trailing and duplicate /
**  * removing /./
**  * removing /A/../
**
** Changes are made in-place.  Return the new name length.
** If the slash parameter is non-zero, the trailing slash, if any,
** is retained.
*/
int file_simplify_name(char *z, int n, int slash){
  int i, j;
  if( n<0 ) n = strlen(z);

  /* On windows convert all \ characters to / */
#if defined(_WIN32)
  for(i=0; i<n; i++){
    if( z[i]=='\\' ) z[i] = '/';
  }
#endif

  /* Removing trailing "/" characters */
  if ( !slash ){
    while( n>1 && z[n-1]=='/' ){ n--; }
  }

  /* Remove duplicate '/' characters.  Except, two // at the beginning
  ** of a pathname is allowed since this is important on windows. */
  for(i=j=1; i<n; i++){
    z[j++] = z[i];
    while( z[i]=='/' && i<n-1 && z[i+1]=='/' ) i++;
  }
  n = j;

  /* Skip over zero or more initial "./" sequences */
  for(i=0; i<n-1 && z[i]=='.' && z[i+1]=='/'; i+=2){}

  /* Begin copying from z[i] back to z[j]... */
  for(j=0; i<n; i++){
    if( z[i]=='/' ){
      /* Skip over internal "/." directory components */
      if( z[i+1]=='.' && (i+2==n || z[i+2]=='/') ){
        i += 1;
        continue;
      }

      /* If this is a "/.." directory component then back out the
      ** previous term of the directory if it is something other than ".."
      ** or "."
      */
      if( z[i+1]=='.' && i+2<n && z[i+2]=='.' && (i+3==n || z[i+3]=='/')
       && backup_dir(z, &j)
      ){
        i += 2;
        continue;
      }
    }
    if( j>=0 ) z[j] = z[i];
    j++;
  }
  if( j==0 ) z[j++] = '.';
  z[j] = 0;
  return j;
}

/*
** COMMAND: test-simplify-name
**
** %fossil test-simplify-name FILENAME...
**
** Print the simplified versions of each FILENAME.
*/
void cmd_test_simplify_name(void){
  int i;
  char *z;
  for(i=2; i<g.argc; i++){
    z = mprintf("%s", g.argv[i]);
    fossil_print("[%s] -> ", z);
    file_simplify_name(z, -1, 0);
    fossil_print("[%s]\n", z);
    fossil_free(z);
  }
}

/*
** Get the current working directory.
**
** On windows, the name is converted from unicode to UTF8 and all '\\'
** characters are converted to '/'.  No conversions are needed on
** unix.
*/
void file_getcwd(char *zBuf, int nBuf){
#ifdef _WIN32
  char *zPwdUtf8;
  int nPwd;
  int i;
  wchar_t zPwd[2000];
  if( _wgetcwd(zPwd, sizeof(zPwd)/sizeof(zPwd[0])-1)==0 ){
    fossil_fatal("cannot find the current working directory.");
  }
  zPwdUtf8 = fossil_unicode_to_utf8(zPwd);
  nPwd = strlen(zPwdUtf8);
  if( nPwd > nBuf-1 ){
    fossil_fatal("pwd too big: max %d\n", nBuf-1);
  }
  for(i=0; zPwdUtf8[i]; i++) if( zPwdUtf8[i]=='\\' ) zPwdUtf8[i] = '/';
  memcpy(zBuf, zPwdUtf8, nPwd+1);
  fossil_mbcs_free(zPwdUtf8);
#else
  if( getcwd(zBuf, nBuf-1)==0 ){
    if( errno==ERANGE ){
      fossil_fatal("pwd too big: max %d\n", nBuf-1);
    }else{
      fossil_fatal("cannot find current working directory; %s",
                   strerror(errno));
    }
  }
#endif
}

/*
** Return true if zPath is an absolute pathname.  Return false
** if it is relative.
*/
int file_is_absolute_path(const char *zPath){
  if( zPath[0]=='/'
#if defined(_WIN32)
      || zPath[0]=='\\'
      || (strlen(zPath)>3 && zPath[1]==':'
           && (zPath[2]=='\\' || zPath[2]=='/'))
#endif
  ){
    return 1;
  }else{
    return 0;
  }
}

/*
** Compute a canonical pathname for a file or directory.
** Make the name absolute if it is relative.
** Remove redundant / characters
** Remove all /./ path elements.
** Convert /A/../ to just /
** If the slash parameter is non-zero, the trailing slash, if any,
** is retained.
*/
void file_canonical_name(const char *zOrigName, Blob *pOut, int slash){
  if( file_is_absolute_path(zOrigName) ){
#if defined(_WIN32)
    char *zOut;
#endif
    blob_set(pOut, zOrigName);
    blob_materialize(pOut);
#if defined(_WIN32)
    /*
    ** On Windows, normalize the drive letter to upper case.
    */
    zOut = blob_str(pOut);
    if( fossil_isalpha(zOut[0]) && zOut[1]==':' ){
      zOut[0] = fossil_toupper(zOut[0]);
    }
#endif
  }else{
    char zPwd[2000];
    file_getcwd(zPwd, sizeof(zPwd)-strlen(zOrigName));
#if defined(_WIN32)
    /*
    ** On Windows, normalize the drive letter to upper case.
    */
    if( fossil_isalpha(zPwd[0]) && zPwd[1]==':' ){
      zPwd[0] = fossil_toupper(zPwd[0]);
    }
#endif
    blob_zero(pOut);
    blob_appendf(pOut, "%//%/", zPwd, zOrigName);
  }
  blob_resize(pOut, file_simplify_name(blob_buffer(pOut),
                                       blob_size(pOut), slash));
}

/*
** COMMAND:  test-canonical-name
** Usage: %fossil test-canonical-name FILENAME...
**
** Test the operation of the canonical name generator.
** Also test Fossil's ability to measure attributes of a file.
*/
void cmd_test_canonical_name(void){
  int i;
  Blob x;
  blob_zero(&x);
  for(i=2; i<g.argc; i++){
    char zBuf[100];
    const char *zName = g.argv[i];
    file_canonical_name(zName, &x, 0);
    fossil_print("[%s] -> [%s]\n", zName, blob_buffer(&x));
    blob_reset(&x);
    sqlite3_snprintf(sizeof(zBuf), zBuf, "%lld", file_wd_size(zName));
    fossil_print("  file_size   = %s\n", zBuf);
    sqlite3_snprintf(sizeof(zBuf), zBuf, "%lld", file_wd_mtime(zName));
    fossil_print("  file_mtime  = %s\n", zBuf);
    fossil_print("  file_isfile = %d\n", file_wd_isfile(zName));
    fossil_print("  file_isfile_or_link = %d\n",file_wd_isfile_or_link(zName));
    fossil_print("  file_islink = %d\n", file_wd_islink(zName));
    fossil_print("  file_isexe  = %d\n", file_wd_isexe(zName));
    fossil_print("  file_isdir  = %d\n", file_wd_isdir(zName));
  }
}

/*
** Return TRUE if the given filename is canonical.
**
** Canonical names are full pathnames using "/" not "\" and which
** contain no "/./" or "/../" terms.
*/
int file_is_canonical(const char *z){
  int i;
  if( z[0]!='/'
#if defined(_WIN32)
    && (z[0]==0 || z[1]!=':' || z[2]!='/')
#endif
  ) return 0;

  for(i=0; z[i]; i++){
    if( z[i]=='\\' ) return 0;
    if( z[i]=='/' ){
      if( z[i+1]=='.' ){
        if( z[i+2]=='/' || z[i+2]==0 ) return 0;
        if( z[i+2]=='.' && (z[i+3]=='/' || z[i+3]==0) ) return 0;
      }
    }
  }
  return 1;
}

/*
** Return a pointer to the first character in a pathname past the
** drive letter.  This routine is a no-op on unix.
*/
char *file_without_drive_letter(char *zIn){
#ifdef _WIN32
  if( fossil_isalpha(zIn[0]) && zIn[1]==':' ) zIn += 2;
#endif
  return zIn;
}

/*
** Compute a pathname for a file or directory that is relative
** to the current directory.  If the slash parameter is non-zero,
** the trailing slash, if any, is retained.
*/
void file_relative_name(const char *zOrigName, Blob *pOut, int slash){
  char *zPath;
  blob_set(pOut, zOrigName);
  blob_resize(pOut, file_simplify_name(blob_buffer(pOut),
                                       blob_size(pOut), slash));
  zPath = file_without_drive_letter(blob_buffer(pOut));
  if( zPath[0]=='/' ){
    int i, j;
    Blob tmp;
    char *zPwd;
    char zBuf[2000];
    zPwd = zBuf;
    file_getcwd(zBuf, sizeof(zBuf)-20);
    zPwd = file_without_drive_letter(zBuf);
    i = 1;
#ifdef _WIN32
    while( zPath[i] && fossil_tolower(zPwd[i])==fossil_tolower(zPath[i]) ) i++;
#else
    while( zPath[i] && zPwd[i]==zPath[i] ) i++;
#endif
    if( zPath[i]==0 ){
      blob_reset(pOut);
      if( zPwd[i]==0 ){
        blob_append(pOut, ".", 1);
      }else{
        blob_append(pOut, "..", 2);
        for(j=i+1; zPwd[j]; j++){
          if( zPwd[j]=='/' ) {
            blob_append(pOut, "/..", 3);
          }
        }
      }
      return;
    }
    if( zPwd[i]==0 && zPath[i]=='/' ){
      memcpy(&tmp, pOut, sizeof(tmp));
      blob_set(pOut, "./");
      blob_append(pOut, &zPath[i+1], -1);
      blob_reset(&tmp);
      return;
    }
    while( zPath[i-1]!='/' ){ i--; }
    blob_set(&tmp, "../");
    for(j=i; zPwd[j]; j++){
      if( zPwd[j]=='/' ) {
        blob_append(&tmp, "../", 3);
      }
    }
    blob_append(&tmp, &zPath[i], -1);
    blob_reset(pOut);
    memcpy(pOut, &tmp, sizeof(tmp));
  }
}

/*
** COMMAND:  test-relative-name
**
** Test the operation of the relative name generator.
*/
void cmd_test_relative_name(void){
  int i;
  Blob x;
  blob_zero(&x);
  for(i=2; i<g.argc; i++){
    file_relative_name(g.argv[i], &x, 0);
    fossil_print("%s\n", blob_buffer(&x));
    blob_reset(&x);
  }
}

/*
** Compute a pathname for a file relative to the root of the local
** tree.  Return TRUE on success.  On failure, print and error
** message and quit if the errFatal flag is true.  If errFatal is
** false, then simply return 0.
**
** The root of the tree is defined by the g.zLocalRoot variable.
*/
int file_tree_name(const char *zOrigName, Blob *pOut, int errFatal){
  Blob localRoot;
  int nLocalRoot;
  char *zLocalRoot;
  Blob full;
  int nFull;
  char *zFull;

  blob_zero(pOut);
  db_must_be_within_tree();
  file_canonical_name(g.zLocalRoot, &localRoot, 1);
  nLocalRoot = blob_size(&localRoot);
  zLocalRoot = blob_buffer(&localRoot);
  assert( nLocalRoot>0 && zLocalRoot[nLocalRoot-1]=='/' );
  file_canonical_name(zOrigName, &full, 0);
  nFull = blob_size(&full);
  zFull = blob_buffer(&full);

  /* Special case.  zOrigName refers to g.zLocalRoot directory. */
  if( nFull==nLocalRoot-1 && memcmp(zLocalRoot, zFull, nFull)==0 ){
    blob_append(pOut, ".", 1);
    blob_reset(&localRoot);
    blob_reset(&full);
    return 1;
  }

  if( nFull<=nLocalRoot || memcmp(zLocalRoot, zFull, nLocalRoot) ){
    blob_reset(&localRoot);
    blob_reset(&full);
    if( errFatal ){
      fossil_fatal("file outside of checkout tree: %s", zOrigName);
    }
    return 0;
  }
  blob_append(pOut, &zFull[nLocalRoot], nFull-nLocalRoot);
  blob_reset(&localRoot);
  blob_reset(&full);
  return 1;
}

/*
** COMMAND:  test-tree-name
**
** Test the operation of the tree name generator.
*/
void cmd_test_tree_name(void){
  int i;
  Blob x;
  blob_zero(&x);
  for(i=2; i<g.argc; i++){
    if( file_tree_name(g.argv[i], &x, 1) ){
      fossil_print("%s\n", blob_buffer(&x));
      blob_reset(&x);
    }
  }
}

/*
** Parse a URI into scheme, host, port, and path.
*/
void file_parse_uri(
  const char *zUri,
  Blob *pScheme,
  Blob *pHost,
  int *pPort,
  Blob *pPath
){
  int i, j;

  for(i=0; zUri[i] && zUri[i]>='a' && zUri[i]<='z'; i++){}
  if( zUri[i]!=':' ){
    blob_zero(pScheme);
    blob_zero(pHost);
    blob_set(pPath, zUri);
    return;
  }
  blob_init(pScheme, zUri, i);
  i++;
  if( zUri[i]=='/' && zUri[i+1]=='/' ){
    i += 2;
    j = i;
    while( zUri[i] && zUri[i]!='/' && zUri[i]!=':' ){ i++; }
    blob_init(pHost, &zUri[j], i-j);
    if( zUri[i]==':' ){
      i++;
      *pPort = atoi(&zUri[i]);
      while( zUri[i] && zUri[i]!='/' ){ i++; }
    }
  }else{
    blob_zero(pHost);
  }
  if( zUri[i]=='/' ){
    blob_set(pPath, &zUri[i]);
  }else{
    blob_set(pPath, "/");
  }
}

/*
** Construct a random temporary filename into zBuf[].
*/
void file_tempname(int nBuf, char *zBuf){
  static const char *azDirs[] = {
#if defined(_WIN32)
     0, /* GetTempPath */
     0, /* TEMP */
     0, /* TMP */
#else
     "/var/tmp",
     "/usr/tmp",
     "/tmp",
     "/temp",
#endif
     ".",
  };
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  unsigned int i, j;
  const char *zDir = ".";
  int cnt = 0;

#if defined(_WIN32)
  wchar_t zTmpPath[MAX_PATH];

  if( GetTempPathW(MAX_PATH, zTmpPath) ){
    azDirs[0] = fossil_unicode_to_utf8(zTmpPath);
  }

  azDirs[1] = fossil_getenv("TEMP");
  azDirs[2] = fossil_getenv("TMP");
#endif


  for(i=0; i<sizeof(azDirs)/sizeof(azDirs[0]); i++){
    if( azDirs[i]==0 ) continue;
    if( !file_isdir(azDirs[i]) ) continue;
    zDir = azDirs[i];
    break;
  }

  /* Check that the output buffer is large enough for the temporary file
  ** name. If it is not, return SQLITE_ERROR.
  */
  if( (strlen(zDir) + 17) >= (size_t)nBuf ){
    fossil_fatal("insufficient space for temporary filename");
  }

  do{
    if( cnt++>20 ) fossil_panic("cannot generate a temporary filename");
    sqlite3_snprintf(nBuf-17, zBuf, "%s/", zDir);
    j = (int)strlen(zBuf);
    sqlite3_randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
  }while( file_size(zBuf)>=0 );

#if defined(_WIN32)
  fossil_mbcs_free((char *)azDirs[1]);
  fossil_mbcs_free((char *)azDirs[2]);
#endif
}


/*
** Return true if a file named zName exists and has identical content
** to the blob pContent.  If zName does not exist or if the content is
** different in any way, then return false.
*/
int file_is_the_same(Blob *pContent, const char *zName){
  i64 iSize;
  int rc;
  Blob onDisk;

  iSize = file_wd_size(zName);
  if( iSize<0 ) return 0;
  if( iSize!=blob_size(pContent) ) return 0;
  if( file_wd_islink(zName) ){
    blob_read_link(&onDisk, zName);
  }else{
    blob_read_from_file(&onDisk, zName);
  }
  rc = blob_compare(&onDisk, pContent);
  blob_reset(&onDisk);
  return rc==0;
}

/*
** Portable unicode implementation of opendir()
*/
#if INTERFACE

#include <dirent.h>
#if defined(_WIN32)
# define DIR _WDIR
# define dirent _wdirent
# define opendir _wopendir
# define readdir _wreaddir
# define closedir _wclosedir
#endif /* _WIN32 */

#endif /* INTERFACE */



/**************************************************************************
** The following routines translate between MBCS and UTF8 on windows.
** Since everything is always UTF8 on unix, these routines are no-ops
** there.
*/

/*
** Translate MBCS to UTF8.  Return a pointer to the translated text.
** Call fossil_mbcs_free() to deallocate any memory used to store the
** returned pointer when done.
*/
char *fossil_mbcs_to_utf8(const char *zMbcs){
#ifdef _WIN32
  extern char *sqlite3_win32_mbcs_to_utf8(const char*);
  return sqlite3_win32_mbcs_to_utf8(zMbcs);
#else
  return (char*)zMbcs;  /* No-op on unix */
#endif
}

/*
** Translate Unicode to UTF8.  Return a pointer to the translated text.
** Call fossil_mbcs_free() to deallocate any memory used to store the
** returned pointer when done.
*/
char *fossil_unicode_to_utf8(const void *zUnicode){
#ifdef _WIN32
  int nByte = WideCharToMultiByte(CP_UTF8, 0, zUnicode, -1, 0, 0, 0, 0);
  char *zUtf = sqlite3_malloc( nByte );
  if( zUtf==0 ){
    return 0;
  }
  WideCharToMultiByte(CP_UTF8, 0, zUnicode, -1, zUtf, nByte, 0, 0);
  return zUtf;
#else
  return (char *)zUnicode;  /* No-op on unix */
#endif
}

/*
** Translate Unicode (filename) to UTF8.  Return a pointer to the
** translated text.  Call fossil_mbcs_free() to deallocate any
** memory used to store the returned pointer when done.
**
** On Windows, characters in the range U+F001 to U+F07F (private use area)
** are translated in ASCII characters in the range U+0001 - U+007F. The
** only place they can come from are filenames using Cygwin's trick
** to circumvent invalid characters in filenames.
** See: <http://cygwin.com/cygwin-ug-net/using-specialnames.html>
** This way, fossil will work nicely together with the cygwin shell
** handling those filenames. On other shells, the generated filename
** might not be as expected, but apart from that nothing goes wrong.
*/
char *fossil_filename_to_utf8(void *zUnicode){
#ifdef _WIN32
  WCHAR *wUnicode = zUnicode;
  while( *wUnicode != 0 ){
    if ( (*wUnicode & 0xFF80) == 0xF000 ){
      WCHAR converted = (*wUnicode & 0x7F);
      /* Only really convert it when the resulting char is in the given range*/
      if ( (converted < 32) || wcschr(L"\"*<>?|:", converted) ){
        *wUnicode = converted;
      }
    }
    ++wUnicode;
  }
  return fossil_unicode_to_utf8(zUnicode);
#else
  return (char *)zUnicode;  /* No-op on unix */
#endif
}

/*
** Translate UTF8 to MBCS for use in system calls.  Return a pointer to the
** translated text..  Call fossil_mbcs_free() to deallocate any memory
** used to store the returned pointer when done.
*/
char *fossil_utf8_to_mbcs(const char *zUtf8){
#ifdef _WIN32
  extern char *sqlite3_win32_utf8_to_mbcs(const char*);
  return sqlite3_win32_utf8_to_mbcs(zUtf8);
#else
  return (char*)zUtf8;  /* No-op on unix */
#endif
}

/*
** Translate UTF8 to unicode for use in system calls.  Return a pointer to the
** translated text..  Call fossil_mbcs_free() to deallocate any memory
** used to store the returned pointer when done.
*/
void *fossil_utf8_to_unicode(const char *zUtf8){
#ifdef _WIN32
  int nByte = MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, 0, 0);
  wchar_t *zUnicode = sqlite3_malloc( nByte * 2 );
  if( zUnicode==0 ){
    return 0;
  }
  MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zUnicode, nByte);

  return zUnicode;
#else
  return (void *)zUtf8;  /* No-op on unix */
#endif
}

/*
** Translate UTF8 to unicode for use in filename translations.
** Return a pointer to the translated text..  Call fossil_mbcs_free()
** to deallocate any memory used to store the returned pointer when done.
**
** On Windows, characters in the range U+0001 to U+0031 and the
** characters '"', '*', ':', '<', '>', '?', '|' and '\\' are invalid
** to be used. Therefore, translated those to characters in the
** (private use area), in the range U+F001 - U+F07F, so those
** characters never arrive in any Windows API. The filenames might
** look strange in Windows explorer, but in the cygwin shell
** everything looks as expected.
**
** See: <http://cygwin.com/cygwin-ug-net/using-specialnames.html>
**
*/
void *fossil_utf8_to_filename(const char *zUtf8){
#ifdef _WIN32
  WCHAR *zUnicode = fossil_utf8_to_unicode(zUtf8);
  WCHAR *wUnicode = zUnicode;
  /* If path starts with "<drive>:/" or "<drive>:\", don't translate the ':' */
  if ( file_is_absolute_path(zUtf8) ){
    wUnicode += 3;
  }
  while( *wUnicode != '\0' ){
    if ( (*wUnicode < 32) || wcschr(L"\"*<>?|:", *wUnicode) ){
      *wUnicode |= 0xF000;
    }
    ++wUnicode;
  }

  return zUnicode;
#else
  return (void *)zUtf8;  /* No-op on unix */
#endif
}

/*
** Return the value of an environment variable as UTF8.
*/
char *fossil_getenv(const char *zName){
#ifdef _WIN32
  wchar_t *uName = fossil_utf8_to_unicode(zName);
  void *zValue = _wgetenv(uName);
  fossil_mbcs_free(uName);
  if( zValue ) zValue = fossil_unicode_to_utf8(zValue);
#else
  char *zValue = getenv(zName);
#endif
  return zValue;
}

/*
** Display UTF8 on the console.  Return the number of
** Characters written. If stdout or stderr is redirected
** to a file, -1 is returned and nothing is written
** to the console.
*/
int fossil_utf8_to_console(const char *zUtf8, int nByte, int toStdErr){
#ifdef _WIN32
  int nChar;
  wchar_t *zUnicode; /* Unicode version of zUtf8 */
  DWORD dummy;

  static int istty[2] = { -1, -1 };
  if( istty[toStdErr] == -1 ){
    istty[toStdErr] = _isatty(toStdErr + 1) != 0;
  }
  if( !istty[toStdErr] ){
    /* stdout/stderr is not a console. */
    return -1;
  }

  nChar = MultiByteToWideChar(CP_UTF8, 0, zUtf8, nByte, NULL, 0);
  zUnicode = malloc( (nChar + 1) *sizeof(zUnicode[0]) );
  if( zUnicode==0 ){
    return 0;
  }
  nChar = MultiByteToWideChar(CP_UTF8, 0, zUtf8, nByte, zUnicode, nChar);
  if( nChar==0 ){
    free(zUnicode);
    return 0;
  }
  zUnicode[nChar] = '\0';
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE - toStdErr), zUnicode, nChar, &dummy, 0);
  return nChar;
#else
  return -1;  /* No-op on unix */
#endif
}

/*
** Translate MBCS to UTF8.  Return a pointer.  Call fossil_mbcs_free()
** to deallocate any memory used to store the returned pointer when done.
*/
void fossil_mbcs_free(void *zOld){
#ifdef _WIN32
  extern void sqlite3_free(void*);
  sqlite3_free(zOld);
#else
  /* No-op on unix */
#endif
}

/*
** Like fopen() but always takes a UTF8 argument.
*/
FILE *fossil_fopen(const char *zName, const char *zMode){
#ifdef _WIN32
  wchar_t *uMode = fossil_utf8_to_unicode(zMode);
  wchar_t *uName = fossil_utf8_to_filename(zName);
  FILE *f = _wfopen(uName, uMode);
  fossil_mbcs_free(uName);
  fossil_mbcs_free(uMode);
#else
  FILE *f = fopen(zName, zMode);
#endif
  return f;
}

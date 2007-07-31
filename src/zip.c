/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License version 2 as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** You should have received a copy of the GNU General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains code used to generate ZIP archives.
*/
#include <assert.h>
#include <zlib.h>
#include "config.h"
#include "zip.h"

/*
** Write a 16- or 32-bit integer as little-endian into the given buffer.
*/
static void put16(char *z, int v){
  z[0] = v & 0xff;
  z[1] = (v>>8) & 0xff;
}
static void put32(char *z, int v){
  z[0] = v & 0xff;
  z[1] = (v>>8) & 0xff;
  z[2] = (v>>16) & 0xff;
  z[3] = (v>>24) & 0xff;
}

/*
** Variables in which to accumulate a growing ZIP archive.
*/
static Blob body;    /* The body of the ZIP archive */
static Blob toc;     /* The table of contents */
static int nEntry;   /* Number of files */

/*
** Initialize a new ZIP archive.
*/
void zip_open(void){
  blob_zero(&body);
  blob_zero(&toc);
  nEntry = 0;
}

/*
** Append a single file to a growing ZIP archive.
**
** pFile is the file to be appended.  zName is the name
** that the file should be saved as.
*/
int zip_add_file(const char *zName, const Blob *pFile){
  z_stream stream;
  int nameLen;
  int skip;
  int toOut;
  int iStart;
  int iCRC;
  int nByte;
  int nByteCompr;
  char *z;
  char zHdr[30];
  char zBuf[100];
  char zOutBuf[100000];

  /* Fill in as much of the header as we know.
  */
  nameLen = strlen(zName);
  put32(&zHdr[0], 0x04034b50);
  put16(&zHdr[4], 0x0014);
  put16(&zHdr[6], 0);
  put16(&zHdr[8], 8);
  put16(&zHdr[10], 0);
  put16(&zHdr[12], 0);
  put16(&zHdr[26], nameLen);
  put16(&zHdr[28], 0);

  /* Write the header and filename.
  */
  iStart = blob_size(&body);
  blob_append(&body, zHdr, 30);
  blob_append(&body, zName, nameLen);

  /* The first two bytes that come out of the deflate compressor are
  ** some kind of header that ZIP does not use.  So skip the first two
  ** output bytes.
  */
  skip = 2;

  /* Write the compressed file.  Compute the CRC as we progress.
  */
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = 0;
  stream.avail_in = blob_size(pFile);
  stream.next_in = (unsigned char*)blob_buffer(pFile);
  stream.avail_out = sizeof(zOutBuf);
  stream.next_out = (unsigned char*)zOutBuf;
  deflateInit(&stream, 9);
  iCRC = crc32(0, stream.next_in, stream.avail_in);
  while( stream.avail_in>0 ){
    deflate(&stream, 0);
    toOut = sizeof(zOutBuf) - stream.avail_out;
    if( toOut>skip ){
      blob_append(&body, &zOutBuf[skip], toOut - skip);
      skip = 0;
    }else{
      skip -= toOut;
    }
    stream.avail_out = sizeof(zOutBuf);
    stream.next_out = (unsigned char*)zOutBuf;
  }
  do{
    stream.avail_out = sizeof(zOutBuf);
    stream.next_out = (unsigned char*)zOutBuf;
    deflate(&stream, Z_FINISH);
    toOut = sizeof(zOutBuf) - stream.avail_out;
    if( toOut>skip ){
      blob_append(&body, &zOutBuf[skip], toOut - skip);
      skip = 0;
    }else{
      skip -= toOut;
    }
  }while( stream.avail_out==0 );
  nByte = stream.total_in;
  nByteCompr = stream.total_out - 2;
  deflateEnd(&stream);

  /* Go back and write the header, now that we know the compressed file size.
  */
  z = &blob_buffer(&body)[iStart];
  put32(&z[14], iCRC);
  put32(&z[18], nByteCompr);
  put32(&z[22], nByte);

  /* Make an entry in the tables of contents
  */
  memset(zBuf, 0, sizeof(zBuf));
  put32(&zBuf[0], 0x02014b50);
  put16(&zBuf[4], 0x0317);
  put16(&zBuf[6], 0x0014);
  put16(&zBuf[8], 0);
  put16(&zBuf[10], 0x0008);
  put16(&zBuf[12], 0);  
  put16(&zBuf[14], 0);
  put32(&zBuf[16], iCRC);
  put32(&zBuf[20], nByteCompr);
  put32(&zBuf[24], nByte);
  put16(&zBuf[28], nameLen);
  put16(&zBuf[30], 0);
  put16(&zBuf[32], 0);
  put16(&zBuf[34], 1);
  put16(&zBuf[36], 0);
  put32(&zBuf[38], (0100000 | 0644)<<16);
  put32(&zBuf[42], iStart);
  blob_append(&toc, zBuf, 46);
  blob_append(&toc, zName, nameLen);
  nEntry++;
}


/*
** Write the ZIP archive into the given BLOB.
*/
int zip_close(Blob *pZip){
  int iTocStart;
  int iTocEnd;
  char zBuf[30];

  iTocStart = blob_size(&body);
  blob_append(&body, blob_buffer(&toc), blob_size(&toc));
  iTocEnd = blob_size(&body);

  memset(zBuf, 0, sizeof(zBuf));
  put32(&zBuf[0], 0x06054b50);
  put16(&zBuf[4], 0);
  put16(&zBuf[6], 0);
  put16(&zBuf[8], nEntry);
  put16(&zBuf[10], nEntry);
  put32(&zBuf[12], iTocEnd - iTocStart);
  put32(&zBuf[16], iTocStart);
  put16(&zBuf[20], 0);
  blob_append(&body, zBuf, 22);
  blob_reset(&toc);
  *pZip = body;
  blob_zero(&body);
  nEntry = 0;
}

/*
** COMMAND: test-filezip
**
** Generate a ZIP archive specified by the first argument that
** contains files given in the second and subsequent arguments.
*/
void filezip_cmd(void){
  int i;
  Blob zip;
  Blob file;
  if( g.argc<3 ){
    usage("ARCHIVE FILE....");
  }
  zip_open();
  for(i=3; i<g.argc; i++){
    blob_zero(&file);
    blob_read_from_file(&file, g.argv[i]);
    zip_add_file(g.argv[i], &file);
    blob_reset(&file);
  }
  zip_close(&zip);
  blob_write_to_file(&zip, g.argv[2]);
}

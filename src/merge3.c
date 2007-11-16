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
** This module implements a 3-way merge
*/
#include "config.h"
#include "merge3.h"

#if 0
#define DEBUG(X)  X
#else
#define DEBUG(X)
#endif

/*
** Opcodes
*/
#define CPY 0
#define DEL 1
#define INS 2
#define END 3
#define UNK 4

/*
** Compare a single line of text from pV1 and pV2.  If the lines
** are the same, return true.  Return false if they are different.
**
** The cursor on both pV1 and pV2 is unchanged.
*/
static int sameLine(Blob *pV1, Blob *pV2){
  char *z1, *z2;
  int i;

  z1 = blob_buffer(pV1);
  z2 = blob_buffer(pV2);
  for(i=0; z1[i]!='\n' && z1[i]==z2[i]; i++){}
  return z2[i]=='\n' || (z2[i]=='\r' && z2[i+1]=='\n')
          || (z1[i]=='\r' && z2[i]=='\n' && z1[i+1]=='\n');
}

/*
** Do a three-way merge.  Initialize pOut to contain the result.
**
** The merge is an edit against pV2.  Both pV1 and pV2 have a
** common origin at pPivot.  Apply the changes of pPivot ==> pV1
** to pV2.
**
** The return is 0 upon complete success. If any input file is binary,
** -1 is returned and pOut is unmodified.  If there are merge
** conflicts, the merge proceeds as best as it can and the number 
** of conflicts is returns
*/
int blob_merge(Blob *pPivot, Blob *pV1, Blob *pV2, Blob *pOut){
  int *aC1;  /* Changes from pPivot to pV1 */
  int *aC2;  /* Changes from pPivot to pV2 */
  int i1, i2;
  int op1, op2;
  int limit1, limit2;
  int inConflict = 0;
  int nConflict = 0;
  static const char zBegin[] = ">>>>>>>> BEGIN MERGE CONFLICT <<<<<<<<\n";
  static const char zEnd[]   = ">>>>>>>>> END MERGE CONFLICT <<<<<<<<<\n";

  aC1 = text_diff(pPivot, pV1, 0, 0);
  aC2 = text_diff(pPivot, pV2, 0, 0);

  if( aC2==0 || aC2==0 ){
    free(aC1);
    free(aC2);
    return -1;
  }
  blob_zero(pOut);
  blob_rewind(pV1);
  blob_rewind(pV2);
  blob_rewind(pPivot);

  for(i1=0; aC1[i1] || aC1[i1+1] || aC1[i1+2]; i1+=3){}
  limit1 = i1;
  for(i2=0; aC2[i2] || aC2[i2+1] || aC2[i2+2]; i2+=3){}
  limit2 = i2;

  DEBUG(
    for(i1=0; i1<limit1; i1+=3){
      printf("c1: %4d %4d %4d\n", aC1[i1], aC1[i1+1], aC1[i1+2]);
    }
    for(i2=0; i2<limit2; i2+=3){
     printf("c2: %4d %4d %4d\n", aC2[i2], aC2[i2+1], aC2[i2+2]);
    }
  )

  op1 = op2 = UNK;
  i1 = i2 = 0;
  while( i1<limit1 && aC1[i1]==0 ){ i1++; }
  while( i2<limit2 && aC2[i2]==0 ){ i2++; }

  while(1){
    if( op1==UNK ){
      if( i1>=limit1 ){
        op1 = END;
        DEBUG( printf("get op1=END\n"); )
      }else{
        op1 = i1 % 3;
        aC1[i1]--;
        DEBUG( printf("get op1=%d from %d (%d->%d)\n",
               op1,i1,aC1[i1]+1,aC1[i1]); )
        while( i1<limit1 && aC1[i1]==0 ){ i1++; }
      }
    }
    if( op2==UNK ){
      if( i2>=limit2 ){
        op2 = END;
        DEBUG( printf("get op2=END\n"); )
      }else{
        op2 = i2 % 3;
        aC2[i2]--;
        DEBUG( printf("get op2=%d from %d (%d->%d)\n",
               op2,i2,aC2[i2]+1,aC2[i2]); )
        while( i2<limit2 && aC2[i2]==0 ){ i2++; }
      }
    }
    DEBUG( printf("op1=%d op2=%d\n", op1, op2); )
    if( op1==END ){
      if( op2==INS ){
        blob_copy_lines(pOut, pV2, 1);
        op2 = UNK;
      }else{
        break;
      }
    }else if( op2==END ){
      if( op1==INS ){
        blob_copy_lines(pOut, pV1, 1);
        op1 = UNK;
      }else{
        break;
      }
    }else if( op1==INS && op2==INS ){
      if( !inConflict && sameLine(pV1, pV2) ){
        blob_copy_lines(pOut, pV2, 1);
        blob_copy_lines(0, pV1, 0);
        op1 = UNK;
        op2 = UNK;
      }else{
        if( !inConflict ){
          inConflict = 1;
          nConflict++;
          blob_appendf(pOut, zBegin);
        }
        blob_copy_lines(pOut, pV1, 1);
        op1 = UNK;
      }
    }else if( op1==INS ){
      blob_copy_lines(pOut, pV1, 1);
      op1 = UNK;
    }else if( op2==INS ){
      blob_copy_lines(pOut, pV2, 1);
      op2 = UNK;
    }else{
      if( inConflict ){
        inConflict = 0;
        blob_appendf(pOut, zEnd);
      }
      if( op1==DEL || op2==DEL ){
        blob_copy_lines(0, pPivot, 1);
        if( op2==CPY ){
          blob_copy_lines(0, pV2, 1);
        }
        if( op1==CPY ){
          blob_copy_lines(0, pV1, 1);
        }
      }else{
        assert( op1==CPY && op2==CPY );
        blob_copy_lines(pOut, pPivot, 1);
        blob_copy_lines(0, pV1, 1);
        blob_copy_lines(0, pV2, 1);
      }
      op1 = UNK;
      op2 = UNK;
    }
  }
  if( inConflict ){
    blob_appendf(pOut, zEnd);
  }

  free(aC1);
  free(aC2);
  return nConflict;
}

/*
** COMMAND:  test-3-way-merge
**
** Combine change in going from PIVOT->VERSION1 with the change going
** from PIVOT->VERSION2 and write the combined changes into MERGED.
*/
void delta_3waymerge_cmd(void){
  Blob pivot, v1, v2, merged;
  if( g.argc!=6 ){
    fprintf(stderr,"Usage: %s %s PIVOT V1 V2 MERGED\n", g.argv[0], g.argv[1]);
    exit(1);
  }
  if( blob_read_from_file(&pivot, g.argv[2])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[2]);
    exit(1);
  }
  if( blob_read_from_file(&v1, g.argv[3])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[3]);
    exit(1);
  }
  if( blob_read_from_file(&v2, g.argv[4])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[4]);
    exit(1);
  }
  blob_merge(&pivot, &v1, &v2, &merged);
  if( blob_write_to_file(&merged, g.argv[5])<blob_size(&merged) ){
    fprintf(stderr,"cannot write %s\n", g.argv[4]);
    exit(1);
  }
  blob_reset(&pivot);
  blob_reset(&v1);
  blob_reset(&v2);
  blob_reset(&merged);
}

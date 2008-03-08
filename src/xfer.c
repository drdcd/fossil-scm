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
** This file contains code to implement the file transfer protocol.
*/
#include "config.h"
#include "xfer.h"

/*
** This structure holds information about the current state of either
** a client or a server that is participating in xfer.
*/
typedef struct Xfer Xfer;
struct Xfer {
  Blob *pIn;          /* Input text from the other side */
  Blob *pOut;         /* Compose our reply here */
  Blob line;          /* The current line of input */
  Blob aToken[5];     /* Tokenized version of line */
  Blob err;           /* Error message text */
  int nToken;         /* Number of tokens in line */
  int nIGotSent;      /* Number of "igot" messages sent */
  int nGimmeSent;     /* Number of gimme messages sent */
  int nFileSent;      /* Number of files sent */
  int nDeltaSent;     /* Number of deltas sent */
  int nFileRcvd;      /* Number of files received */
  int nDeltaRcvd;     /* Number of deltas received */
  int nDanglingFile;  /* Number of dangling deltas received */
  int mxSend;         /* Stop sending "file" with pOut reaches this size */
};


/*
** The input blob contains a UUID.  Convert it into a record ID.
** Create a phantom record if no prior record exists and
** phantomize is true.
**
** Compare to uuid_to_rid().  This routine takes a blob argument
** and does less error checking.
*/
static int rid_from_uuid(Blob *pUuid, int phantomize){
  int rid = db_int(0, "SELECT rid FROM blob WHERE uuid='%b'", pUuid);
  if( rid==0 && phantomize ){
    rid = content_put(0, blob_str(pUuid), 0);
  }
  return rid;
}

/*
** Remember that the other side of the connection already has a copy
** of the file rid.
*/
static void remote_has(int rid){
  db_multi_exec("INSERT OR IGNORE INTO onremote VALUES(%d)", rid);
}

/*
** The aToken[0..nToken-1] blob array is a parse of a "file" line 
** message.  This routine finishes parsing that message and does
** a record insert of the file.
**
** The file line is in one of the following two forms:
**
**      file UUID SIZE \n CONTENT
**      file UUID DELTASRC SIZE \n CONTENT
**
** The content is SIZE bytes immediately following the newline.
** If DELTASRC exists, then the CONTENT is a delta against the
** content of DELTASRC.
**
** If any error occurs, write a message into pErr which has already
** be initialized to an empty string.
*/
static void xfer_accept_file(Xfer *pXfer){
  int n;
  int rid;
  int srcid = 0;
  Blob content, hash;
  
  if( pXfer->nToken<3 
   || pXfer->nToken>4
   || !blob_is_uuid(&pXfer->aToken[1])
   || !blob_is_int(&pXfer->aToken[pXfer->nToken-1], &n)
   || n<0
   || (pXfer->nToken==4 && !blob_is_uuid(&pXfer->aToken[2]))
  ){
    blob_appendf(&pXfer->err, "malformed file line");
    return;
  }
  blob_zero(&content);
  blob_zero(&hash);
  blob_extract(pXfer->pIn, n, &content);
  if( db_exists("SELECT 1 FROM shun WHERE uuid=%B", &pXfer->aToken[1]) ){
    /* Ignore files that have been shunned */
    return;
  }
  if( pXfer->nToken==4 ){
    Blob src;
    srcid = rid_from_uuid(&pXfer->aToken[2], 1);
    if( content_get(srcid, &src)==0 ){
      content_put(&content, blob_str(&pXfer->aToken[1]), srcid);
      blob_appendf(pXfer->pOut, "gimme %b\n", &pXfer->aToken[2]);
      pXfer->nGimmeSent++;
      pXfer->nDanglingFile++;
      return;
    }
    pXfer->nDeltaRcvd++;
    blob_delta_apply(&src, &content, &content);
    blob_reset(&src);
  }else{
    pXfer->nFileRcvd++;
  }
  sha1sum_blob(&content, &hash);
  if( !blob_eq_str(&pXfer->aToken[1], blob_str(&hash), -1) ){
    blob_appendf(&pXfer->err, "content does not match sha1 hash");
  }
  blob_reset(&hash);
  rid = content_put(&content, 0, 0);
  if( rid==0 ){
    blob_appendf(&pXfer->err, "%s", g.zErrMsg);
  }else{
    manifest_crosslink(rid, &content);
  }
  remote_has(rid);
}

/*
** Try to send a file as a delta against its parent.
** If successful, return the number of bytes in the delta.
** If we cannot generate an appropriate delta, then send
** nothing and return zero.
*/
static int send_delta_parent(
  Xfer *pXfer,            /* The transfer context */
  int rid,                /* record id of the file to send */
  Blob *pContent,         /* The content of the file to send */
  Blob *pUuid             /* The UUID of the file to send */
){
  static const char *azQuery[] = {
    "SELECT pid FROM plink x"
    " WHERE cid=%d"
    "   AND NOT EXISTS(SELECT 1 FROM phantom WHERE rid=pid)"
    "   AND NOT EXISTS(SELECT 1 FROM plink y"
                      " WHERE y.pid=x.cid AND y.cid=x.pid)",

    "SELECT pid FROM mlink x"
    " WHERE fid=%d"
    "   AND NOT EXISTS(SELECT 1 FROM phantom WHERE rid=pid)"
    "   AND NOT EXISTS(SELECT 1 FROM mlink y"
                     "  WHERE y.pid=x.fid AND y.fid=x.pid)"
  };
  int i;
  Blob src, delta;
  int size = 0;
  int srcId = 0;

  for(i=0; srcId==0 && i<count(azQuery); i++){
    srcId = db_int(0, azQuery[i], rid);
  }
  if( srcId>0 && content_get(srcId, &src) ){
    char *zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", srcId);
    blob_delta_create(&src, pContent, &delta);
    size = blob_size(&delta);
    if( size>=blob_size(pContent)-50 ){
      size = 0;
    }else{
      blob_appendf(pXfer->pOut, "file %b %s %d\n", pUuid, zUuid, size);
      blob_append(pXfer->pOut, blob_buffer(&delta), size);
      /* blob_appendf(pXfer->pOut, "\n", 1); */
    }
    blob_reset(&delta);
    free(zUuid);
    blob_reset(&src);
  }
  return size;
}

/*
** Try to send a file as a native delta.  
** If successful, return the number of bytes in the delta.
** If we cannot generate an appropriate delta, then send
** nothing and return zero.
*/
static int send_delta_native(
  Xfer *pXfer,            /* The transfer context */
  int rid,                /* record id of the file to send */
  Blob *pUuid             /* The UUID of the file to send */
){
  Blob src, delta;
  int size = 0;
  int srcId;

  srcId = db_int(0, "SELECT srcid FROM delta WHERE rid=%d", rid);
  if( srcId>0 ){
    blob_zero(&delta);
    db_blob(&delta, "SELECT content FROM blob WHERE rid=%d", rid);
    blob_uncompress(&delta, &delta);
    blob_zero(&src);
    db_blob(&src, "SELECT uuid FROM blob WHERE rid=%d", srcId);
    blob_appendf(pXfer->pOut, "file %b %b %d\n",
                pUuid, &src, blob_size(&delta));
    blob_append(pXfer->pOut, blob_buffer(&delta), blob_size(&delta));
    size = blob_size(&delta);
    blob_reset(&delta);
    blob_reset(&src);
  }else{
    size = 0;
  }
  return size;
}

/*
** Send the file identified by rid.
**
** The pUuid can be NULL in which case the correct UUID is computed
** from the rid.
**
** Try to send the file as a native delta if nativeDelta is true, or
** as a parent delta if nativeDelta is false.
*/
static void send_file(Xfer *pXfer, int rid, Blob *pUuid, int nativeDelta){
  Blob content, uuid;
  int size = 0;

  if( db_exists("SELECT 1 FROM onremote WHERE rid=%d", rid) ){
     return;
  }
  blob_zero(&uuid);
  if( pUuid==0 ){
    db_blob(&uuid, "SELECT uuid FROM blob WHERE rid=%d AND size>=0", rid);
    if( blob_size(&uuid)==0 ){
      return;
    }
    pUuid = &uuid;
  }
  if( pXfer->mxSend<=blob_size(pXfer->pOut) ){
    blob_appendf(pXfer->pOut, "igot %b\n", pUuid);
    pXfer->nIGotSent++;
    blob_reset(&uuid);
    return;
  }
  if( nativeDelta ){
    size = send_delta_native(pXfer, rid, pUuid);
    if( size ){
      pXfer->nDeltaSent++;
    }
  }
  if( size==0 ){
    content_get(rid, &content);

    if( !nativeDelta && blob_size(&content)>100 ){
      size = send_delta_parent(pXfer, rid, &content, pUuid);
    }
    if( size==0 ){
      int size = blob_size(&content);
      blob_appendf(pXfer->pOut, "file %b %d\n", pUuid, size);
      blob_append(pXfer->pOut, blob_buffer(&content), size);
      pXfer->nFileSent++;
    }else{
      pXfer->nDeltaSent++;
    }
  }
  remote_has(rid);
  blob_reset(&uuid);
}

/*
** Send a gimme message for every phantom.
*/
static void request_phantoms(Xfer *pXfer, int maxReq){
  Stmt q;
  db_prepare(&q, 
    "SELECT uuid FROM phantom JOIN blob USING(rid)"
    " WHERE NOT EXISTS(SELECT 1 FROM shun WHERE uuid=blob.uuid)"
  );
  while( db_step(&q)==SQLITE_ROW && maxReq-- > 0 ){
    const char *zUuid = db_column_text(&q, 0);
    blob_appendf(pXfer->pOut, "gimme %s\n", zUuid);
    pXfer->nGimmeSent++;
  }
  db_finalize(&q);
}

/*
** Compute an SHA1 hash on the tail of pMsg.  Verify that it matches the
** the hash given in pHash.  Return 1 on a successful match.  Return 0
** if there is a mismatch.
*/
static int check_tail_hash(Blob *pHash, Blob *pMsg){
  Blob tail;
  Blob h2;
  int rc;
  blob_tail(pMsg, &tail);
  sha1sum_blob(&tail, &h2);
  rc = blob_compare(pHash, &h2);
  blob_reset(&h2);
  blob_reset(&tail);
  return rc==0;
}


/*
** Check the signature on an application/x-fossil payload received by
** the HTTP server.  The signature is a line of the following form:
**
**        login LOGIN NONCE SIGNATURE
**
** The NONCE is the SHA1 hash of the remainder of the input.  
** SIGNATURE is the SHA1 checksum of the NONCE concatenated 
** with the users password.
**
** The parameters to this routine are ephermeral blobs holding the
** LOGIN, NONCE and SIGNATURE.
**
** This routine attempts to locate the user and verify the signature.
** If everything checks out, the USER.CAP column for the USER table
** is consulted to set privileges in the global g variable.
**
** If anything fails to check out, no changes are made to privileges.
**
** Signature generation on the client side is handled by the 
** http_exchange() routine.
*/
void check_login(Blob *pLogin, Blob *pNonce, Blob *pSig){
  Stmt q;
  int rc;

  db_prepare(&q, "SELECT pw, cap, uid FROM user WHERE login=%B", pLogin);
  if( db_step(&q)==SQLITE_ROW ){
    Blob pw, combined, hash;
    blob_zero(&pw);
    db_ephemeral_blob(&q, 0, &pw);
    blob_zero(&combined);
    blob_copy(&combined, pNonce);
    blob_append(&combined, blob_buffer(&pw), blob_size(&pw));
    /* CGIDEBUG(("presig=[%s]\n", blob_str(&combined))); */
    sha1sum_blob(&combined, &hash);
    rc = blob_compare(&hash, pSig);
    blob_reset(&hash);
    blob_reset(&combined);
    if( rc==0 ){
      const char *zCap;
      zCap = db_column_text(&q, 1);
      login_set_capabilities(zCap);
      g.userUid = db_column_int(&q, 2);
      g.zLogin = mprintf("%b", pLogin);
      g.zNonce = mprintf("%b", pNonce);
    }
  }
  db_reset(&q);
}

/*
** Send the content of all files in the unsent table.
**
** This is really just an optimization.  If you clear the
** unsent table, all the right files will still get transferred.
** It just might require an extra round trip or two.
*/
static void send_unsent(Xfer *pXfer){
  Stmt q;
  db_prepare(&q, "SELECT rid FROM unsent");
  while( db_step(&q)==SQLITE_ROW ){
    int rid = db_column_int(&q, 0);
    send_file(pXfer, rid, 0, 0);
  }
  db_finalize(&q);
  db_multi_exec("DELETE FROM unsent");
}

/*
** Check to see if the number of unclustered entries is greater than
** 100 and if it is, form a new cluster.  Unclustered phantoms do not
** count toward the 100 total.  And phantoms are never added to a new
** cluster.
*/
static void create_cluster(void){
  Blob cluster, cksum;
  Stmt q;
  int nUncl;
  nUncl = db_int(0, "SELECT count(*) FROM unclustered"
                    " WHERE NOT EXISTS(SELECT 1 FROM phantom"
                                      " WHERE rid=unclustered.rid)");
  if( nUncl<100 ){
    return;
  }
  blob_zero(&cluster);
  db_prepare(&q, "SELECT uuid FROM unclustered, blob"
                 " WHERE NOT EXISTS(SELECT 1 FROM phantom"
                 "                   WHERE rid!=unclustered.rid)"
                 "   AND unclustered.rid=blob.rid"
                 " ORDER BY 1");
  while( db_step(&q)==SQLITE_ROW ){
    blob_appendf(&cluster, "M %s\n", db_column_text(&q, 0));
  }
  db_finalize(&q);
  md5sum_blob(&cluster, &cksum);
  blob_appendf(&cluster, "Z %b\n", &cksum);
  blob_reset(&cksum);
  db_multi_exec("DELETE FROM unclustered");
  content_put(&cluster, 0, 0);
  blob_reset(&cluster);
}

/*
** Send an igot message for every entry in unclustered table.
** Return the number of messages sent.
*/
static int send_unclustered(Xfer *pXfer){
  Stmt q;
  int cnt = 0;
  db_prepare(&q, 
    "SELECT uuid FROM unclustered JOIN blob USING(rid)"
    " WHERE NOT EXISTS(SELECT 1 FROM shun WHERE uuid=blob.uuid)"
  );
  while( db_step(&q)==SQLITE_ROW ){
    blob_appendf(pXfer->pOut, "igot %s\n", db_column_text(&q, 0));
    cnt++;
  }
  db_finalize(&q);
  return cnt;
}

/*
** If this variable is set, disable login checks.  Used for debugging
** only.
*/
static int disableLogin = 0;

/*
** WEBPAGE: xfer
**
** This is the transfer handler on the server side.  The transfer
** message has been uncompressed and placed in the g.cgiIn blob.
** Process this message and form an appropriate reply.
*/
void page_xfer(void){
  int isPull = 0;
  int isPush = 0;
  int nErr = 0;
  Xfer xfer;
  int deltaFlag = 0;

  memset(&xfer, 0, sizeof(xfer));
  blobarray_zero(xfer.aToken, count(xfer.aToken));
  cgi_set_content_type(g.zContentType);
  blob_zero(&xfer.err);
  xfer.pIn = &g.cgiIn;
  xfer.pOut = cgi_output_blob();
  xfer.mxSend = db_get_int("max-download", 5000000);

  db_begin_transaction();
  db_multi_exec(
     "CREATE TEMP TABLE onremote(rid INTEGER PRIMARY KEY);"
  );
  while( blob_line(xfer.pIn, &xfer.line) ){
    xfer.nToken = blob_tokenize(&xfer.line, xfer.aToken, count(xfer.aToken));

    /*   file UUID SIZE \n CONTENT
    **   file UUID DELTASRC SIZE \n CONTENT
    **
    ** Accept a file from the client.
    */
    if( blob_eq(&xfer.aToken[0], "file") ){
      if( !isPush ){
        cgi_reset_content();
        @ error not\sauthorized\sto\swrite
        nErr++;
        break;
      }
      xfer_accept_file(&xfer);
      if( blob_size(&xfer.err) ){
        cgi_reset_content();
        @ error %T(blob_str(&xfer.err))
        nErr++;
        break;
      }
    }else

    /*   gimme UUID
    **
    ** Client is requesting a file.  Send it.
    */
    if( blob_eq(&xfer.aToken[0], "gimme")
     && xfer.nToken==2
     && blob_is_uuid(&xfer.aToken[1])
    ){
      if( isPull ){
        int rid = rid_from_uuid(&xfer.aToken[1], 0);
        if( rid ){
          send_file(&xfer, rid, &xfer.aToken[1], deltaFlag);
        }
      }
    }else

    /*   igot UUID
    **
    ** Client announces that it has a particular file.
    */
    if( xfer.nToken==2
     && blob_eq(&xfer.aToken[0], "igot")
     && blob_is_uuid(&xfer.aToken[1])
    ){
      if( isPush ){
        rid_from_uuid(&xfer.aToken[1], 1);
      }
    }else
  
    
    /*    pull  SERVERCODE  PROJECTCODE
    **    push  SERVERCODE  PROJECTCODE
    **
    ** The client wants either send or receive.  The server should
    ** verify that the project code matches and that the server code
    ** does not match.
    */
    if( xfer.nToken==3
     && (blob_eq(&xfer.aToken[0], "pull") || blob_eq(&xfer.aToken[0], "push"))
     && blob_is_uuid(&xfer.aToken[1])
     && blob_is_uuid(&xfer.aToken[2])
    ){
      const char *zSCode;
      const char *zPCode;

      zSCode = db_get("server-code", 0);
      if( zSCode==0 ){
        fossil_panic("missing server code");
      }
      if( blob_eq_str(&xfer.aToken[1], zSCode, -1) ){
        cgi_reset_content();
        @ error server\sloop
        nErr++;
        break;
      }
      zPCode = db_get("project-code", 0);
      if( zPCode==0 ){
        fossil_panic("missing project code");
      }
      if( !blob_eq_str(&xfer.aToken[2], zPCode, -1) ){
        cgi_reset_content();
        @ error wrong\sproject
        nErr++;
        break;
      }
      login_check_credentials();
      if( blob_eq(&xfer.aToken[0], "pull") ){
        if( !g.okRead ){
          cgi_reset_content();
          @ error not\sauthorized\sto\sread
          nErr++;
          break;
        }
        isPull = 1;
      }else{
        if( !g.okWrite ){
          if( !isPull ){
            cgi_reset_content();
            @ error not\sauthorized\sto\swrite
            nErr++;
          }else{
            @ message pull\sonly\s-\snot\sauthorized\sto\spush
          }
        }else{
          isPush = 1;
        }
      }
    }else

    /*    clone
    **
    ** The client knows nothing.  Tell all.
    */
    if( blob_eq(&xfer.aToken[0], "clone") ){
      login_check_credentials();
      if( !g.okClone ){
        cgi_reset_content();
        @ error not\sauthorized\sto\sclone
        nErr++;
        break;
      }
      isPull = 1;
      deltaFlag = 1;
      @ push %s(db_get("server-code", "x")) %s(db_get("project-code", "x"))
    }else

    /*    login  USER  NONCE  SIGNATURE
    **
    ** Check for a valid login.  This has to happen before anything else.
    ** The client can send multiple logins.  Permissions are cumulative.
    */
    if( blob_eq(&xfer.aToken[0], "login")
     && xfer.nToken==4
    ){
      if( disableLogin ){
        g.okRead = g.okWrite = 1;
      }else if( check_tail_hash(&xfer.aToken[2], xfer.pIn) ){
        check_login(&xfer.aToken[1], &xfer.aToken[2], &xfer.aToken[3]);
      }
    }else
    
    /*    cookie TEXT
    **
    ** A cookie contains a arbitrary-length argument that is server-defined.
    ** The argument must be encoded so as not to contain any whitespace.
    ** The server can optionally send a cookie to the client.  The client
    ** might then return the same cookie back to the server on its next
    ** communication.  The cookie might record information that helps
    ** the server optimize a push or pull.
    **
    ** The client is not required to return a cookie.  So the server
    ** must not depend on the cookie.  The cookie should be an optimization
    ** only.  The client might also send a cookie that came from a different
    ** server.  So the server must be prepared to distinguish its own cookie
    ** from cookies originating from other servers.  The client might send
    ** back several different cookies to the server.  The server should be
    ** prepared to sift through the cookies and pick the one that it wants.
    */
    if( blob_eq(&xfer.aToken[0], "cookie") && xfer.nToken==2 ){
      /* Process the cookie */
    }else

    /* Unknown message
    */
    {
      cgi_reset_content();
      @ error bad\scommand:\s%F(blob_str(&xfer.line))
    }
    blobarray_reset(xfer.aToken, xfer.nToken);
  }
  if( isPush ){
    request_phantoms(&xfer, 500);
  }
  if( isPull ){
    create_cluster();
    send_unclustered(&xfer);
  }
  db_end_transaction(0);
}

/*
** COMMAND: test-xfer
**
** This command is used for debugging the server.  There is a single
** argument which is the uncompressed content of an "xfer" message
** from client to server.  This command interprets that message as
** if had been received by the server.
**
** On the client side, run:
**
**      fossil push http://bogus/ --httptrace
**
** Or a similar command to provide the output.  The content of the
** message will appear on standard output.  Capture this message
** into a file named (for example) out.txt.  Then run the
** server in gdb:
**
**     gdb fossil
**     r test-xfer out.txt
*/
void cmd_test_xfer(void){
  int notUsed;
  if( g.argc!=2 && g.argc!=3 ){
    usage("?MESSAGEFILE?");
  }
  db_must_be_within_tree();
  blob_zero(&g.cgiIn);
  blob_read_from_file(&g.cgiIn, g.argc==2 ? "-" : g.argv[2]);
  disableLogin = 1;
  page_xfer();
  printf("%s\n", cgi_extract_content(&notUsed));
}


/*
** Sync to the host identified in g.urlName and g.urlPath.  This
** routine is called by the client.
**
** Records are pushed to the server if pushFlag is true.  Records
** are pulled if pullFlag is true.  A full sync occurs if both are
** true.
*/
void client_sync(int pushFlag, int pullFlag, int cloneFlag){
  int go = 1;        /* Loop until zero */
  const char *zSCode = db_get("server-code", "x");
  const char *zPCode = db_get("project-code", 0);
  int nMsg = 0;          /* Number of messages sent or received */
  int nCycle = 0;        /* Number of round trips to the server */
  int nFileSend = 0;
  int nFileRecv;          /* Number of files received */
  int mxPhantomReq = 200; /* Max number of phantoms to request per comm */
  const char *zCookie;    /* Server cookie */
  Blob send;        /* Text we are sending to the server */
  Blob recv;        /* Reply we got back from the server */
  Xfer xfer;        /* Transfer data */

  memset(&xfer, 0, sizeof(xfer));
  xfer.pIn = &recv;
  xfer.pOut = &send;
  xfer.mxSend = db_get_int("max-upload", 250000);

  assert( pushFlag || pullFlag || cloneFlag );
  assert( !g.urlIsFile );          /* This only works for networking */

  db_begin_transaction();
  db_multi_exec(
    "CREATE TEMP TABLE onremote(rid INTEGER PRIMARY KEY);"
  );
  blobarray_zero(xfer.aToken, count(xfer.aToken));
  blob_zero(&send);
  blob_zero(&recv);
  blob_zero(&xfer.err);
  blob_zero(&xfer.line);

  /*
  ** Always begin with a clone, pull, or push message
  */
  if( cloneFlag ){
    blob_appendf(&send, "clone\n");
    pushFlag = 0;
    pullFlag = 0;
    nMsg++;
  }else if( pullFlag ){
    blob_appendf(&send, "pull %s %s\n", zSCode, zPCode);
    nMsg++;
  }
  if( pushFlag ){
    blob_appendf(&send, "push %s %s\n", zSCode, zPCode);
    nMsg++;
  }


  while( go ){
    int newPhantom = 0;

    /* Send make the most recently received cookie.  Let the server
    ** figure out if this is a cookie that it cares about.
    */
    zCookie = db_get("cookie", 0);
    if( zCookie ){
      blob_appendf(&send, "cookie %s\n", zCookie);
    }
    
    /* Generate gimme messages for phantoms and leaf messages
    ** for all leaves.
    */
    if( pullFlag || cloneFlag ){
      request_phantoms(&xfer, mxPhantomReq);
    }
    if( pushFlag ){
      send_unsent(&xfer);
      nMsg += send_unclustered(&xfer);
    }

    /* Exchange messages with the server */
    nFileSend = xfer.nFileSent + xfer.nDeltaSent;
    printf("Sent:      %10d bytes, %4d messages, %4d files (%d+%d)\n",
            blob_size(&send), nMsg+xfer.nGimmeSent+xfer.nIGotSent,
            nFileSend, xfer.nFileSent, xfer.nDeltaSent);
    nMsg = 0;
    xfer.nFileSent = 0;
    xfer.nDeltaSent = 0;
    xfer.nGimmeSent = 0;
    fflush(stdout);
    http_exchange(&send, &recv);
    blob_reset(&send);

    /* Begin constructing the next message (which might never be
    ** sent) by beginning with the pull or push messages
    */
    if( pullFlag ){
      blob_appendf(&send, "pull %s %s\n", zSCode, zPCode);
      nMsg++;
    }
    if( pushFlag ){
      blob_appendf(&send, "push %s %s\n", zSCode, zPCode);
      nMsg++;
    }

    /* Process the reply that came back from the server */
    while( blob_line(&recv, &xfer.line) ){
      if( blob_buffer(&xfer.line)[0]=='#' ){
        continue;
      }
      xfer.nToken = blob_tokenize(&xfer.line, xfer.aToken, count(xfer.aToken));
      nMsg++;
      printf("\r%d", nMsg);
      fflush(stdout);

      /*   file UUID SIZE \n CONTENT
      **   file UUID DELTASRC SIZE \n CONTENT
      **
      ** Receive a file transmitted from the server.
      */
      if( blob_eq(&xfer.aToken[0],"file") ){
        xfer_accept_file(&xfer);
      }else

      /*   gimme UUID
      **
      ** Server is requesting a file.  If the file is a manifest, assume
      ** that the server will also want to know all of the content files
      ** associated with the manifest and send those too.
      */
      if( blob_eq(&xfer.aToken[0], "gimme")
       && xfer.nToken==2
       && blob_is_uuid(&xfer.aToken[1])
      ){
        if( pushFlag ){
          int rid = rid_from_uuid(&xfer.aToken[1], 0);
          send_file(&xfer, rid, &xfer.aToken[1], 0);
        }
      }else
  
      /*   igot UUID
      **
      ** Server announces that it has a particular file.  If this is
      ** not a file that we have and we are pulling, then create a
      ** phantom to cause this file to be requested on the next cycle.
      ** Always remember that the server has this file so that we do
      ** not transmit it by accident.
      */
      if( xfer.nToken==2
       && blob_eq(&xfer.aToken[0], "igot")
       && blob_is_uuid(&xfer.aToken[1])
      ){
        int rid = 0;
        if( pullFlag || cloneFlag ){
          if( !db_exists("SELECT 1 FROM blob WHERE uuid='%b' AND size>=0",
                &xfer.aToken[1]) ){
            rid = content_put(0, blob_str(&xfer.aToken[1]), 0);
            newPhantom = 1;
          }
        }
        if( rid==0 ){
          rid = rid_from_uuid(&xfer.aToken[1], 0);
        }
        remote_has(rid);
      }else
    
      
      /*   push  SERVERCODE  PRODUCTCODE
      **
      ** Should only happen in response to a clone.  This message tells
      ** the client what product to use for the new database.
      */
      if( blob_eq(&xfer.aToken[0],"push")
       && xfer.nToken==3
       && cloneFlag
       && blob_is_uuid(&xfer.aToken[1])
       && blob_is_uuid(&xfer.aToken[2])
      ){
        if( blob_eq_str(&xfer.aToken[1], zSCode, -1) ){
          fossil_fatal("server loop");
        }
        if( zPCode==0 ){
          zPCode = mprintf("%b", &xfer.aToken[2]);
          db_set("project-code", zPCode, 0);
        }
        blob_appendf(&send, "clone\n");
        nMsg++;
      }else
      
      /*    cookie TEXT
      **
      ** The server might include a cookie in its reply.  The client
      ** should remember this cookie and send it back to the server
      ** in its next query.
      **
      ** Each cookie received overwrites the prior cookie from the
      ** same server.
      */
      if( blob_eq(&xfer.aToken[0], "cookie") && xfer.nToken==2 ){
        db_set("cookie", blob_str(&xfer.aToken[1]), 0);
      }else

      /*   message MESSAGE
      **
      ** Print a message.  Similar to "error" but does not stop processing
      */        
      if( blob_eq(&xfer.aToken[0],"message") && xfer.nToken==2 ){
        char *zMsg = blob_terminate(&xfer.aToken[1]);
        defossilize(zMsg);
        printf("Server says: %s\n", zMsg);
      }else

      /*   error MESSAGE
      **
      ** Report an error and abandon the sync session
      */        
      if( blob_eq(&xfer.aToken[0],"error") && xfer.nToken==2 ){
        char *zMsg = blob_terminate(&xfer.aToken[1]);
        defossilize(zMsg);
        blob_appendf(&xfer.err, "server says: %s", zMsg);
      }else

      /* Unknown message */
      {
        blob_appendf(&xfer.err, "unknown command: %b", &xfer.aToken[0]);
      }

      if( blob_size(&xfer.err) ){
        fossil_fatal("%b", &xfer.err);
      }
      blobarray_reset(xfer.aToken, xfer.nToken);
      blob_reset(&xfer.line);
    }
    printf("\rReceived:  %10d bytes, %4d messages, %4d files (%d+%d+%d)\n",
            blob_size(&recv), nMsg,
            xfer.nFileRcvd + xfer.nDeltaRcvd + xfer.nDanglingFile,
            xfer.nFileRcvd, xfer.nDeltaRcvd, xfer.nDanglingFile);

    blob_reset(&recv);
    nCycle++;
    go = 0;

    /* If we received one or more files on the previous exchange but
    ** there are still phantoms, then go another round.
    */
    nFileRecv = xfer.nFileRcvd + xfer.nDeltaRcvd + xfer.nDanglingFile;
    if( (nFileRecv>0 || newPhantom) && db_exists("SELECT 1 FROM phantom") ){
      go = 1;
      mxPhantomReq = nFileRecv*2;
      if( mxPhantomReq<200 ) mxPhantomReq = 200;
    }
    nMsg = 0;
    xfer.nFileRcvd = 0;
    xfer.nDeltaRcvd = 0;
    xfer.nDanglingFile = 0;

    /* If we have one or more files queued to send, then go
    ** another round 
    */
    if( xfer.nFileSent+xfer.nDeltaSent>0 ){
      go = 1;
    }
  };
  http_close();
  db_multi_exec("DROP TABLE onremote");
  db_end_transaction(0);
}

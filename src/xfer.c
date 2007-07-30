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
static void xfer_accept_file(Blob *pIn, Blob *aToken, int nToken, Blob *pErr){
  int n;
  int rid;
  Blob content, hash;
  
  if( nToken<3 || nToken>4 || !blob_is_uuid(&aToken[1])
       || !blob_is_int(&aToken[nToken-1], &n) || n<=0
       || (nToken==4 && !blob_is_uuid(&aToken[2])) ){
    blob_appendf(pErr, "malformed file line");
    return;
  }
  blob_zero(&content);
  blob_zero(&hash);
  blob_extract(pIn, n, &content);
  if( nToken==4 ){
    Blob src;
    int srcid = db_int(0, "SELECT rid FROM blob WHERE uuid=%B", &aToken[2]);
    if( srcid==0 ){
      blob_appendf(pErr, "unknown delta source: %b", &aToken[2]);
      return;
    }
    content_get(srcid, &src);
    blob_delta_apply(&src, &content, &content);
    blob_reset(&src);
  }
  sha1sum_blob(&content, &hash);
  if( !blob_eq_str(&aToken[1], blob_str(&hash), -1) ){
    blob_appendf(pErr, "content does not match sha1 hash");
  }
  blob_reset(&hash);
  rid = content_put(&content, 0);
  manifest_crosslink(rid, &content);
  if( rid==0 ){
    blob_appendf(pErr, "%s", g.zErrMsg);
  }
}

/*
** Send the file identified by rid.
**
** If pOut is not NULL, then append the text of the send message
** to pOut.  Otherwise, append the text to the CGI output.
*/
static int send_file(int rid, Blob *pOut){
  Blob content, uuid;
  int size;

#if 0
SELECT srcid FROM delta
 WHERE rid=%d
   AND EXISTS(SELECT 1 FROM onremote WHERE rid=srcid)
UNION ALL
SELECT id FROM delta
 WHERE srcid=%d
   AND EXISTS(SELECT 1 FROM onremote WHERE rid=delta.rid)
LIMIT 1
#endif

  /* TODO:
  ** Check for related files in the onremote TEMP table.  If related
  ** files are found, then send a delta rather than the whole file.
  */

  blob_zero(&uuid);
  db_blob(&uuid, "SELECT uuid FROM blob WHERE rid=%d AND size>=0", rid);
  if( blob_size(&uuid)==0 ){
    return 0;
  }
  content_get(rid, &content);
  size = blob_size(&content);
  if( pOut ){
    blob_appendf(pOut, "file %b %d\n", &uuid, size);
    blob_append(pOut, blob_buffer(&content), size);
  }else{
    cgi_printf("file %b %d\n", &uuid, size);
    cgi_append_content(blob_buffer(&content), size);
  }
  blob_reset(&content);
  blob_reset(&uuid);
  db_multi_exec("INSERT OR IGNORE INTO onremote VALUES(%d)", rid);
  return size;
}


/*
** Send all pending files.
*/
static int send_all_pending(Blob *pOut){
  int sent = 0;
  int nSent = 0;
  int maxSize = db_get_int("http-msg-size", 1000000);
  Stmt q;
#if 0
  db_multi_exec(
    "CREATE TEMP TABLE priority(rid INTEGER PRIMARY KEY);"
    "INSERT INTO priority"
    " SELECT srcid FROM delta"
    "  WHERE EXISTS(SELECT 1 FROM onremote WHERE onremote.rid=delta.rid)"
    "    AND EXISTS(SELECT 1 FROM pending WHERE delta.srcid=pending.rid);"
    "INSERT OR IGNORE INTO priority"
    " SELECT rid FROM delta"
    "  WHERE EXISTS(SELECT 1 FROM onremote WHERE onremote.rid=delta.srcid)"
    "    AND EXISTS(SELECT 1 FROM pending WHERE delta.rid=pending.rid);"
  );
  while( sent<maxSize && (rid = db_int(0, "SELECT rid FROM priority"))!=0 ){
    sent += send_file(rid, pOut);
    db_multi_exec(
      "INSERT OR IGNORE INTO priority"
      " SELECT srcid FROM delta WHERE rid=%d"
      " UNION ALL"
      " SELECT rid FROM delta WHERE srcid=%d",
      rid, rid
    );
  }
#endif
  db_prepare(&q, "SELECT rid FROM pending");
  while( db_step(&q)==SQLITE_ROW ){
    int rid = db_column_int(&q, 0);
    if( sent<maxSize ){
      sent += send_file(rid, pOut);
      nSent++;
    }else{
      char *zUuid = db_text(0,
                      "SELECT uuid FROM blob WHERE rid=%d AND size>=0", rid);
      if( zUuid ){
        if( pOut ){
          blob_appendf(pOut, "igot %s\n", zUuid);
        }else{
          cgi_printf("igot %s\n", zUuid);
        }
        free(zUuid);
      }
    }
  }
  db_finalize(&q);
  
#if 0
  db_multi_exec("DROP TABLE priority");
#endif
  return nSent;
}


/*
** Check the signature on an application/x-fossil payload received by
** the HTTP server.  The signature is a line of the following form:
**
**        login LOGIN NONCE SIGNATURE
**
** The NONCE is a random string.  The server will never accept a
** repeat NONCE.  SIGNATURE is the SHA1 checksum of the NONCE 
** concatenated with the users password.
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

  if( db_exists("SELECT 1 FROM rcvfrom WHERE nonce=%B", pNonce) ){
    return;  /* Never accept a repeated nonce */
  }
  db_prepare(&q, "SELECT pw, cap, uid FROM user WHERE login=%B", pLogin);
  if( db_step(&q)==SQLITE_ROW ){
    Blob pw, combined, hash;
    blob_zero(&pw);
    db_ephemeral_blob(&q, 0, &pw);
    blob_zero(&combined);
    blob_copy(&combined, pNonce);
    blob_append(&combined, blob_buffer(&pw), blob_size(&pw));
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
  int nToken;
  int isPull = 0;
  int isPush = 0;
  int nErr = 0;
  Blob line, errmsg, aToken[5];

  db_begin_transaction();
  blobarray_zero(aToken, count(aToken));
  cgi_set_content_type(g.zContentType);
  blob_zero(&errmsg);
  db_multi_exec(
     "CREATE TEMP TABLE onremote(rid INTEGER PRIMARY KEY);" /* Client has */
     "CREATE TEMP TABLE pending(rid INTEGER PRIMARY KEY);"  /* Client needs */
  );
  while( blob_line(&g.cgiIn, &line) ){
    nToken = blob_tokenize(&line, aToken, count(aToken));

    /*   file UUID SIZE \n CONTENT
    **   file UUID DELTASRC SIZE \n CONTENT
    **
    ** Accept a file from the client.
    */
    if( blob_eq(&aToken[0], "file") && nToken>=2 && nToken<=3 ){
      if( !isPush ){
        cgi_reset_content();
        @ error not\sauthorized\sto\swrite
        nErr++;
        break;
      }
      xfer_accept_file(&g.cgiIn, aToken, nToken, &errmsg);
      if( blob_size(&errmsg) ){
        cgi_reset_content();
        @ error %T(blob_str(&errmsg))
        nErr++;
        break;
      }
    }else

    /*   gimme UUID
    **
    ** Client is requesting a file
    */
    if( blob_eq(&aToken[0], "gimme") && nToken==2 && blob_is_uuid(&aToken[1]) ){
      if( isPull ){
        db_multi_exec(
          "INSERT OR IGNORE INTO pending(rid) "
          "SELECT rid FROM blob WHERE uuid=%B AND size>=0", &aToken[1]
        );
      }
    }else

    /*   igot UUID
    **   leaf UUID
    **
    ** Client announces that it has a particular file
    */
    if( nToken==2
          && (blob_eq(&aToken[0], "igot") || blob_eq(&aToken[0],"leaf"))
          && blob_is_uuid(&aToken[1]) ){
      if( isPull || isPush ){
        int rid = db_int(0, "SELECT rid FROM blob WHERE uuid=%B", &aToken[1]);
        if( rid>0 ){
          db_multi_exec(
            "INSERT OR IGNORE INTO onremote(rid) VALUES(%d)", rid
          );
          if( isPull && blob_eq(&aToken[0], "leaf") ){
            db_multi_exec(
              "INSERT OR IGNORE INTO pending(rid) "
              "SELECT cid FROM plink WHERE pid=%d", rid
            );
          }
        }else if( isPush ){
          content_put(0, blob_str(&aToken[1]));
        }
      }
    }else

    /*    pull  SERVERCODE  PROJECTCODE
    **    push  SERVERCODE  PROJECTCODE
    **
    ** The client wants either send or receive
    */
    if( nToken==3
               && (blob_eq(&aToken[0], "pull") || blob_eq(&aToken[0], "push"))
               && blob_is_uuid(&aToken[1]) && blob_is_uuid(&aToken[2]) ){
      const char *zSCode;
      const char *zPCode;

      zSCode = db_get("server-code", 0);
      if( zSCode==0 ){
        fossil_panic("missing server code");
      }
      if( blob_eq_str(&aToken[1], zSCode, -1) ){
        cgi_reset_content();
        @ error server\sloop
        nErr++;
        break;
      }
      zPCode = db_get("project-code", 0);
      if( zPCode==0 ){
        fossil_panic("missing project code");
      }
      if( !blob_eq_str(&aToken[2], zPCode, -1) ){
        cgi_reset_content();
        @ error wrong\sproject
        nErr++;
        break;
      }
      login_check_credentials();
      if( blob_eq(&aToken[0], "pull") ){
        if( !g.okRead ){
          cgi_reset_content();
          @ error not\sauthorized\sto\sread
          nErr++;
          break;
        }
        isPull = 1;
      }else{
        if( !g.okWrite ){
          cgi_reset_content();
          @ error not\sauthorized\sto\swrite
          nErr++;
          break;
        }
        isPush = 1;

      }
    }else

    /*    clone
    **
    ** The client knows nothing.  Tell all.
    */
    if( blob_eq(&aToken[0], "clone") ){
      login_check_credentials();
      if( !g.okRead ){
        cgi_reset_content();
        @ error not\sauthorized\sto\sread
        nErr++;
        break;
      }
      isPull = 1;
      @ push %s(db_get("server-code", "x")) %s(db_get("project-code", "x"))
      db_multi_exec(
        "INSERT OR IGNORE INTO pending(rid) "
        "SELECT rid FROM blob WHERE size>=0"
      );
    }else

    /*    login  USER  NONCE  SIGNATURE
    **
    ** Check for a valid login.  This has to happen before anything else.
    */
    if( blob_eq(&aToken[0], "login") && nToken==4 ){
      if( disableLogin ){
        g.okRead = g.okWrite = 1;
      }else{
        check_login(&aToken[1], &aToken[2], &aToken[3]);
      }
    }else

    /* Unknown message
    */
    {
      cgi_reset_content();
      @ error bad\scommand:\s%F(blob_str(&line))
    }
    blobarray_reset(aToken, nToken);
  }

  /* The input message has now been processed.  Generate a reply. */
  if( isPush ){
    Stmt q;
    db_prepare(&q, "SELECT uuid FROM blob WHERE size<0");
    while( db_step(&q)==SQLITE_ROW ){
      const char *zUuid = db_column_text(&q, 0);
      @ gimme %s(zUuid)
    }
    db_finalize(&q);
  }
  if( isPull ){
    send_all_pending(0);
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
  int nToken;
  const char *zSCode = db_get("server-code", "x");
  const char *zPCode = db_get("project-code", 0);
  int nFile = 0;
  int nMsg = 0;
  int nReq = 0;
  Blob send;        /* Text we are sending to the server */
  Blob recv;        /* Reply we got back from the server */
  Blob line;        /* A single line of the reply */
  Blob aToken[5];   /* A tokenization of line */
  Blob errmsg;      /* Error message */

  assert( pushFlag || pullFlag || cloneFlag );
  assert( !g.urlIsFile );          /* This only works for networking */

  db_begin_transaction();
  db_multi_exec(
    /* Records which we know the other side also has */
    "CREATE TEMP TABLE onremote(rid INTEGER PRIMARY KEY);"
    /* Records we know the other side needs */
    "CREATE TEMP TABLE pending(rid INTEGER PRIMARY KEY);"
  );
  blobarray_zero(aToken, count(aToken));
  blob_zero(&send);
  blob_zero(&recv);
  blob_zero(&errmsg);


  while( go ){
    go = 0;
    nFile = nReq = nMsg = 0;

    /* Generate a request to be sent to the server.
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

    if( pullFlag ){
      /* Send gimme message for every phantom that we hold.
      */
      Stmt q;
      db_prepare(&q, "SELECT uuid FROM blob WHERE size<0");
      while( db_step(&q)==SQLITE_ROW ){
        const char *zUuid = db_column_text(&q, 0);
        blob_appendf(&send,"gimme %s\n", zUuid);
        nReq++;
      }
      db_finalize(&q);
    }

    if( pushFlag ){
      /* Send the server any files that the server has requested */
      nFile += send_all_pending(&send);
    }

    if( pullFlag || pushFlag ){
      /* Always send our leaves */
      Stmt q;
      db_prepare(&q, 
         "SELECT uuid FROM blob WHERE rid IN"
         "  (SELECT cid FROM plink EXCEPT SELECT pid FROM plink)"
      );
      while( db_step(&q)==SQLITE_ROW ){
        const char *zUuid = db_column_text(&q, 0);
        blob_appendf(&send, "leaf %s\n", zUuid);
        nMsg++;
      }
      db_finalize(&q);
    }

    /* Exchange messages with the server */
    printf("Send:      %d files, %d requests, %d other messages\n",
            nFile, nReq, nMsg);
    nFile = nReq = nMsg = 0;
    http_exchange(&send, &recv);
    blob_reset(&send);

    /* Process the reply that came back from the server */
    while( blob_line(&recv, &line) ){
      nToken = blob_tokenize(&line, aToken, count(aToken));

      /*   file UUID SIZE \n CONTENT
      **   file UUID DELTASRC SIZE \n CONTENT
      **
      ** Receive a file transmitted from the other side
      */
      if( blob_eq(&aToken[0],"file") ){
        xfer_accept_file(&recv, aToken, nToken, &errmsg);
        nFile++;
        go = 1;
      }else

      /*   gimme UUID
      **
      ** Server is requesting a file
      */
      if( blob_eq(&aToken[0], "gimme") && nToken==2
               && blob_is_uuid(&aToken[1]) ){
        nReq++;
        if( pushFlag ){
          db_multi_exec(
            "INSERT OR IGNORE INTO pending(rid) "
            "SELECT rid FROM blob WHERE uuid=%B AND size>=0", &aToken[1]
          );
          go = 1;
        }
      }else
  
      /*   igot UUID
      **   leaf UUID
      **
      ** Server proclaims that it has a particular file.  A leaf message
      ** means that the file is a leaf manifest on the server.
      */
      if( nToken==2
              && (blob_eq(&aToken[0], "igot") || blob_eq(&aToken[0], "leaf"))
              && blob_is_uuid(&aToken[1]) ){
        int rid = db_int(0, "SELECT rid FROM blob WHERE uuid=%B", &aToken[1]);
        nMsg++;
        if( rid>0 ){
          db_multi_exec(
            "INSERT OR IGNORE INTO onremote(rid) VALUES(%d)", rid
          );
          /* Add to the pending set all children of the server's leaves */
          if( pushFlag && blob_eq(&aToken[0], "leaf") ){
            db_multi_exec(
              "INSERT OR IGNORE INTO pending(rid) "
              "SELECT cid FROM plink WHERE pid=%d", rid
            );
            go = 1;
          }
          if( pullFlag && !go && 
              db_exists("SELECT 1 FROM blob WHERE rid=%d AND size>=0", rid) ){
            go = 1;
          }
        }else if( pullFlag ){
          go = 1;
          content_put(0, blob_str(&aToken[1]));
        }
      }else
  
      /*   push  SERVERCODE  PRODUCTCODE
      **
      ** Should only happen in response to a clone.
      */
      if( blob_eq(&aToken[0],"push") && nToken==3 && cloneFlag
              && blob_is_uuid(&aToken[1]) && blob_is_uuid(&aToken[2]) ){

        if( blob_eq_str(&aToken[1], zSCode, -1) ){
          fossil_fatal("server loop");
        }
        nMsg++;
        if( zPCode==0 ){
          zPCode = mprintf("%b", &aToken[2]);
          db_set("project-code", zPCode);
        }
        cloneFlag = 0;
        pullFlag = 1;
      }else

      /*   error MESSAGE
      **
      ** Report an error
      */        
      if( blob_eq(&aToken[0],"error") && nToken==2 ){
        char *zMsg = blob_terminate(&aToken[1]);
        defossilize(zMsg);
        blob_appendf(&errmsg, "server says: %s", zMsg);
      }else

      /* Unknown message */
      {
        blob_appendf(&errmsg, "unknown command: %b", &aToken[0]);
      }

      if( blob_size(&errmsg) ){
        fossil_fatal("%b", &errmsg);
      }
      blobarray_reset(aToken, nToken);
    }
    blob_reset(&recv);
    printf("Received:  %d files, %d requests, %d other messages\n",
            nFile, nReq, nMsg);
    nFile = nReq = nMsg = 0;
  };
  http_close();
  db_end_transaction(0);
  db_multi_exec(
    "DROP TABLE onremote;"
    "DROP TABLE pending;"
  );
}

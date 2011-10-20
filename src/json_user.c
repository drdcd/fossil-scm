/*
** Copyright (c) 2011 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*/
#include "VERSION.h"
#include "config.h"
#include "json_user.h"

#if INTERFACE
#include "json_detail.h"
#endif

static cson_value * json_user_get();
static cson_value * json_user_list();
static cson_value * json_user_save();
#if 0
static cson_value * json_user_create();

#endif

/*
** Mapping of /json/user/XXX commands/paths to callbacks.
*/
static const JsonPageDef JsonPageDefs_User[] = {
{"create", json_page_nyi, 1},
{"save", json_user_save, 1},
{"get", json_user_get, 0},
{"list", json_user_list, 0},
/* Last entry MUST have a NULL name. */
{NULL,NULL,0}
};


/*
** Implements the /json/user family of pages/commands.
**
*/
cson_value * json_page_user(){
  return json_page_dispatch_helper(&JsonPageDefs_User[0]);
}


/*
** Impl of /json/user/list. Requires admin rights.
*/
static cson_value * json_user_list(){
  cson_value * payV = NULL;
  Stmt q;
  if(!g.perm.Admin){
    g.json.resultCode = FSL_JSON_E_DENIED;
    return NULL;
  }
  db_prepare(&q,"SELECT uid AS uid,"
             " login AS name,"
             " cap AS capabilities,"
             " info AS info,"
             " mtime AS mtime"
             " FROM user ORDER BY login");
  payV = json_stmt_to_array_of_obj(&q, NULL);
  db_finalize(&q);
  if(NULL == payV){
    json_set_err(FSL_JSON_E_UNKNOWN,
                 "Could not convert user list to JSON.");
  }
  return payV;  
}

/*
** Impl of /json/user/get. Requires admin rights.
*/
static cson_value * json_user_get(){
  cson_value * payV = NULL;
  char const * pUser = NULL;
  Stmt q;
  if(!g.perm.Admin){
    json_set_err(FSL_JSON_E_DENIED,
                 "Requires 'a' privileges.");
    return NULL;
  }
  pUser = json_command_arg(g.json.dispatchDepth+1);
  if( g.isHTTP && (!pUser || !*pUser) ){
    pUser = json_getenv_cstr("name")
      /* ACHTUNG: fossil apparently internally sets name=user/get/XYZ
         if we pass the name as part of the path, which is why we check
         with json_command_path() before trying to get("name").
      */;
  }
  if(!pUser || !*pUser){
    json_set_err(FSL_JSON_E_MISSING_ARGS,"Missing 'name' property.");
    return NULL;
  }
  db_prepare(&q,"SELECT uid AS uid,"
             " login AS name,"
             " cap AS capabilities,"
             " info AS info,"
             " mtime AS mtime"
             " FROM user"
             " WHERE login=%Q",
             pUser);
  if( (SQLITE_ROW == db_step(&q)) ){
    payV = cson_sqlite3_row_to_object(q.pStmt);
    if(!payV){
      json_set_err(FSL_JSON_E_UNKNOWN,"Could not convert user row to JSON.");
    }
  }else{
    json_set_err(FSL_JSON_E_RESOURCE_NOT_FOUND,"User not found.");
  }
  db_finalize(&q);
  return payV;  
}

/*
** Don't use - not yet finished.
*/
int json_user_update_from_json( cson_object const * pUser ){
#define CSTR(X) cson_string_cstr(cson_value_get_string( cson_object_get(pUser, X ) ))
  char const * zName = CSTR("name");
  char * zNameFree = NULL;
  char const * zInfo = CSTR("info");
  char const * zCap = CSTR("capabilities");
  char const * zPW = CSTR("password");
  int gotFields = 0;
#undef CSTR
  cson_int_t const uid = cson_value_get_integer( cson_object_get(pUser, "uid") );
  Blob sql = empty_blob;
  Stmt q = empty_Stmt;
  
  if(uid<=0 && (!zName||!*zName)){
    return json_set_err(FSL_JSON_E_MISSING_ARGS,
                        "One of 'uid' or 'name' is required.");
  }else if(uid>0){
    zNameFree = db_text(NULL, "SELECT login FROM user WHERE uid=%d",uid);
    if(!zNameFree){
      return json_set_err(FSL_JSON_E_RESOURCE_NOT_FOUND,
                          "No login found for uid %d.", uid);
    }
    zName = zNameFree;
  }
  
  /*
    TODO: do not allow an admin user to modify a setup user
    unless the admin is also a setup user. setup.c uses
    that logic.
  */
  
  blob_append(&sql, "UPDATE USER SET ",-1 );
  if( zInfo ){
    blob_appendf(&sql, " info=%Q", zInfo);
    ++gotFields;
  }
  if( zCap ){
    blob_appendf(&sql, "%c cap=%Q", (gotFields ? ',' : ' '), zCap);
    ++gotFields;
  }
  if( zPW ){
    assert( zName != NULL);
    blob_appendf(&sql, "%c password=coalesce(shared_secret(%Q,%Q,"
                 "(SELECT value FROM config WHERE name='project-code')),pw),",
                 (gotFields ? ',' : ' '), zPW, zName);
    ++gotFields;
  }

  if(!gotFields){
    free( zNameFree );
    blob_reset(&sql);
    return FSL_JSON_E_MISSING_ARGS;
  }
  blob_append(&sql, " WHERE", -1);
  if(uid>0){
    blob_appendf(&sql, " uid=%d", uid);
  }else{
    blob_appendf(&sql, " login=%Q", zName);
  }
  free( zNameFree );

  puts(blob_str(&sql));
  blob_reset(&sql);
  assert(0 && "This is going to require extra work for login groups.");
  return 0;
}


/*
** Don't use - not yet finished.
*/
static cson_value * json_user_save(){
  if( !g.perm.Admin || !g.perm.Setup ){
    json_set_err(FSL_JSON_E_DENIED,
                 "Requires 'a' or 's' privileges.");
  }
  if(! g.json.reqPayload.o ){
    json_set_err(FSL_JSON_E_MISSING_ARGS,
                 "User data must be contained in the request payload.");
    return NULL;

  }
  json_user_update_from_json( g.json.reqPayload.o );
  return NULL;
}

/*
** Copyright (c) 2007 D. Richard Hipp
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
** Code for the JSON API.
**
** For notes regarding the public JSON interface, please see:
**
** https://docs.google.com/document/d/1fXViveNhDbiXgCuE7QDXQOKeFzf2qNUkBEgiUvoqFN4/edit
**
**
*/
#include "config.h"
#include "VERSION.h"
#include "json.h"
#include <assert.h>
#include <time.h>

#if INTERFACE
#include "cson_amalgamation.h"
#include "json_detail.h" /* workaround for apparent enum limitation in makeheaders */
#endif

/*
** Holds keys used for various JSON API properties.
*/
static const struct FossilJsonKeys_{
  char const * authToken;
  char const * commandPath;
} FossilJsonKeys = {
  "authToken"  /*authToken*/,
  "COMMAND_PATH" /*commandPath*/
};

/*
** Given a FossilJsonCodes value, it returns a string suitable for use
** as a resultText string. Returns some unspecified string if errCode
** is not one of the FossilJsonCodes values.
*/
char const * json_err_str( int errCode ){
  switch( errCode ){
    case 0: return "Success";
#define C(X,V) case FSL_JSON_E_ ## X: return V

    C(GENERIC,"Generic error");
    C(INVALID_REQUEST,"Invalid request");
    C(UNKNOWN_COMMAND,"Unknown Command");
    C(UNKNOWN,"Unknown error");
    C(RESOURCE_NOT_FOUND,"Resource not found");
    C(TIMEOUT,"Timeout reached");
    C(ASSERT,"Assertion failed");
    C(ALLOC,"Resource allocation failed");
    C(NYI,"Not yet implemented.");
    C(AUTH,"Authentication error");
    C(LOGIN_FAILED,"Login failed");
    C(LOGIN_FAILED_NONAME,"Login failed - name not supplied");
    C(LOGIN_FAILED_NOPW,"Login failed - password not supplied");
    C(LOGIN_FAILED_NOTFOUND,"Login failed - no match found");
    C(MISSING_AUTH,"Authentication info missing from request");
    C(DENIED,"Access denied");
    C(WRONG_MODE,"Request not allowed (wrong operation mode)");

    C(USAGE,"Usage error");
    C(INVALID_ARGS,"Invalid arguments");
    C(MISSING_ARGS,"Missing arguments");

    C(DB,"Database error");
    C(STMT_PREP,"Statement preparation failed");
    C(STMT_BIND,"Statement parameter binding failed");
    C(STMT_EXEC,"Statement execution/stepping failed");
    C(DB_LOCKED,"Database is locked");
#undef C
    default:
      return "Unknown Error";
  }
}
/*
** Implements the cson_data_dest_f() interface and outputs the data to
** a fossil Blob object.  pState must be-a initialized (Blob*), to
** which n bytes of src will be appended.
**/
int cson_data_dest_Blob(void * pState, void const * src, unsigned int n){
  Blob * b = (Blob*)pState;
  blob_append( b, (char const *)src, (int)n ) /* will die on OOM */;
  return 0;
}

/*
** Implements the cson_data_source_f() interface and reads
** input from a fossil Blob object. pState must be-a (Blob*).
*/
int cson_data_src_Blob(void * pState, void * dest, unsigned int * n){
  Blob * b = (Blob*)pState;
  *n = blob_read( b, dest, *n );
  return 0;
}

/*
** Convenience wrapper around cson_output() which appends the output
** to pDest. pOpt may be NULL, in which case g.json.outOpt will be used.
*/
int cson_output_Blob( cson_value const * pVal, Blob * pDest, cson_output_opt const * pOpt ){
  return cson_output( pVal, cson_data_dest_Blob,
                      pDest, pOpt ? pOpt : &g.json.outOpt );
}

/*
** Convenience wrapper around cson_parse() which reads its input
** from pSrc. pSrc is rewound before parsing.
**
** pInfo may be NULL. If it is not NULL then it will contain details
** about the parse state when this function returns.
**
** On success a new JSON Object or Array is returned. On error NULL is
** returned.
*/
cson_value * cson_parse_Blob( Blob * pSrc, cson_parse_info * pInfo ){
  cson_value * root = NULL;
  blob_rewind( pSrc );
  cson_parse( &root, cson_data_src_Blob, pSrc, NULL, pInfo );
  return root;
}

/*
** Returns a string in the form FOSSIL-XXXX, where XXXX is a
** left-zero-padded value of code. The returned buffer is static, and
** must be copied if needed for later.  The returned value will always
** be 11 bytes long (not including the trailing NUL byte).
**
** In practice we will only ever call this one time per app execution
** when constructing the JSON response envelope, so the static buffer
** "shouldn't" be a problem.
**
*/
char const * json_rc_cstr( int code ){
  enum { BufSize = 12 };
  static char buf[BufSize] = {'F','O','S','S','I','L','-',0};
  assert((code >= 1000) && (code <= 9999) && "Invalid Fossil/JSON code.");
  sprintf(buf+7,"%04d", code);
  return buf;
}

/*
** Adds v to the API-internal cleanup mechanism. key must be a unique
** key for the given element. Adding another item with that key may
** free the previous one. If freeOnError is true then v is passed to
** cson_value_free() if the key cannot be inserted, otherweise
** ownership of v is not changed on error.
**
** Returns 0 on success.
**
*** On success, ownership of v is transfered to (or shared with)
*** g.json.gc, and v will be valid until that object is cleaned up or
*** its key is replaced via another call to this function.
*/
int json_gc_add( char const * key, cson_value * v, char freeOnError ){
  int const rc = cson_object_set( g.json.gc.o, key, v );
  assert( NULL != g.json.gc.o );
  if( (0 != rc) && freeOnError ){
    cson_value_free( v );
  }
  return rc;
}


/*
** Returns the value of json_rc_cstr(code) as a new JSON
** string, which is owned by the caller and must eventually
** be cson_value_free()d or transfered to a JSON container.
*/
cson_value * json_rc_string( int code ){
  return cson_value_new_string( json_rc_cstr(code), 11 );
}


/*
** Gets a POST/GET/COOKIE value. The returned memory is owned by the
** g.json object (one of its sub-objects). Returns NULL if no match is
** found.
**
** Precedence: GET, COOKIE, POST. COOKIE _should_ be last
** but currently is not for internal order-of-init reasons.
** Since fossil only uses one cookie, this is not a high-prio
** problem.
*/
cson_value * json_getenv( char const * zKey ){
  cson_value * rc;
  rc = cson_object_get( g.json.param.o, zKey );
  if( rc ){
    return rc;
  }else{
    rc = cson_object_get( g.json.post.o, zKey );
    if(rc){
      return rc;
    }else{
      char const * cv = PD(zKey,NULL);
      if(cv){/*transform it to JSON for later use.*/
        rc = cson_value_new_string(cv,strlen(cv));
        cson_object_set( g.json.param.o, zKey, rc );
        return rc;
      }
    }
  }
  return NULL;
}

/*
** Adds v to g.json.param.o using the given key. May cause
** any prior item with that key to be destroyed (depends on
** current reference count for that value).
** On succes, transfers ownership of v to g.json.param.o.
** On error ownership of v is not modified.
*/
int json_setenv( char const * zKey, cson_value * v ){
  return cson_object_set( g.json.param.o, zKey, v );
}

/*
** Guesses a RESPONSE Content-Type value based (primarily) on the
** HTTP_ACCEPT header.
**
** It will try to figure out if the client can support
** application/json or application/javascript, and will fall back to
** text/plain if it cannot figure out anything more specific.
**
** Returned memory is static and immutable.
**
*/
char const * json_guess_content_type(){
  char const * cset;
  char doUtf8;
  cset = PD("HTTP_ACCEPT_CHARSET",NULL);
  doUtf8 = ((NULL == cset) || (NULL!=strstr("utf-8",cset)))
    ? 1 : 0;
  if( g.json.jsonp ){
    return doUtf8
      ? "application/javascript; charset=utf-8"
      : "application/javascript";
  }else{
    /*
      Content-type
      
      If the browser does not sent an ACCEPT for application/json
      then we fall back to text/plain.
    */
    char const * cstr;
    cstr = PD("HTTP_ACCEPT",NULL);
    if( NULL == cstr ){
      return doUtf8
        ? "application/json; charset=utf-8"
        : "application/json";
    }else{
      if( strstr( cstr, "application/json" )
          || strstr( cstr, "*/*" ) ){
        return doUtf8
          ? "application/json; charset=utf-8"
          : "application/json";
      }else{
        return "text/plain";
      }
    }
  }
}

/*
** Returns the current request's JSON authentication token, or NULL if
** none is found. The token's memory is owned by (or shared with)
** g.json.
**
** If an auth token is found in the GET/POST JSON request data then
** fossil is given that data for use in authentication for this
** session.
**
** Must be called once before login_check_credentials() is called or
** we will not be able to replace fossil's internal idea of the auth
** info in time (and future changes to that state may cause unexpected
** results).
**
** The result of this call are cached for future calls.
*/
static cson_value * json_auth_token(){
  if( !g.json.authToken ){
    /* Try to get an authorization token from GET parameter, POSTed
       JSON, or fossil cookie (in that order). */
    g.json.authToken = json_getenv(FossilJsonKeys.authToken);
    if(g.json.authToken
       && cson_value_is_string(g.json.authToken)
       && !PD(login_cookie_name(),NULL)){
      /* tell fossil to use this login info.

      FIXME?: because the JSON bits don't carry around
      login_cookie_name(), there is a potential login hijacking
      window here. We may need to change the JSON auth token to be
      in the form: login_cookie_name()=...

      Then again, the hardened cookie value helps ensure that
      only a proper key/value match is valid.
      */
      cgi_replace_parameter( login_cookie_name(), cson_value_get_cstr(g.json.authToken) );
    }else if( g.isCGI ){
      /* try fossil's conventional cookie. */
      /* Reminder: chicken/egg scenario regarding db access in CLI
         mode because login_cookie_name() needs the db. CLI
         mode does not use any authentication, so we don't need
         to support it here.
      */
      char const * zCookie = P(login_cookie_name());
      if( zCookie && *zCookie ){
        /* Transfer fossil's cookie to JSON for downstream convenience... */
        cson_value * v = cson_value_new_string(zCookie, strlen(zCookie));
        json_setenv( FossilJsonKeys.authToken, v );
        g.json.authToken = v;
      }
    }
  }
  return g.json.authToken;
}

/*
** Initializes some JSON bits which need to be initialized relatively
** early on. It should only be called from cgi_init() or
** json_cmd_top() (early on in those functions).
**
** Initializes g.json.gc and g.json.param.
*/
void json_main_bootstrap(){
  cson_value * v;
  assert( (NULL == g.json.gc.v) && "cgi_json_bootstrap() was called twice!" );
  v = cson_value_new_object();
  g.json.gc.v = v;
  g.json.gc.o = cson_value_get_object(v);

  v = cson_value_new_object();
  g.json.param.v = v;
  g.json.param.o = cson_value_get_object(v);
  json_gc_add("$PARAMS", v, 1);
}


/*
** Performs some common initialization of JSON-related state.  Must be
** called by the json_page_top() and json_cmd_top() dispatching
** functions to set up the JSON stat used by the dispatched functions.
**
** Implicitly sets up the login information state in CGI mode, but
** does not perform any permissions checking. It _might_ (haven't
** tested this) die with an error if an auth cookie is malformed.
**
** This must be called by the top-level JSON command dispatching code
** before they do any work.
**
** This must only be called once, or an assertion may be triggered.
*/
static void json_mode_bootstrap(){
  static char once = 0  /* guard against multiple runs */;
  char const * zPath = P("PATH_INFO");
  cson_value * pathSplit = NULL;
  assert( (0==once) && "json_mode_bootstrap() called too many times!");
  if( once ){
    return;
  }else{
    once = 1;
  }
  g.json.isJsonMode = 1;
  g.json.resultCode = 0;
  g.json.cmd.offset = -1;
  if( !g.isCGI && g.fullHttpReply ){
    /* workaround for server mode, so we see it as CGI mode. */
    g.isCGI = 1;
  }
  if(! g.json.post.v ){
    /* If cgi_init() reads POSTed JSON then it sets the content type.
       If it did not then we need to set it.
    */
    cgi_set_content_type(json_guess_content_type());
  }

#if defined(NDEBUG)
  /* avoids debug messages on stderr in JSON mode */
  sqlite3_config(SQLITE_CONFIG_LOG, NULL, 0);
#endif

  g.json.cmd.v = cson_value_new_array();
  g.json.cmd.a = cson_value_get_array(g.json.cmd.v);
  json_gc_add( FossilJsonKeys.commandPath, g.json.cmd.v, 1 );
  /*
    The following if/else block translates the PATH_INFO path (in
    CLI/server modes) or g.argv (CLI mode) into an internal list so
    that we can simplify command dispatching later on.

    Note that translating g.argv this way is overkill but allows us to
    avoid CLI-only special-case handling in other code, e.g.
    json_command_arg().
  */
  if( zPath ){/* Either CGI or server mode... */
    /* Translate PATH_INFO into JSON for later convenience. */
    char const * p = zPath /* current byte */;
    char const * head = p  /* current start-of-token */;
    unsigned int len = 0   /* current token's lengh */;
    assert( g.isCGI && "g.isCGI should have been set by now." );
    for( ; ; ++p){
      if( !*p || ('/' == *p) ){
        if( len ){
          cson_value * part;
          char * zPart;
          assert( head != p );
          zPart = (char*)malloc(len+1);
          assert( zPart != NULL );
          memcpy(zPart, head, len);
          zPart[len] = 0;
          dehttpize(zPart);
          part = cson_value_new_string(zPart, strlen(zPart));
          free(zPart);
          cson_array_append( g.json.cmd.a, part );
          len = 0;
        }
        if( !*p ){
          break;
        }
        head = p+1;
        continue;
      }
      ++len;
    }
  }else{/* assume CLI mode */
    int i;
    char const * arg;
    cson_value * part;
    for(i = 1/*skip argv[0]*/; i < g.argc; ++i ){
      arg = g.argv[i];
      if( !arg || !*arg ){
        continue;
      }
      part = cson_value_new_string(arg,strlen(arg));
      cson_array_append(g.json.cmd.a, part);
    }
  }
  
  /* g.json.reqPayload exists only to simplify some of our access to
     the request payload. We currently only use this in the context of
     Object payloads, not Arrays, strings, etc. */
  g.json.reqPayload.v = cson_object_get( g.json.post.o, "payload" );
  if( g.json.reqPayload.v ){
    g.json.reqPayload.o = cson_value_get_object( g.json.reqPayload.v )
        /* g.json.reqPayload.o may legally be NULL, which means only that
           g.json.reqPayload.v is-not-a Object.
        */;
  }

  do{/* set up JSON out formatting options. */
    unsigned char indent = g.isCGI ? 0 : 1;
    cson_value const * indentV = json_getenv("indent");
    if(indentV){
      if(cson_value_is_string(indentV)){
        int n = atoi(cson_string_cstr(cson_value_get_string(indentV)));
        indent = (n>0)
          ? (unsigned char)n
          : 0;
      }else if(cson_value_is_number(indentV)){
        double n = cson_value_get_integer(indentV);
        indent = (n>0)
          ? (unsigned char)n
          : 0;
      }
    }
    g.json.outOpt.indentation = indent;
    g.json.outOpt.addNewline = g.isCGI ? 0 : 1;
  }while(0);

  json_auth_token()/* will copy our auth token, if any, to fossil's core. */;
  if( g.isCGI ){
    login_check_credentials()/* populates g.perm */;
  }
  else{
    db_find_and_open_repository(OPEN_ANY_SCHEMA,0);
  }
}

/*
** Returns the ndx'th item in the "command path", where index 0 is the
** position of the "json" part of the path. Returns NULL if ndx is out
** of bounds or there is no "json" path element.
**
** In CLI mode the "path" is the list of arguments (skipping argv[0]).
** In server/CGI modes the path is taken from PATH_INFO.
**
**
** Reminder to self: this breaks in CLI mode when called with an
** abbreviated name because we rely on the full name "json" here. The
** g object probably has the short form which we can use for our
** purposes, but i haven't yet looked for it.
*/
static char const * json_command_arg(unsigned char ndx){
  cson_array * ar = g.json.cmd.a;
  assert((NULL!=ar) && "Internal error. Was json_mode_bootstrap() called?");
  if( g.json.cmd.offset < 0 ){
    /* first-time setup. */
    short i = 0;
#define NEXT cson_string_cstr(          \
                 cson_value_get_string( \
                   cson_array_get(ar,i) \
                   ))
    char const * tok = NEXT;
    while( tok ){
      if( 0==strncmp("json",tok,4) ){
        g.json.cmd.offset = i;
        break;
      }
      ++i;
      tok = NEXT;
    }
  }
#undef NEXT
  if(g.json.cmd.offset < 0){
    return NULL;
  }else{
    ndx = g.json.cmd.offset + ndx;
    return cson_string_cstr(cson_value_get_string(cson_array_get( ar, g.json.cmd.offset + ndx )));
  }
}

/*
** If g.json.reqPayload.o is NULL then NULL is returned, else the
** given property is searched for in the request payload.  If found it
** is returned. The returned value is owned by (or shares ownership
** with) g.json, and must NOT be cson_value_free()'d by the
** caller.
*/
static cson_value * json_payload_property( char const * key ){
  return g.json.reqPayload.o ?
    cson_object_get( g.json.reqPayload.o, key )
    : NULL;
}


/* Returns the C-string form of json_auth_token(), or NULL
** if json_auth_token() returns NULL.
*/
char const * json_auth_token_cstr(){
  return cson_value_get_cstr( json_auth_token() );
}

/*
** Holds name-to-function mappings for JSON page/command dispatching.
**
*/
typedef struct JsonPageDef{
  /*
  ** The commmand/page's name (path, not including leading /json/).
  **
  ** Reminder to self: we cannot use sub-paths with commands this way
  ** without additional string-splitting downstream. e.g. foo/bar.
  ** Alternately, we can create different JsonPageDef arrays for each
  ** subset.
  */
  char const * name;
  /*
  ** Returns a payload object for the response.  If it returns a
  ** non-NULL value, the caller owns it.  To trigger an error this
  ** function should set g.json.resultCode to a value from the
  ** FossilJsonCodes enum.
  */
  cson_value * (*func)();
  /*
  ** Which mode(s) of execution does func() support:
  **
  ** <0 = CLI only, >0 = HTTP only, 0==both
  */
  char runMode;
} JsonPageDef;

/*
** Returns the JsonPageDef with the given name, or NULL if no match is
** found.
**
** head must be a pointer to an array of JsonPageDefs in which the
** last entry has a NULL name.
*/
static JsonPageDef const * json_handler_for_name( char const * name, JsonPageDef const * head ){
  JsonPageDef const * pageDef = head;
  assert( head != NULL );
  if(name && *name) for( ; pageDef->name; ++pageDef ){
    if( 0 == strcmp(name, pageDef->name) ){
      return pageDef;
    }
  }
  return NULL;
}

/*
** Given a Fossil/JSON result code, this function "dumbs it down"
** according to the current value of g.json.errorDetailParanoia. The
** dumbed-down value is returned.
**
** This function assert()s that code is either 0
** or between the range of 1000 and 9999.
*/
static int json_dumbdown_rc( int code ){
  if( !code ){
    return 0;
  }else{
    int modulo = 0;
    assert((code >= 1000) && (code <= 9999) && "Invalid Fossil/JSON code.");
    switch( g.json.errorDetailParanoia ){
      case 1: modulo = 10; break;
      case 2: modulo = 100; break;
      case 3: modulo = 1000; break;
      default: break;
    }
    if( modulo ) code = code - (code % modulo);
    return code;
  }
}

#if 0
static unsigned int json_timestamp(){

}
#endif


/*
** Creates a new Fossil/JSON response envelope skeleton.  It is owned
** by the caller, who must eventually free it using cson_value_free(),
** or add it to a cson container to transfer ownership. Returns NULL
** on error.
**
** If payload is not NULL and resultCode is 0 then it is set as the
** "payload" property of the returned object. If resultCode is non-0
** then this function will destroy payload if it is not NULL. i.e.
** onwership of payload is transfered to this function.
**
** pMsg is an optional message string (resultText) property of the
** response. If resultCode is non-0 and pMsg is NULL then
** json_err_str() is used to get the error string. The caller may
** provide his own or may use an empty string to suppress the
** resultText property.
**
** If resultCode is non-zero and payload is not NULL then this
** function calls cson_value_free(payload) and does not insert the
** payload into the response.
**
*/
cson_value * json_response_skeleton( int resultCode,
                                     cson_value * payload,
                                     char const * pMsg ){
  cson_value * v = NULL;
  cson_value * tmp = NULL;
  cson_object * o = NULL;
  int rc;
  resultCode = json_dumbdown_rc(resultCode);
  v = cson_value_new_object();
  o = cson_value_get_object(v);
  if( ! o ) return NULL;
#define SET(K) if(!tmp) goto cleanup; \
  rc = cson_object_set( o, K, tmp ); \
  if(rc) do{\
    cson_value_free(tmp); \
    tmp = NULL; \
    goto cleanup; \
  }while(0)

  tmp = cson_value_new_string(MANIFEST_UUID,strlen(MANIFEST_UUID));
  SET("fossil");
 
  {/* "timestamp" */
    cson_int_t jsTime;
#if 1
    jsTime = (cson_int_t)time(0);
#elif 1
    /* Ge Weijers has pointed out that time(0) commonly returns
       UTC, but is not required to by The Standard.

       There is a mkfmtime() function in cgi.c but it requires
       a (tm *), and i don't have that without calling gmtime()
       or populating the tm myself (which is what i'm trying to
       have done for me!).
    */
    time_t const t = (time_t)time(0);
    struct tm gt = *gmtime(&t);
    gt.tm_isdst = -1;
    jsTime = (cson_int_t)mktime(&gt);
#else
    /* i'm not 100% sure that the above actually does what i expect,
       but we can't use the following because this function can be
       called in response to error handling if the db cannot be opened
       (or before that).
    */
    jsTime = (cson_int_t)db_int64(0, "SELECT strftime('%%s','now')");
#endif
    tmp = cson_value_new_integer(jsTime);
    SET("timestamp");
  }
  if( 0 != resultCode ){
    if( ! pMsg ) pMsg = json_err_str(resultCode);
    tmp = json_rc_string(resultCode);
    SET("resultCode");
  }
  if( pMsg && *pMsg ){
    tmp = cson_value_new_string(pMsg,strlen(pMsg));
    SET("resultText");
  }
  tmp = json_getenv("requestId");
  if( tmp ) cson_object_set( o, "requestId", tmp );
  if( NULL != payload ){
    if( resultCode ){
      cson_value_free(payload);
      payload = NULL;
    }else{
      tmp = payload;
      SET("payload");
    }
  }
#undef SET

  if(0){/*Only for debuggering, add some info to the response.*/
    tmp = cson_value_new_integer( g.json.cmd.offset );
    cson_object_set( o, "cmd.offset", tmp );
    cson_object_set( o, "isCGI", cson_value_new_bool( g.isCGI ) );
  }

  goto ok;
  cleanup:
  cson_value_free(v);
  v = NULL;
  ok:
  return v;
}

/*
** Outputs a JSON error response to g.httpOut.  If rc is 0 then
** g.json.resultCode is used. If that is also 0 then the "Unknown
** Error" code is used.
**
** If g.isCGI then the generated error object replaces any currently
** buffered page output.
**
** If alsoOutput is true AND g.isCGI then the cgi_reply() is called to
** flush the output (and headers). Generally only do this if you are
** about to call exit().
**
** !g.isCGI then alsoOutput is ignored and all output is sent to
** stdout immediately.
**
** This clears any previously buffered CGI content, replacing it with
** JSON.
*/
void json_err( int code, char const * msg, char alsoOutput ){
  int rc = code ? code : (g.json.resultCode
                          ? g.json.resultCode
                          : FSL_JSON_E_UNKNOWN);
  cson_value * resp = NULL;
  rc = json_dumbdown_rc(rc);
  if( rc && !msg ){
    msg = json_err_str(rc);
  }
  resp = json_response_skeleton(rc, NULL, msg);
  if( g.isCGI ){
    Blob buf = empty_blob;
    cgi_reset_content();
    cson_output_Blob( resp, &buf, &g.json.outOpt );
    cgi_set_content(&buf);
    if( alsoOutput ){
      cgi_reply();
    }
  }else{
    cson_output_FILE( resp, stdout, &g.json.outOpt );
  }
  cson_value_free(resp);
}

/*
** /json/version implementation.
**
** Returns the payload object (owned by the caller).
*/
cson_value * json_page_version(void){
  cson_value * jval = NULL;
  cson_object * jobj = NULL;
  jval = cson_value_new_object();
  jobj = cson_value_get_object(jval);
#define FSET(X,K) cson_object_set( jobj, K, cson_value_new_string(X,strlen(X)))
  FSET(MANIFEST_UUID,"manifestUuid");
  FSET(MANIFEST_VERSION,"manifestVersion");
  FSET(MANIFEST_DATE,"manifestDate");
  FSET(MANIFEST_YEAR,"manifestYear");
  FSET(RELEASE_VERSION,"releaseVersion");
#undef FSET
  cson_object_set( jobj, "releaseVersionNumber",
                   cson_value_new_integer(RELEASE_VERSION_NUMBER) );
  cson_object_set( jobj, "resultCodeParanoiaLevel",
                   cson_value_new_integer(g.json.errorDetailParanoia) );
  return jval;
}

/*
** Returns the string form of a json_getenv() value, but ONLY
** If that value is-a String. Non-strings are not converted
** to strings for this purpose. Returned memory is owned by
** g.json or fossil..
*/
static char const * json_getenv_cstr( char const * zKey ){
  return cson_value_get_cstr( json_getenv(zKey) );
}


/*
** Implementation for /json/cap
**
** Returned object contains details about the "capabilities" of the
** current user (what he may/may not do).
**
** This is primarily intended for debuggering, but may have
** a use in client code. (?)
*/
cson_value * json_page_cap(void){
  cson_value * payload = cson_value_new_object();
  cson_value * sub = cson_value_new_object();
  char * zCap;
  Stmt q;
  cson_object * obj = cson_value_get_object(payload);
  if( g.zLogin ){
    cson_object_set( obj, "userName",
                     cson_value_new_string(g.zLogin,strlen(g.zLogin)) );
  }
  db_prepare(&q, "SELECT cap FROM user WHERE uid=%d", g.userUid);
  if( db_step(&q)==SQLITE_ROW ){
    char const * zCap = (char const *)sqlite3_column_text(q.pStmt,0);
    if( zCap ){
      cson_object_set( obj, "capabilities",
                       cson_value_new_string(zCap,strlen(zCap)) );
    }
  }
  db_finalize(&q);
  cson_object_set( obj, "permissionFlags", sub );
  obj = cson_value_get_object(sub);

#define ADD(X) cson_object_set(obj, #X, cson_value_new_bool(g.perm.X))
  ADD(Setup);
  ADD(Admin);
  ADD(Delete);
  ADD(Password);
  ADD(Query);
  ADD(Write);
  ADD(Read);
  ADD(History);
  ADD(Clone);
  ADD(RdWiki);
  ADD(NewWiki);
  ADD(ApndWiki);
  ADD(WrWiki);
  ADD(RdTkt);
  ADD(NewTkt);
  ADD(ApndTkt);
  ADD(WrTkt);
  ADD(Attach);
  ADD(TktFmt);
  ADD(RdAddr);
  ADD(Zip);
  ADD(Private);
#undef ADD
  return payload;
}

/*
** Implementation of the /json/login page.
**
** NOT YET FINSIHED!
** TODOs:
**
** - anonymous user login (requires separate handling
** due to random password).
**
** - more testing with ONLY the JSON-specified authToken
** (no cookie). In theory that works but we don't yet have
** a non-browser client to play with.
**
*/
cson_value * json_page_login(void){
  static char preciseErrors =
#if 0
    g.json.errorDetailParanoia ? 0 : 1
#else
    0
#endif
    ;
  /*
    FIXME: we want to check the GET/POST args in this order:

    - GET: name, n, password, p
    - POST: name, password

    but a bug in cgi_parameter() is breaking that, causing PD() to
    return the last element of the PATH_INFO instead.

    Summary: If we check for P("name") first, then P("n"),
    then ONLY a GET param of "name" will match ("n"
    is not recognized). If we reverse the order of the
    checks then both forms work. Strangely enough, the
    "p"/"password" check is not affected by this.
   */
  char const * name = cson_value_get_cstr(json_payload_property("name"));
  char const * pw = NULL;
  if( !name ){
    name = PD("n",NULL);
    if( !name ){
      name = PD("name",NULL);
      if( !name ){
        g.json.resultCode = preciseErrors
          ? FSL_JSON_E_LOGIN_FAILED_NONAME
          : FSL_JSON_E_LOGIN_FAILED;
        return NULL;
      }
    }
  }

  pw = cson_value_get_cstr(json_payload_property("password"));
  if( !pw ){
    pw = PD("p",NULL);
    if( !pw ){
      pw = PD("password",NULL);
    }
  }
  if(!pw){
    g.json.resultCode = preciseErrors
      ? FSL_JSON_E_LOGIN_FAILED_NOPW
      : FSL_JSON_E_LOGIN_FAILED;
    return NULL;
  }else{
    cson_value * payload = NULL;
    int uid = 0;
#if 0
    /* only for debugging the PD()-incorrect-result problem */
    cson_object * o = NULL;
    uid = login_search_uid( name, pw );
    payload = cson_value_new_object();
    o = cson_value_get_object(payload);
    cson_object_set( o, "n", cson_value_new_string(name,strlen(name)));
    cson_object_set( o, "p", cson_value_new_string(pw,strlen(pw)));
    return payload;
#else
    uid = login_search_uid( name, pw );
    if( !uid ){
      g.json.resultCode = preciseErrors
        ? FSL_JSON_E_LOGIN_FAILED_NOTFOUND
        : FSL_JSON_E_LOGIN_FAILED;
    }else{
      char * cookie = NULL;
      login_set_user_cookie(name, uid, &cookie);
      payload = cson_value_new_string( cookie, strlen(cookie) );
      free(cookie);
    }
    return payload;
#endif
  }
}

/*
** Impl of /json/logout.
**
*/
cson_value * json_page_logout(void){
  cson_value const *token = g.json.authToken;
    /* Remember that json_bootstrap() replaces the login cookie with
       the JSON auth token if the request contains it. If the reqest
       is missing the auth token then this will fetch fossil's
       original cookie. Either way, it's what we want :).

       We require the auth token to avoid someone maliciously
       trying to log someone else out (not 100% sure if that
       would be possible, given fossil's hardened cookie, but
       i'll assume it would be for the time being).
    */
    ;
  if(!token){
    g.json.resultCode = FSL_JSON_E_MISSING_AUTH;
  }else{
    login_clear_login_data();
    g.json.authToken = NULL /* memory is owned elsewhere.*/;
  }
  return NULL;
}

/*
** Implementation of the /json/stat page/command.
**
*/
cson_value * json_page_stat(void){
  i64 t, fsize;
  int n, m;
  const char *zDb;
  enum { BufLen = 1000 };
  char zBuf[BufLen];
  cson_value * jv = NULL;
  cson_object * jo = NULL;
  cson_value * jv2 = NULL;
  cson_object * jo2 = NULL;
  login_check_credentials();
  if( !g.perm.Read ){
    g.json.resultCode = FSL_JSON_E_DENIED;
    return NULL;
  }
#define SETBUF(O,K) cson_object_set(O, K, cson_value_new_string(zBuf, strlen(zBuf)));

  jv = cson_value_new_object();
  jo = cson_value_get_object(jv);

  sqlite3_snprintf(BufLen, zBuf, db_get("project-name",""));
  SETBUF(jo, "projectName");
  /* FIXME: don't include project-description until we ensure that
     zBuf will always be big enough. We "should" replace zBuf
     with a blob for this purpose.
  */
  fsize = file_size(g.zRepositoryName);
  cson_object_set(jo, "repositorySize", cson_value_new_integer((cson_int_t)fsize));

  n = db_int(0, "SELECT count(*) FROM blob");
  m = db_int(0, "SELECT count(*) FROM delta");
  cson_object_set(jo, "blobCount", cson_value_new_integer((cson_int_t)n));
  cson_object_set(jo, "deltaCount", cson_value_new_integer((cson_int_t)m));
  if( n>0 ){
    int a, b;
    Stmt q;
    db_prepare(&q, "SELECT total(size), avg(size), max(size)"
                   " FROM blob WHERE size>0");
    db_step(&q);
    t = db_column_int64(&q, 0);
    cson_object_set(jo, "uncompressedArtifactSize",
                    cson_value_new_integer((cson_int_t)t));
    cson_object_set(jo, "averageArtifactSize",
                    cson_value_new_integer((cson_int_t)db_column_int(&q, 1)));
    cson_object_set(jo, "maxArtifactSize",
                    cson_value_new_integer((cson_int_t)db_column_int(&q, 2)));
    db_finalize(&q);
    if( t/fsize < 5 ){
      b = 10;
      fsize /= 10;
    }else{
      b = 1;
    }
    a = t/fsize;
    sqlite3_snprintf(BufLen,zBuf, "%d:%d", a, b);
    SETBUF(jo, "compressionRatio");
  }
  n = db_int(0, "SELECT count(distinct mid) FROM mlink /*scan*/");
  cson_object_set(jo, "checkinCount", cson_value_new_integer((cson_int_t)n));
  n = db_int(0, "SELECT count(*) FROM filename /*scan*/");
  cson_object_set(jo, "fileCount", cson_value_new_integer((cson_int_t)n));
  n = db_int(0, "SELECT count(*) FROM tag  /*scan*/"
                " WHERE +tagname GLOB 'wiki-*'");
  cson_object_set(jo, "wikiPageCount", cson_value_new_integer((cson_int_t)n));
  n = db_int(0, "SELECT count(*) FROM tag  /*scan*/"
                " WHERE +tagname GLOB 'tkt-*'");
  cson_object_set(jo, "ticketCount", cson_value_new_integer((cson_int_t)n));
  n = db_int(0, "SELECT julianday('now') - (SELECT min(mtime) FROM event)"
                " + 0.99");
  cson_object_set(jo, "ageDays", cson_value_new_integer((cson_int_t)n));
  cson_object_set(jo, "ageYears", cson_value_new_double(n/365.24));
  sqlite3_snprintf(BufLen, zBuf, db_get("project-code",""));
  SETBUF(jo, "projectCode");
  sqlite3_snprintf(BufLen, zBuf, db_get("server-code",""));
  SETBUF(jo, "serverCode");
  cson_object_set(jo, "compiler", cson_value_new_string(COMPILER_NAME, strlen(COMPILER_NAME)));

  jv2 = cson_value_new_object();
  jo2 = cson_value_get_object(jv2);
  cson_object_set(jo, "sqlite", jv2);
  sqlite3_snprintf(BufLen, zBuf, "%.19s [%.10s] (%s)",
                   SQLITE_SOURCE_ID, &SQLITE_SOURCE_ID[20], SQLITE_VERSION);
  SETBUF(jo2, "version");
  zDb = db_name("repository");
  cson_object_set(jo2, "pageCount", cson_value_new_integer((cson_int_t)db_int(0, "PRAGMA %s.page_count", zDb)));
  cson_object_set(jo2, "pageSize", cson_value_new_integer((cson_int_t)db_int(0, "PRAGMA %s.page_size", zDb)));
  cson_object_set(jo2, "freeList", cson_value_new_integer((cson_int_t)db_int(0, "PRAGMA %s.freelist_count", zDb)));
  sqlite3_snprintf(BufLen, zBuf, "%s", db_text(0, "PRAGMA %s.encoding", zDb));
  SETBUF(jo2, "encoding");
  sqlite3_snprintf(BufLen, zBuf, "%s", db_text(0, "PRAGMA %s.journal_mode", zDb));
  cson_object_set(jo2, "journalMode", *zBuf ? cson_value_new_string(zBuf, strlen(zBuf)) : cson_value_null());
  return jv;
#undef SETBUF
}

/*
** Implements the /json/wiki family of pages/commands. Far from
** complete.
**
*/
cson_value * json_page_wiki(void){
  cson_value * jlist = NULL;
  cson_value * rows = NULL;
  Stmt q;
  wiki_prepare_page_list(&q);
  cson_sqlite3_stmt_to_json( q.pStmt, &jlist, 1 );
  db_finalize(&q);
  assert( NULL != jlist );
  rows = cson_object_take( cson_value_get_object(jlist), "rows" );
  assert( NULL != rows );
  cson_value_free( jlist );
  return rows;
}

/*
** Mapping of names to JSON pages/commands.  Each name is a subpath of
** /json (in CGI mode) or a subcommand of the json command in CLI mode
*/
static const JsonPageDef JsonPageDefs[] = {
/* please keep alphabetically sorted (case-insensitive) for maintenance reasons. */
{"cap", json_page_cap, 0},
{"HAI",json_page_version,0},
{"login",json_page_login,1/*should be >0. Only 0 for dev/testing purposes.*/},
{"logout",json_page_logout,1/*should be >0. Only 0 for dev/testing purposes.*/},
{"stat",json_page_stat,0},
{"version",json_page_version,0},
{"wiki",json_page_wiki,0},
/* Last entry MUST have a NULL name. */
{NULL,NULL}
};

/*
** WEBPAGE: json
**
** Pages under /json/... must be entered into JsonPageDefs.
*/
void json_page_top(void){
  int rc = FSL_JSON_E_UNKNOWN_COMMAND;
  Blob buf = empty_blob;
  char const * cmd;
  cson_value * payload = NULL;
  cson_value * root = NULL;
  JsonPageDef const * pageDef = NULL;
  json_mode_bootstrap();
  cmd = json_command_arg(1);
  /*cgi_printf("{\"cmd\":\"%s\"}\n",cmd); return;*/
  pageDef = json_handler_for_name(cmd,&JsonPageDefs[0]);
  if( ! pageDef ){
    json_err( FSL_JSON_E_UNKNOWN_COMMAND, cmd, 0 );
    return;
  }else if( pageDef->runMode < 0 /*CLI only*/){
    rc = FSL_JSON_E_WRONG_MODE;
  }else{
    rc = 0;
    payload = (*pageDef->func)();
  }
  if( g.json.resultCode ){
    json_err(g.json.resultCode, NULL, 0);
  }else{
    blob_zero(&buf);
    root = json_response_skeleton(rc, payload, NULL);
    cson_output_Blob( root, &buf, NULL );
    cson_value_free(root);
    cgi_set_content(&buf)/*takes ownership of the buf memory*/;
  }
}

/*
** COMMAND: json
**
** Usage: %fossil json subcommand
**
** The commands include:
**
**   stat
**   version (alias: HAI)
**
**
** TODOs:
**
**   wiki
**   timeline
**   tickets
**   ...
**
*/
void json_cmd_top(void){
  char const * cmd = NULL;
  int rc = 1002;
  cson_value * payload = NULL;
  JsonPageDef const * pageDef;
  memset( &g.perm, 0xff, sizeof(g.perm) )
    /* In CLI mode fossil does not use permissions
       and they all default to false. We enable them
       here because (A) fossil doesn't use them in local
       mode but (B) having them set gives us one less
       difference in the CLI/CGI/Server-mode JSON
       handling.
    */
    ;
  json_main_bootstrap();
  json_mode_bootstrap();
  if( g.argc<3 ){
    goto usage;
  }
  db_find_and_open_repository(0, 0);
  cmd = json_command_arg(1);
  if( !cmd || !*cmd ){
    goto usage;
  }
  pageDef = json_handler_for_name(cmd,&JsonPageDefs[0]);
  if( ! pageDef ){
    json_err( FSL_JSON_E_UNKNOWN_COMMAND, NULL, 1 );
    return;
  }else if( pageDef->runMode > 0 /*HTTP only*/){
    rc = FSL_JSON_E_WRONG_MODE;
  }else{
    rc = 0;
    payload = (pageDef->func)();
  }
  if( g.json.resultCode ){
    json_err(g.json.resultCode, NULL, 1);
  }else{
    payload = json_response_skeleton(rc, payload, NULL);
    cson_output_FILE( payload, stdout, &g.json.outOpt );
    cson_value_free( payload );
    if((0 != rc) && !g.isCGI){
      /* FIXME: we need a way of passing this error back
         up to the routine which called this callback.
         e.g. add g.errCode.
      */
      fossil_exit(1);
    }
  }
  return;
  usage:
  usage("subcommand");
}

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
** This file contains code to do formatting of wiki text.
*/
#include <assert.h>
#include "config.h"
#include "wikiformat.h"

#if INTERFACE
/*
** Allowed wiki transformation operations
*/
#define WIKI_NOFOLLOW       0x001
#define WIKI_HTML           0x002
#define WIKI_INLINE         0x004  /* Do not surround with <p>..</p> */
#define WIKI_NOBLOCK        0x008  /* No block markup of any kind */
#endif


/*
** These are the only markup attributes allowed.
*/
#define ATTR_ALIGN              1
#define ATTR_ALT                2
#define ATTR_BGCOLOR            3
#define ATTR_BORDER             4
#define ATTR_CELLPADDING        5
#define ATTR_CELLSPACING        6
#define ATTR_CLEAR              7
#define ATTR_COLOR              8
#define ATTR_COLSPAN            9
#define ATTR_COMPACT            10
#define ATTR_FACE               11
#define ATTR_HEIGHT             12
#define ATTR_HREF               13
#define ATTR_HSPACE             14
#define ATTR_ID                 15
#define ATTR_NAME               16
#define ATTR_ROWSPAN            17
#define ATTR_SIZE               18
#define ATTR_SRC                19
#define ATTR_START              20
#define ATTR_TYPE               21
#define ATTR_VALIGN             22
#define ATTR_VALUE              23
#define ATTR_VSPACE             24
#define ATTR_WIDTH              25
#define AMSK_ALIGN              0x0000001
#define AMSK_ALT                0x0000002
#define AMSK_BGCOLOR            0x0000004
#define AMSK_BORDER             0x0000008
#define AMSK_CELLPADDING        0x0000010
#define AMSK_CELLSPACING        0x0000020
#define AMSK_CLEAR              0x0000040
#define AMSK_COLOR              0x0000080
#define AMSK_COLSPAN            0x0000100
#define AMSK_COMPACT            0x0000200
#define AMSK_FACE               0x0000400
#define AMSK_HEIGHT             0x0000800
#define AMSK_HREF               0x0001000
#define AMSK_HSPACE             0x0002000
#define AMSK_ID                 0x0004000
#define AMSK_NAME               0x0008000
#define AMSK_ROWSPAN            0x0010000
#define AMSK_SIZE               0x0020000
#define AMSK_SRC                0x0040000
#define AMSK_START              0x0080000
#define AMSK_TYPE               0x0100000
#define AMSK_VALIGN             0x0200000
#define AMSK_VALUE              0x0400000
#define AMSK_VSPACE             0x0800000
#define AMSK_WIDTH              0x1000000

static const struct AllowedAttribute {
  const char *zName;
  unsigned int iMask;
} aAttribute[] = {
  { 0, 0 },
  { "align",         AMSK_ALIGN,          },
  { "alt",           AMSK_ALT,            },
  { "bgcolor",       AMSK_BGCOLOR,        },
  { "border",        AMSK_BORDER,         },
  { "cellpadding",   AMSK_CELLPADDING,    },
  { "cellspacing",   AMSK_CELLSPACING,    },
  { "clear",         AMSK_CLEAR,          },
  { "color",         AMSK_COLOR,          },
  { "colspan",       AMSK_COLSPAN,        },
  { "compact",       AMSK_COMPACT,        },
  { "face",          AMSK_FACE,           },
  { "height",        AMSK_HEIGHT,         },
  { "href",          AMSK_HREF,           },
  { "hspace",        AMSK_HSPACE,         },
  { "id",            AMSK_ID,             },
  { "name",          AMSK_NAME,           },
  { "rowspan",       AMSK_ROWSPAN,        },
  { "size",          AMSK_SIZE,           },
  { "src",           AMSK_SRC,            },
  { "start",         AMSK_START,          },
  { "type",          AMSK_TYPE,           },
  { "valign",        AMSK_VALIGN,         },
  { "value",         AMSK_VALUE,          },
  { "vspace",        AMSK_VSPACE,         },
  { "width",         AMSK_WIDTH,          },
};

/*
** Use binary search to locate a tag in the aAttribute[] table.
*/
static int findAttr(const char *z){
  int i, c, first, last;
  first = 1;
  last = sizeof(aAttribute)/sizeof(aAttribute[0]) - 1;
  while( first<=last ){
    i = (first+last)/2;
    c = strcmp(aAttribute[i].zName, z);
    if( c==0 ){
      return i;
    }else if( c<0 ){
      first = i+1;
    }else{
      last = i-1;
    }
  }
  return 0;
}



/*
** Allowed markup.
**
** Except for MARKUP_INVALID, this must all be in alphabetical order
** and in numerical sequence.  The first markup type must be zero.
** The value for MARKUP_XYZ must correspond to the <xyz> entry 
** in aAllowedMarkup[].
*/
#define MARKUP_INVALID           0
#define MARKUP_A                 1
#define MARKUP_ADDRESS           2
#define MARKUP_B                 3
#define MARKUP_BIG               4
#define MARKUP_BLOCKQUOTE        5
#define MARKUP_BR                6
#define MARKUP_CENTER            7
#define MARKUP_CITE              8
#define MARKUP_CODE              9
#define MARKUP_DD               10
#define MARKUP_DFN              11
#define MARKUP_DIV              12
#define MARKUP_DL               13
#define MARKUP_DT               14
#define MARKUP_EM               15
#define MARKUP_FONT             16
#define MARKUP_H1               17
#define MARKUP_H2               18
#define MARKUP_H3               19
#define MARKUP_H4               20
#define MARKUP_H5               21
#define MARKUP_H6               22
#define MARKUP_HR               23
#define MARKUP_I                24
#define MARKUP_IMG              25
#define MARKUP_KBD              26
#define MARKUP_LI               27
#define MARKUP_NOBR             28
#define MARKUP_NOWIKI           29
#define MARKUP_OL               30
#define MARKUP_P                31
#define MARKUP_PRE              32
#define MARKUP_S                33
#define MARKUP_SAMP             34
#define MARKUP_SMALL            35
#define MARKUP_STRIKE           36
#define MARKUP_STRONG           37
#define MARKUP_SUB              38
#define MARKUP_SUP              39
#define MARKUP_TABLE            40
#define MARKUP_TD               41
#define MARKUP_TH               42
#define MARKUP_TR               43
#define MARKUP_TT               44
#define MARKUP_U                45
#define MARKUP_UL               46
#define MARKUP_VAR              47
#define MARKUP_VERBATIM         48

/*
** The various markup is divided into the following types:
*/
#define MUTYPE_SINGLE      0x0001   /* <img>, <br>, or <hr> */
#define MUTYPE_BLOCK       0x0002   /* Forms a new paragraph. ex: <p>, <h2> */
#define MUTYPE_FONT        0x0004   /* Font changes. ex: <b>, <font>, <sub> */
#define MUTYPE_LIST        0x0010   /* Lists.  <ol>, <ul>, or <dl> */
#define MUTYPE_LI          0x0020   /* List items.  <li>, <dd>, <dt> */
#define MUTYPE_TABLE       0x0040   /* <table> */
#define MUTYPE_TR          0x0080   /* <tr> */
#define MUTYPE_TD          0x0100   /* <td> or <th> */
#define MUTYPE_SPECIAL     0x0200   /* <nowiki> or <verbatim> */
#define MUTYPE_HYPERLINK   0x0400   /* <a> */

/*
** These markup types must have an end tag.
*/
#define MUTYPE_STACK  (MUTYPE_BLOCK | MUTYPE_FONT | MUTYPE_LIST | MUTYPE_TABLE)

/*
** This markup types are allowed for "inline" text.
*/
#define MUTYPE_INLINE (MUTYPE_FONT | MUTYPE_HYPERLINK)

static const struct AllowedMarkup {
  const char *zName;       /* Name of the markup */
  char iCode;              /* The MARKUP_* code */
  short int iType;         /* The MUTYPE_* code */
  int allowedAttr;         /* Allowed attributes on this markup */
} aMarkup[] = {
 { 0,               MARKUP_INVALID,      0,                    0  },
 { "a",             MARKUP_A,            MUTYPE_HYPERLINK,
                    AMSK_HREF|AMSK_NAME },
 { "address",       MARKUP_ADDRESS,      MUTYPE_BLOCK,         0  },
 { "b",             MARKUP_B,            MUTYPE_FONT,          0  },
 { "big",           MARKUP_BIG,          MUTYPE_FONT,          0  },
 { "blockquote",    MARKUP_BLOCKQUOTE,   MUTYPE_BLOCK,         0  },
 { "br",            MARKUP_BR,           MUTYPE_SINGLE,        AMSK_CLEAR  },
 { "center",        MARKUP_CENTER,       MUTYPE_BLOCK,         0  },
 { "cite",          MARKUP_CITE,         MUTYPE_FONT,          0  },
 { "code",          MARKUP_CODE,         MUTYPE_FONT,          0  },
 { "dd",            MARKUP_DD,           MUTYPE_LI,            0  },
 { "dfn",           MARKUP_DFN,          MUTYPE_FONT,          0  },
 { "div",           MARKUP_DIV,          MUTYPE_BLOCK,         AMSK_ID      },
 { "dl",            MARKUP_DL,           MUTYPE_LIST,          AMSK_COMPACT },
 { "dt",            MARKUP_DT,           MUTYPE_LI,            0  },
 { "em",            MARKUP_EM,           MUTYPE_FONT,          0  },
 { "font",          MARKUP_FONT,         MUTYPE_FONT,
                    AMSK_COLOR|AMSK_FACE|AMSK_SIZE   },
 { "h1",            MARKUP_H1,           MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "h2",            MARKUP_H2,           MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "h3",            MARKUP_H3,           MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "h4",            MARKUP_H4,           MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "h5",            MARKUP_H5,           MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "h6",            MARKUP_H6,           MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "hr",            MARKUP_HR,           MUTYPE_SINGLE,        
                    AMSK_ALIGN|AMSK_COLOR|AMSK_SIZE|AMSK_WIDTH  },
 { "i",             MARKUP_I,            MUTYPE_FONT,          0  },
 { "img",           MARKUP_IMG,          MUTYPE_SINGLE,        
                    AMSK_ALIGN|AMSK_ALT|AMSK_BORDER|AMSK_HEIGHT|
                    AMSK_HSPACE|AMSK_SRC|AMSK_VSPACE|AMSK_WIDTH  },
 { "kbd",           MARKUP_KBD,          MUTYPE_FONT,          0  },
 { "li",            MARKUP_LI,           MUTYPE_LI,            
                    AMSK_TYPE|AMSK_VALUE  },
 { "nobr",          MARKUP_NOBR,         MUTYPE_FONT,          0  },
 { "nowiki",        MARKUP_NOWIKI,       MUTYPE_SPECIAL,       0  },
 { "ol",            MARKUP_OL,           MUTYPE_LIST,          
                    AMSK_START|AMSK_TYPE|AMSK_COMPACT  },
 { "p",             MARKUP_P,            MUTYPE_BLOCK,         AMSK_ALIGN  },
 { "pre",           MARKUP_PRE,          MUTYPE_BLOCK,         0  },
 { "s",             MARKUP_S,            MUTYPE_FONT,          0  },
 { "samp",          MARKUP_SAMP,         MUTYPE_FONT,          0  },
 { "small",         MARKUP_SMALL,        MUTYPE_FONT,          0  },
 { "strike",        MARKUP_STRIKE,       MUTYPE_FONT,          0  },
 { "strong",        MARKUP_STRONG,       MUTYPE_FONT,          0  },
 { "sub",           MARKUP_SUB,          MUTYPE_FONT,          0  },
 { "sup",           MARKUP_SUP,          MUTYPE_FONT,          0  },
 { "table",         MARKUP_TABLE,        MUTYPE_TABLE,         
                    AMSK_ALIGN|AMSK_BGCOLOR|AMSK_BORDER|AMSK_CELLPADDING|
                    AMSK_CELLSPACING|AMSK_HSPACE|AMSK_VSPACE  },
 { "td",            MARKUP_TD,           MUTYPE_TD,            
                    AMSK_ALIGN|AMSK_BGCOLOR|AMSK_COLSPAN|
                    AMSK_ROWSPAN|AMSK_VALIGN  },
 { "th",            MARKUP_TH,           MUTYPE_TD,
                    AMSK_ALIGN|AMSK_BGCOLOR|AMSK_COLSPAN|
                    AMSK_ROWSPAN|AMSK_VALIGN  },
 { "tr",            MARKUP_TR,           MUTYPE_TR, 
                    AMSK_ALIGN|AMSK_BGCOLOR||AMSK_VALIGN  },
 { "tt",            MARKUP_TT,           MUTYPE_FONT,          0  },
 { "u",             MARKUP_U,            MUTYPE_FONT,          0  },
 { "ul",            MARKUP_UL,           MUTYPE_LIST,          
                    AMSK_TYPE|AMSK_COMPACT  },
 { "var",           MARKUP_VAR,          MUTYPE_FONT,          0  },
 { "verbatim",      MARKUP_VERBATIM,     MUTYPE_SPECIAL,       AMSK_ID },
};

/*
** Use binary search to locate a tag in the aMarkup[] table.
*/
static int findTag(const char *z){
  int i, c, first, last;
  first = 1;
  last = sizeof(aMarkup)/sizeof(aMarkup[0]) - 1;
  while( first<=last ){
    i = (first+last)/2;
    c = strcmp(aMarkup[i].zName, z);
    if( c==0 ){
      assert( aMarkup[i].iCode==i );
      return i;
    }else if( c<0 ){
      first = i+1;
    }else{
      last = i-1;
    }
  }
  return MARKUP_INVALID;
}

/*
** Token types
*/
#define TOKEN_MARKUP        1    /* <...> */
#define TOKEN_CHARACTER     2    /* "&" or "<" not part of markup */
#define TOKEN_LINK          3    /* [...] */
#define TOKEN_PARAGRAPH     4    /* blank lines */
#define TOKEN_NEWLINE       5    /* A single "\n" */
#define TOKEN_BULLET        6    /*  "  *  " */
#define TOKEN_ENUM          7    /*  "  \(?\d+[.)]?  " */
#define TOKEN_INDENT        8    /*  "   " */
#define TOKEN_TEXT          9    /* None of the above */

/*
** State flags
*/
#define AT_NEWLINE          0x001  /* At start of a line */
#define AT_PARAGRAPH        0x002  /* At start of a paragraph */
#define ALLOW_WIKI          0x004  /* Allow wiki markup */
#define FONT_MARKUP_ONLY    0x008  /* Only allow MUTYPE_FONT markup */
#define INLINE_MARKUP_ONLY  0x010  /* Allow only "inline" markup */
#define IN_LIST             0x020  /* Within wiki <ul> or <ol> */

/*
** Current state of the rendering engine
*/
typedef struct Renderer Renderer;
struct Renderer {
  Blob *pOut;                 /* Output appended to this blob */
  int state;                  /* Flag that govern rendering */
  int wikiList;               /* Current wiki list type */
  int inVerbatim;             /* True in <verbatim> mode */
  int preVerbState;           /* Value of state prior to verbatim */
  int wantAutoParagraph;      /* True if a <p> is desired */
  int inAutoParagraph;        /* True if within an automatic paragraph */
  const char *zVerbatimId;    /* The id= attribute of <verbatim> */
  int nStack;                 /* Number of elements on the stack */
  int nAlloc;                 /* Space allocated for aStack */
  struct sStack {
    short iCode;                 /* Markup code */
    short allowWiki;             /* ALLOW_WIKI if wiki allowed before tag */
    const char *zId;             /* ID attribute or NULL */
  } *aStack;
};


/*
** z points to a "<" character.  Check to see if this is the start of
** a valid markup.  If it is, return the total number of characters in
** the markup including the initial "<" and the terminating ">".  If
** it is not well-formed markup, return 0.
*/
static int markupLength(const char *z){
  int n = 1;
  int inparen = 0;
  if( z[n]=='/' ){ n++; }
  if( !isalpha(z[n]) ) return 0;
  while( isalnum(z[n]) ){ n++; }
  if( z[n]!='>' && !isspace(z[n]) ) return 0;
  while( z[n] && (z[n]!='>' || inparen) ){
    if( z[n]=='"' ){
      inparen = !inparen;
    }
    n++;
  }
  if( z[n]!='>' ) return 0;
  return n+1;
}

/*
** z points to a "\n" character.  Check to see if this newline is
** followed by one or more blank lines.  If it is, return the number
** of characters through the closing "\n".  If not, return 0.
*/
static int paragraphBreakLength(const char *z){
  int i, n;
  int nNewline = 1;
  for(i=1, n=0; isspace(z[i]); i++){
    if( z[i]=='\n' ){
      nNewline++;
      n = i;
    }
  }
  if( nNewline>=2 ){
    return n+1;
  }else{
    return 0;
  }
}

/*
** Return the number of characters until the next "interesting"
** characters.
**
** Interesting characters are:
**
**      <
**      &
**      \n
**      [
**
** The "[" and "\n" are only considered interesting if the "useWiki"
** flag is set.
*/
static int textLength(const char *z, int useWiki){
  int n = 0;
  int c;
  while( (c = z[0])!=0 && c!='<' && c!='&' &&
               (useWiki==0 || (c!='[' && c!='\n')) ){
    n++;
    z++;
  }
  return n; 
}

/*
** Return true if z[] begins with an HTML character element.
*/
static int isElement(const char *z){
  int i;
  assert( z[0]=='&' );
  if( z[1]=='#' ){
    for(i=2; isdigit(z[i]); i++){}
    return i>2 && z[i]==';';
  }else{
    for(i=1; isalpha(z[i]); i++){}
    return i>1 && z[i]==';';
  }
}

/*
** Check to see if the z[] string is the beginning of a wiki bullet.
** If it is, return the length of the bullet text.  Otherwise return 0.
*/
static int bulletLength(const char *z){
  int i, n;
  n = 0;
  i = 0;
  while( z[n]==' ' || z[n]=='\t' ){
    if( z[n]=='\t' ) i++;
    i++;
    n++;
  }
  if( i<2 || z[n]!='*' ) return 0;
  n++;
  i = 0;
  while( z[n]==' ' || z[n]=='\t' ){
    if( z[n]=='\t' ) i++;
    i++;
    n++;
  }
  if( i<2 || isspace(z[n]) ) return 0;
  return n;
}

/*
** Check to see if the z[] string is the beginning of a enumeration value.
** If it is, return the length of the bullet text.  Otherwise return 0.
**
** Syntax:
**    *  a tab or two or more spaces
**    *  one or more digits
**    *  optional "."
**    *  another tab or two ore more spaces.
**
*/
static int enumLength(const char *z){
  int i, n;
  n = 0;
  i = 0;
  while( z[n]==' ' || z[n]=='\t' ){
    if( z[n]=='\t' ) i++;
    i++;
    n++;
  }
  if( i<2 ) return 0;
  for(i=0; isdigit(z[n]); i++, n++){}
  if( i==0 ) return 0;
  if( z[n]=='.' ){
    n++;
  }
  i = 0;
  while( z[n]==' ' || z[n]=='\t' ){
    if( z[n]=='\t' ) i++;
    i++;
    n++;
  }
  if( i<2 || isspace(z[n]) ) return 0;
  return n;
}

/*
** Check to see if the z[] string is the beginning of an indented
** paragraph.  If it is, return the length of the indent.  Otherwise
** return 0.
*/
static int indentLength(const char *z){
  int i, n;
  n = 0;
  i = 0;
  while( z[n]==' ' || z[n]=='\t' ){
    if( z[n]=='\t' ) i++;
    i++;
    n++;
  }
  if( i<2 || isspace(z[n]) ) return 0;
  return n;
}

/*
** Check to see if the z[] string is a wiki hyperlink.  If it is,
** return the length of the hyperlink.  Otherwise return 0.
*/
static int linkLength(const char *z){
  int n;
  assert( z[0]=='[' );
  for(n=0; z[n] && z[n]!=']'; n++){}
  if( z[n]==']' ){
    return n+1;
  }else{
    return 0;
  }
}


/*
** z points to the start of a token.  Return the number of
** characters in that token.  Write the token type into *pTokenType.
*/
static int nextToken(const char *z, Renderer *p, int *pTokenType){
  int n;
  if( z[0]=='<' ){
    n = markupLength(z);
    if( n>0 ){
      *pTokenType = TOKEN_MARKUP;
      return n;
    }else{
      *pTokenType = TOKEN_CHARACTER;
      return 1;
    }
  }
  if( z[0]=='&' && (p->inVerbatim || !isElement(z)) ){
    *pTokenType = TOKEN_CHARACTER;
    return 1;
  }
  if( (p->state & ALLOW_WIKI)!=0 ){
    if( z[0]=='\n' ){
      n = paragraphBreakLength(z);
      if( n>0 ){
        *pTokenType = TOKEN_PARAGRAPH;
        return n;
      }else if( isspace(z[1]) ){
        *pTokenType = TOKEN_NEWLINE;
        return 1;
      }
    }
    if( (p->state & AT_NEWLINE)!=0 && isspace(z[0]) ){
      n = bulletLength(z);
      if( n>0 ){
        *pTokenType = TOKEN_BULLET;
        return n;
      }
      n = enumLength(z);
      if( n>0 ){
        *pTokenType = TOKEN_ENUM;
        return n;
      }
    }
    if( (p->state & AT_PARAGRAPH)!=0 && isspace(z[0]) ){
      n = indentLength(z);
      if( n>0 ){
        *pTokenType = TOKEN_INDENT;
        return n;
      }
    }
    if( z[0]=='[' && (n = linkLength(z))>0 ){
      *pTokenType = TOKEN_LINK;
      return n;
    }
  }
  *pTokenType = TOKEN_TEXT;
  return 1 + textLength(z+1, p->state & ALLOW_WIKI);
}

/*
** A single markup is parsed into an instance of the following
** structure.
*/
typedef struct ParsedMarkup ParsedMarkup;
struct ParsedMarkup {
  unsigned char endTag;   /* True if </...> instead of <...> */
  unsigned char iCode;    /* MARKUP_* */
  unsigned char nAttr;    /* Number of attributes */
  unsigned short iType;   /* MUTYPE_* */
  struct {
    unsigned char iACode;    /* ATTR_* */
    char *zValue;            /* Argument to this attribute.  Might be NULL */
    char cTerm;              /* Original argument termination character */
  } aAttr[10];
};

/*
** z[] is an HTML markup element - something that begins with '<'.
** Parse this element into the p structure.
**
** The content of z[] might be modified by converting characters 
** to lowercase and by inserting some "\000" characters.
*/
static void parseMarkup(ParsedMarkup *p, char *z){
  int i, j, c;
  int iACode;
  char *zValue;
  int seen = 0;
  char zTag[100];

  if( z[1]=='/' ){
    p->endTag = 1;
    i = 2;
  }else{
    p->endTag = 0;
    i = 1;
  }
  j = 0;
  while( isalnum(z[i]) ){ 
    if( j<sizeof(zTag)-1 ) zTag[j++] = tolower(z[i]);
    i++;
  }
  zTag[j] = 0;
  p->iCode = findTag(zTag);
  p->iType = aMarkup[p->iCode].iType;
  p->nAttr = 0;
  while( isspace(z[i]) ){ i++; }
  while( p->nAttr<8 && isalpha(z[i]) ){
    int attrOk;    /* True to preserver attribute.  False to ignore it */
    j = 0;
    while( isalnum(z[i]) ){ 
      if( j<sizeof(zTag)-1 ) zTag[j++] = tolower(z[i]);
      i++;
    }
    zTag[j] = 0;
    p->aAttr[p->nAttr].iACode = iACode = findAttr(zTag);
    attrOk = iACode!=0 && (seen & aAttribute[iACode].iMask)==0;
    while( isspace(z[i]) ){ z++; }
    if( z[i]!='=' ){
      p->aAttr[p->nAttr].zValue = 0;
      p->aAttr[p->nAttr].cTerm = 0;
      c = 0;
    }else{
      i++;
      while( isspace(z[i]) ){ z++; }
      if( z[i]=='"' ){
        i++;
        zValue = &z[i];
        while( z[i] && z[i]!='"' ){ i++; }
      }else{
        zValue = &z[i];
        while( !isspace(z[i]) && z[i]!='>' ){ z++; }
      }
      if( attrOk ){
        p->aAttr[p->nAttr].zValue = zValue;
        p->aAttr[p->nAttr].cTerm = c = z[i];
        z[i] = 0;
      }
      i++;
    }
    if( attrOk ){
      seen |= aAttribute[iACode].iMask;
      p->nAttr++;
    }
    while( isspace(z[i]) ){ i++; }
    if( z[i]=='>' || (z[i]=='/' && z[i+1]=='>') ) break;
  }
}

/*
** Render markup on the given blob.
*/
static void renderMarkup(Blob *pOut, ParsedMarkup *p){
  int i;
  if( p->endTag ){
    blob_appendf(pOut, "</%s>", aMarkup[p->iCode].zName);
  }else{
    blob_appendf(pOut, "<%s", aMarkup[p->iCode].zName);
    for(i=0; i<p->nAttr; i++){
      blob_appendf(pOut, " %s", aAttribute[p->aAttr[i].iACode].zName);
      if( p->aAttr[i].zValue ){
        blob_appendf(pOut, "=\"%s\"", p->aAttr[i].zValue);
      }
    }
    blob_append(pOut, ">", 1);
  }
}

/*
** When the markup was parsed, some "\000" may have been inserted.
** This routine restores to those "\000" values back to their
** original content.
*/
static void unparseMarkup(ParsedMarkup *p){
  int i, n;
  for(i=0; i<p->nAttr; i++){
    char *z = p->aAttr[i].zValue;
    if( z==0 ) continue;
    n = strlen(z);
    z[n] = p->aAttr[i].cTerm;
  }
}

/*
** Return the ID attribute for markup.  Return NULL if there is no
** ID attribute.
*/
static const char *markupId(ParsedMarkup *p){
  int i;
  for(i=0; i<p->nAttr; i++){
    if( p->aAttr[i].iACode==ATTR_ID ){
      return p->aAttr[i].zValue;
    }
  }
  return 0;
}

/*
** Pop a single element off of the stack.  As the element is popped,
** output its end tag if it is not a </div> tag.
*/
static void popStack(Renderer *p){
  if( p->nStack ){
    int iCode;
    p->nStack--;
    iCode = p->aStack[p->nStack].iCode;
    if( iCode!=MARKUP_DIV ){
      blob_appendf(p->pOut, "</%s>", aMarkup[iCode].zName);
    }
  }
}

/*
** Push a new markup value onto the stack.  Enlarge the stack
** if necessary.
*/
static void pushStackWithId(Renderer *p, int elem, const char *zId, int w){
  if( p->nStack>=p->nAlloc ){
    p->nAlloc = p->nAlloc*2 + 100;
    p->aStack = realloc(p->aStack, p->nAlloc*sizeof(p->aStack[0]));
    if( p->aStack==0 ){
      fossil_panic("out of memory");
    }
  }
  p->aStack[p->nStack].iCode = elem;
  p->aStack[p->nStack].zId = zId;
  p->aStack[p->nStack].allowWiki = w;
  p->nStack++;
}
static void pushStack(Renderer *p, int elem){
  pushStackWithId(p, elem, 0, 0);
}

/*
** Pop the stack until the top-most iTag element is removed.
** If there is no iTag element on the stack, this routine
** is a no-op.
*/
static void popStackToTag(Renderer *p, int iTag){
  int i;
  for(i=p->nStack-1; i>=0; i--){
    if( p->aStack[i].iCode!=iTag ) continue;
    if( p->aStack[i].zId ) continue;
    break;
  }
  if( i<0 ) return;
  while( p->nStack>i ){
    popStack(p);
  }
}

/*
** Attempt to find a find a tag of type iTag with id zId.  Return -1
** if not found.  If found, return its stack level.  
*/
static int findTagWithId(Renderer *p, int iTag, const char *zId){
  int i;
  assert( zId!=0 );
  for(i=p->nStack-1; i>=0; i--){
    if( p->aStack[i].iCode!=iTag ) continue;
    if( p->aStack[i].zId==0 ) continue;
    if( strcmp(zId, p->aStack[i].zId)!=0 ) continue;
    break;
  }
  return i;
}

/*
** Pop the stack until the top-most element of the stack
** is an element that matches the type in iMask.  Return
** code of the markup element that is on left on top of the stack.
** If the stack does not have an element
** that matches iMask, then leave the stack unchanged and
** return false (MARKUP_INVALID).
*/
static int backupToType(Renderer *p, int iMask){
  int i;
  for(i=p->nStack-1; i>=0; i--){
    if( aMarkup[p->aStack[i].iCode].iType & iMask ) break;
  }
  if( i<0 ) return 0;
  i++;
  while( p->nStack>i ){
    popStack(p);
  }
  return p->aStack[i-1].iCode;
}

/*
** Begin a new paragraph if that something that is needed.
*/
static void startAutoParagraph(Renderer *p){
  if( p->wantAutoParagraph==0 ) return;
  blob_appendf(p->pOut, "<p>", -1);
  pushStack(p, MARKUP_P);
  p->wantAutoParagraph = 0;
  p->inAutoParagraph = 1;
}

/*
** End a paragraph if we are in one.
*/
static void endAutoParagraph(Renderer *p){
  if( p->inAutoParagraph ){
    popStackToTag(p, MARKUP_P);
    p->inAutoParagraph = 0;
  }
}

/*
** If the input string corresponds to an existing baseline,
** return true.
*/
static int is_valid_uuid(const char *z){
  int n = strlen(z);
  if( n<4 || n>UUID_SIZE ) return 0;
  if( !validate16(z, n) ) return 0;
  return 1;
}

/*
** zTarget is guaranteed to be a UUID.  It might be the UUID of a ticket.
** If it is, store in *pClosed a true or false depending on whether or not
** the ticket is closed and return true. If zTarget
** is not the UUID of a ticket, return false.
*/
static int is_ticket(
  const char *zTarget,    /* Ticket UUID */
  int *pClosed            /* True if the ticket is closed */
){
  static Stmt q;
  static int once = 1;
  int n;
  int rc;
  char zLower[UUID_SIZE+1];
  char zUpper[UUID_SIZE+1];
  n = strlen(zTarget);
  memcpy(zLower, zTarget, n+1);
  canonical16(zLower, n+1);
  memcpy(zUpper, zLower, n+1);
  zUpper[n-1]++;
  if( once ){
    const char *zClosedExpr = db_get("ticket-closed-expr", "status='Closed'");
    db_static_prepare(&q, 
      "SELECT %s FROM ticket "
      " WHERE tkt_uuid>=:lwr AND tkt_uuid<:upr",
      zClosedExpr
    );
    once = 0;
  }
  db_bind_text(&q, ":lwr", zLower);
  db_bind_text(&q, ":upr", zUpper);
  if( db_step(&q)==SQLITE_ROW ){
    rc = 1;
    *pClosed = db_column_int(&q, 0);
  }else{
    rc = 0;
  }
  db_reset(&q);
  return rc;
}

/*
** Resolve a hyperlink.  The zTarget argument is the content of the [...]
** in the wiki.  Append to the output string whatever text is approprate
** for opening the hyperlink.  Write into zClose[0...nClose-1] text that will
** close the markup.
**
** Actually, this routine might or might not append the hyperlink, depending
** on current rendering rules: specifically does the current user have
** "History" permission.
**
**    [http://www.fossil-scm.org/]
**    [https://www.fossil-scm.org/]
**    [ftp://www.fossil-scm.org/]
**    [mailto:fossil-users@lists.fossil-scm.org]
**
**    [/path]
**
**    [./relpath]
**
**    [WikiPageName]
**
**    [0123456789abcdef]
**
**    [#fragment]
*/
static void openHyperlink(
  Renderer *p,            /* Rendering context */
  const char *zTarget,    /* Hyperlink traget; text within [...] */
  char *zClose,           /* Write hyperlink closing text here */
  int nClose              /* Bytes available in zClose[] */
){
  const char *zTerm = "</a>";
  assert( nClose>10 );

  if( strncmp(zTarget, "http:", 5)==0 
   || strncmp(zTarget, "https:", 6)==0
   || strncmp(zTarget, "ftp:", 4)==0 
   || strncmp(zTarget, "mailto:", 7)==0
  ){
    blob_appendf(p->pOut, "<a href=\"%s\">", zTarget);
  }else if( zTarget[0]=='/' ){
    if( 1 /* g.okHistory */ ){
      blob_appendf(p->pOut, "<a href=\"%s%h\">", g.zBaseURL, zTarget);
    }else{
      zTerm = "";
    }
  }else if( zTarget[0]=='.' || zTarget[0]=='#' ){
    if( 1 /* g.okHistory */ ){
      blob_appendf(p->pOut, "<a href=\"%h\">", zTarget);
    }else{
      zTerm = "";
    }
  }else if( is_valid_uuid(zTarget) ){
    int isClosed = 0;
    if( is_ticket(zTarget, &isClosed) ){
      /* Special display processing for tickets.  Display the hyperlink
      ** as crossed out if the ticket is closed.
      */
      if( isClosed ){
        if( g.okHistory ){
          blob_appendf(p->pOut,"<a href=\"%s/info/%s\"><s>",
              g.zBaseURL, zTarget
          );
          zTerm = "</s></a>";
        }else{
          blob_appendf(p->pOut,"<s>");
          zTerm = "</s>";
        }
      }else{
        if( g.okHistory ){
          blob_appendf(p->pOut,"<a href=\"%s/info/%s\">",
              g.zBaseURL, zTarget
          );
        }else{
          zTerm = "";
        }
      }
    }else if( g.okHistory ){
      blob_appendf(p->pOut, "<a href=\"%s/info/%s\">", g.zBaseURL, zTarget);
    }
  }else if( wiki_name_is_wellformed(zTarget) ){
    blob_appendf(p->pOut, "<a href=\"%s/wiki?name=%T\">", g.zBaseURL, zTarget);
  }else{
    blob_appendf(p->pOut, "[bad-link: %h]", zTarget);
    zTerm = "";
  }
  assert( strlen(zTerm)<nClose );
  strcpy(zClose, zTerm);
}

/*
** Check to see if the given parsed markup is the correct
** </verbatim> tag.
*/
static int endVerbatim(Renderer *p, ParsedMarkup *pMarkup){
  char *z;
  assert( p->inVerbatim );
  if( pMarkup->iCode!=MARKUP_VERBATIM ) return 0;
  if( !pMarkup->endTag ) return 0;
  if( p->zVerbatimId==0 ) return 1;
  if( pMarkup->nAttr!=1 ) return 0;
  z = pMarkup->aAttr[0].zValue;
  return strcmp(z, p->zVerbatimId)==0;
}

/*
** Return the MUTYPE for the top of the stack.
*/
static int stackTopType(Renderer *p){
  if( p->nStack<=0 ) return 0;
  return aMarkup[p->aStack[p->nStack-1].iCode].iType;
}

/*
** Convert the wiki in z[] into html in the renderer p.  The
** renderer has already been initialized.
**
** This routine will probably modify the content of z[].
*/
static void wiki_render(Renderer *p, char *z){
  int tokenType;
  ParsedMarkup markup;
  int n;
  int inlineOnly = (p->state & INLINE_MARKUP_ONLY)!=0;

  while( z[0] ){
    n = nextToken(z, p, &tokenType);
    p->state &= ~(AT_NEWLINE|AT_PARAGRAPH);
    switch( tokenType ){
      case TOKEN_PARAGRAPH: {
        if( inlineOnly ){
          /* blob_append(p->pOut, " &para; ", -1); */
          blob_append(p->pOut, " &nbsp;&nbsp; ", -1);
        }else{
          if( p->wikiList ){
            popStackToTag(p, p->wikiList);
            p->wikiList = 0;
          }
          endAutoParagraph(p);
          blob_appendf(p->pOut, "\n\n", 1);
          p->wantAutoParagraph = 1;
        }
        p->state |= AT_PARAGRAPH|AT_NEWLINE;
        break;
      }
      case TOKEN_NEWLINE: {
        blob_append(p->pOut, "\n", 1);
        p->state |= AT_NEWLINE;
        break;
      }
      case TOKEN_BULLET: {
        if( inlineOnly ){
          blob_append(p->pOut, " &bull; ", -1);
        }else{
          if( p->wikiList!=MARKUP_UL ){
            if( p->wikiList ){
              popStackToTag(p, p->wikiList);
            }
            pushStack(p, MARKUP_UL);
            blob_append(p->pOut, "<ul>", 4);
            p->wikiList = MARKUP_UL;
          }
          popStackToTag(p, MARKUP_LI);
          startAutoParagraph(p);
          pushStack(p, MARKUP_LI);
          blob_append(p->pOut, "<li>", 4);
        }
        break;
      }
      case TOKEN_ENUM: {
        if( inlineOnly ){
          blob_appendf(p->pOut, " (%d) ", atoi(z));
        }else{
          if( p->wikiList!=MARKUP_OL ){
            if( p->wikiList ){
              popStackToTag(p, p->wikiList);
            }
            pushStack(p, MARKUP_OL);
            blob_append(p->pOut, "<ol>", 4);
            p->wikiList = MARKUP_OL;
          }
          popStackToTag(p, MARKUP_LI);
          startAutoParagraph(p);
          pushStack(p, MARKUP_LI);
          blob_appendf(p->pOut, "<li value=\"%d\">", atoi(z));
        }
        break;
      }
      case TOKEN_INDENT: {
        if( !inlineOnly ){
          assert( p->wikiList==0 );
          pushStack(p, MARKUP_BLOCKQUOTE);
          blob_append(p->pOut, "<blockquote>", -1);
          p->wantAutoParagraph = 0;
          p->wikiList = MARKUP_BLOCKQUOTE;
        }
        break;
      }
      case TOKEN_CHARACTER: {
        startAutoParagraph(p);
        if( z[0]=='<' ){
          blob_append(p->pOut, "&lt;", 4);
        }else if( z[0]=='&' ){
          blob_append(p->pOut, "&amp;", 5);
        }
        break;
      }
      case TOKEN_LINK: {
        char *zTarget;
        char *zDisplay = 0;
        int i, j;
        int savedState;
        char zClose[20];

        startAutoParagraph(p);
        zTarget = &z[1];
        for(i=1; z[i] && z[i]!=']'; i++){
          if( z[i]=='|' && zDisplay==0 ){
            zDisplay = &z[i+1];
            z[i] = 0;
            for(j=i-1; j>0 && isspace(z[j]); j--){ z[j] = 0; }
          }
        }
        z[i] = 0;
        if( zDisplay==0 ){
          zDisplay = zTarget;
        }else{
          while( isspace(*zDisplay) ) zDisplay++;
        }
        openHyperlink(p, zTarget, zClose, sizeof(zClose));
        savedState = p->state;
        p->state &= ~ALLOW_WIKI;
        p->state |= FONT_MARKUP_ONLY;
        wiki_render(p, zDisplay);
        p->state = savedState;
        blob_append(p->pOut, zClose, -1);
        break;
      }
      case TOKEN_TEXT: {
        startAutoParagraph(p);
        blob_append(p->pOut, z, n);
        break;
      }
      case TOKEN_MARKUP: {
        const char *zId;
        int iDiv;
        parseMarkup(&markup, z);

        /* Markup of the form </div id=ID> where there is a matching
        ** ID somewhere on the stack.  Exit the verbatim if were are in
        ** it.  Pop the stack up to the matching <div>.  Discard the 
        ** </div>
        */
        if( markup.iCode==MARKUP_DIV && markup.endTag &&
             (zId = markupId(&markup))!=0 &&
             (iDiv = findTagWithId(p, MARKUP_DIV, zId))>=0
        ){
          if( p->inVerbatim ){
            p->inVerbatim = 0;
            p->state = p->preVerbState;
            blob_append(p->pOut, "</pre>", 6);
          }
          while( p->nStack>iDiv+1 ) popStack(p);
          if( p->aStack[iDiv].allowWiki ){
            p->state |= ALLOW_WIKI;
          }else{
            p->state &= ~ALLOW_WIKI;
          }
          assert( p->nStack==iDiv+1 );
          p->nStack--;
        }else

        /* If within <verbatim id=ID> ignore everything other than
        ** </verbatim id=ID> and the </dev id=ID2> above.
        */           
        if( p->inVerbatim ){
          if( endVerbatim(p, &markup) ){
            p->inVerbatim = 0;
            p->state = p->preVerbState;
            blob_append(p->pOut, "</pre>", 6);
          }else{
            unparseMarkup(&markup);
            blob_append(p->pOut, "&lt;", 4);
            n = 1;
          }
        }else

        /* Render invalid markup literally.  The markup appears in the
        ** final output as plain text.
        */
        if( markup.iCode==MARKUP_INVALID ){
          unparseMarkup(&markup);
          startAutoParagraph(p);
          blob_append(p->pOut, "&lt;", 4);
          n = 1;
        }else

        /* If the markup is not font-change markup ignore it if the
        ** font-change-only flag is set.
        */
        if( (markup.iType&MUTYPE_FONT)==0 && (p->state & FONT_MARKUP_ONLY)!=0 ){
          /* Do nothing */
        }else

        /* Ignore block markup for in-line rendering.
        */
        if( inlineOnly && (markup.iType&MUTYPE_INLINE)==0 ){
          /* Do nothing */
        }else

        if( markup.iCode==MARKUP_NOWIKI ){
          if( markup.endTag ){
            p->state |= ALLOW_WIKI;
          }else{
            p->state &= ~ALLOW_WIKI;
          }
        }else

        /* Generate end-tags */
        if( markup.endTag ){
          popStackToTag(p, markup.iCode);
        }else

        /* Push <div> markup onto the stack together with the id=ID attribute.
        */
        if( markup.iCode==MARKUP_DIV ){
          pushStackWithId(p, markup.iCode, markupId(&markup),
                          (p->state & ALLOW_WIKI)!=0);
        }else

        /* Enter <verbatim> processing.  With verbatim enabled, all other
        ** markup other than the corresponding end-tag with the same ID is
        ** ignored. 
        */
        if( markup.iCode==MARKUP_VERBATIM ){
          if( markup.nAttr==1 ){
            p->zVerbatimId = markup.aAttr[0].zValue;
          }else{
            p->zVerbatimId = 0;
          }
          p->inVerbatim = 1;
          p->preVerbState = p->state;
          p->state &= ~ALLOW_WIKI;
          blob_append(p->pOut, "<pre class='verbatim'>",-1);
          p->wantAutoParagraph = 0;
        }else
        if( markup.iType==MUTYPE_LI ){
          if( backupToType(p, MUTYPE_LIST)==0 ){
            pushStack(p, MARKUP_UL);
            blob_append(p->pOut, "<ul>", 4);
          }
          pushStack(p, MARKUP_LI);
          renderMarkup(p->pOut, &markup);
        }else
        if( markup.iType==MUTYPE_TR ){
          if( backupToType(p, MUTYPE_TABLE) ){
            pushStack(p, MARKUP_TR);
            renderMarkup(p->pOut, &markup);
          }
        }else
        if( markup.iType==MUTYPE_TD ){
          if( backupToType(p, MUTYPE_TABLE|MUTYPE_TR) ){
            if( stackTopType(p)==MUTYPE_TABLE ){
              pushStack(p, MARKUP_TR);
              blob_append(p->pOut, "<tr>", 4);
            }
            pushStack(p, markup.iCode);
            renderMarkup(p->pOut, &markup);
          }
        }else
        if( markup.iType==MUTYPE_HYPERLINK ){
          popStackToTag(p, markup.iCode);
          startAutoParagraph(p);
          renderMarkup(p->pOut, &markup);
          pushStack(p, markup.iCode);
        }else
        {
          if( markup.iType==MUTYPE_FONT ){
            startAutoParagraph(p);
          }else if( markup.iType==MUTYPE_BLOCK ){
            p->wantAutoParagraph = 0;
          }
          if( (markup.iType & MUTYPE_STACK )!=0 ){
            pushStack(p, markup.iCode);
          }
          renderMarkup(p->pOut, &markup);
        }
        break;
      }
    }
    z += n;
  }
}


/*
** Transform the text in the pIn blob.  Write the results
** into the pOut blob.  The pOut blob should already be
** initialized.  The output is merely appended to pOut.
** If pOut is NULL, then the output is appended to the CGI
** reply.
*/
void wiki_convert(Blob *pIn, Blob *pOut, int flags){
  char *z;
  Renderer renderer;
  
  memset(&renderer, 0, sizeof(renderer));
  renderer.state = ALLOW_WIKI|AT_NEWLINE|AT_PARAGRAPH;
  if( flags & WIKI_NOBLOCK ){
    renderer.state |= INLINE_MARKUP_ONLY;
  }
  if( flags & WIKI_INLINE ){
    renderer.wantAutoParagraph = 0;
  }else{
    renderer.wantAutoParagraph = 1;
  }
  if( pOut ){
    renderer.pOut = pOut;
  }else{
    renderer.pOut = cgi_output_blob();
  }

  z = blob_str(pIn);
  wiki_render(&renderer, z);
  endAutoParagraph(&renderer);
  while( renderer.nStack ){
    popStack(&renderer);
  }
  blob_append(renderer.pOut, "\n", 1);
  free(renderer.aStack);
}

/*
** COMMAND: test-wiki-render
*/
void test_wiki_render(void){
  Blob in, out;
  if( g.argc!=3 ) usage("FILE");
  blob_zero(&out);
  blob_read_from_file(&in, g.argv[2]);
  wiki_convert(&in, &out, 0);
  blob_write_to_file(&out, "-");
}

/*
** Search for a <title>...</title> at the beginning of a wiki page.
** Return true (nonzero) if a title is found.  Return zero if there is
** not title.
** 
** If a title is found, initialize the pTitle blob to be the content
** of the title and initialize pTail to be the text that follows the
** title.
*/
int wiki_find_title(Blob *pIn, Blob *pTitle, Blob *pTail){
  char *z;
  int i;
  int iStart;
  z = blob_str(pIn);
  for(i=0; isspace(z[i]); i++){}
  if( z[i]!='<' ) return 0;
  i++;
  if( strncmp(&z[i],"title>", 6)!=0 ) return 0;
  iStart = i+6;
  for(i=iStart; z[i] && (z[i]!='<' || strncmp(&z[i],"</title>",8)!=0); i++){}
  if( z[i]!='<' ) return 0;
  blob_init(pTitle, &z[iStart], i-iStart);
  blob_init(pTail, &z[i+8], -1);
  return 1;
}

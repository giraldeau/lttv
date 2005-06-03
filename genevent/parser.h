#ifndef PARSER_H
#define PARSER_H

/* Extensible array container */

typedef struct _sequence {
  int size;
  int position;
  void **array;
} sequence;

void sequence_init(sequence *t);
void sequence_dispose(sequence *t);
void sequence_push(sequence *t, void *elem);
void *sequence_pop(sequence *t);


/* Hash table */

typedef struct _table {
  sequence keys;
  sequence values;
} table;

void table_init(table *t);
void table_dispose(table *t);
void table_insert(table *t, char *key, void *value);
void *table_find(table *t, char *key);
void table_insert_int(table *t, int *key, void *value);
void *table_find_int(table *t, int *key);


/* Token types */

typedef enum _token_type {
  ENDFILE,
  FORWARDSLASH,
  LANGLEBRACKET,
  RANGLEBRACKET,
  EQUAL,
  QUOTEDSTRING,
  NUMBER,
  NAME
} token_type;


/* State associated with a file being parsed */
typedef struct _parse_file {
  char *name;
  FILE * fp;
  int lineno;
  char *buffer;
  token_type type; 
  int unget;
  void (*error) (struct _parse_file *, char *);
} parse_file;

void ungetToken(parse_file * in);
char *getToken(parse_file *in);
char *getForwardslash(parse_file *in);
char *getLAnglebracket(parse_file *in);
char *getRAnglebracket(parse_file *in);
char *getQuotedString(parse_file *in);
char *getName(parse_file *in);
int   getNumber(parse_file *in);
char *getEqual(parse_file *in);
char  seekNextChar(parse_file *in);

void skipComment(parse_file * in);
void skipEOL(parse_file * in);

/* Some constants */

static const int BUFFER_SIZE = 1024;


/* Events data types */

typedef enum _data_type {
  INT,
  UINT,
	POINTER,
	LONG,
	ULONG,
	SIZE_T,
	SSIZE_T,
	OFF_T,
  FLOAT,
  STRING,
  ENUM,
  ARRAY,
  SEQUENCE,
  STRUCT,
  UNION,
  NONE
} data_type;

/* Event type descriptors */

typedef struct _type_descriptor {
  char * type_name; //used for named type
  data_type type;
  char *fmt;
  int size;
  sequence labels; // for enumeration
  sequence fields; // for structure
  struct _type_descriptor *nested_type; // for array and sequence 
} type_descriptor;


/* Fields within types */

typedef struct _field{
  char *name;
  char *description;
  type_descriptor *type;
} field;


/* Events definitions */

typedef struct _event {  
  char *name;
  char *description;
  type_descriptor *type; 
} event;

typedef struct _facility {
  char * name;
	char * capname;
  char * description;
  sequence events;
  sequence unnamed_types;
  table named_types;
} facility;

int getSize(parse_file *in);
unsigned long getTypeChecksum(unsigned long aCrc, type_descriptor * type);

void parseFacility(parse_file *in, facility * fac);
void parseEvent(parse_file *in, event *ev, sequence * unnamed_types, table * named_types);
void parseTypeDefinition(parse_file *in, sequence * unnamed_types, table * named_types);
type_descriptor *parseType(parse_file *in, type_descriptor *t, sequence * unnamed_types, table * named_types);
void parseFields(parse_file *in, type_descriptor *t, sequence * unnamed_types, table * named_types);
void checkNamedTypesImplemented(table * namedTypes);
type_descriptor * find_named_type(char *name, table * named_types);
void generateChecksum(char * facName, unsigned long * checksum, sequence * events);


/* get attributes */
char * getNameAttribute(parse_file *in);
char * getFormatAttribute(parse_file *in);
int    getSizeAttribute(parse_file *in);
int    getValueAttribute(parse_file *in);
char * getValueStrAttribute(parse_file *in);

char * getDescription(parse_file *in);


static char *intOutputTypes[] = {
  "int8_t", "int16_t", "int32_t", "int64_t", "short int", "int", "long int" };

static char *uintOutputTypes[] = {
  "uint8_t", "uint16_t", "uint32_t", "uint64_t", "unsigned short int", 
  "unsigned int", "unsigned long int" };

static char *floatOutputTypes[] = {
  "undef", "undef", "float", "double", "undef", "float", "double" };


/* Dynamic memory allocation and freeing */

void * memAlloc(int size);
char *allocAndCopy(char * str);
char *appendString(char *s, char *suffix);
void freeTypes(sequence *t);
void freeType(type_descriptor * td);
void freeEvents(sequence *t);
void freeNamedType(table * t);
void error_callback(parse_file *in, char *msg);


//checksum part
static const unsigned int crctab32[] =
{
#include "crc32.tab"
};

static inline unsigned long
partial_crc32_one(unsigned char c, unsigned long crc)
{
  return crctab32[(crc ^ c) & 0xff] ^ (crc >> 8);
}

static inline unsigned long
partial_crc32(const char *s, unsigned long crc)
{
  while (*s)
    crc = partial_crc32_one(*s++, crc);
  return crc;
}

static inline unsigned long
crc32(const char *s)
{
  return partial_crc32(s, 0xffffffff) ^ 0xffffffff;
}


#endif // PARSER_H

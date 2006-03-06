#ifndef PARSER_H
#define PARSER_H

/* Extensible array container */

typedef struct _sequence {
  int size;
  int position;
  void **array;
} sequence_t;

void sequence_init(sequence_t *t);
void sequence_dispose(sequence_t *t);
void sequence_push(sequence_t *t, void *elem);
void *sequence_pop(sequence_t *t);


/* Hash table */

typedef struct _table {
  sequence_t keys;
  sequence_t values;
} table_t;

void table_init(table_t *t);
void table_dispose(table_t *t);
void table_insert(table_t *t, char *key, void *value);
void *table_find(table_t *t, char *key);
void table_insert_int(table_t *t, int *key, void *value);
void *table_find_int(table_t *t, int *key);


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
} token_type_t;


/* State associated with a file being parsed */
typedef struct _parse_file {
  char *name;
  FILE * fp;
  int lineno;
  char *buffer;
  token_type_t type; 
  int unget;
  void (*error) (struct _parse_file *, char *);
} parse_file_t;

void ungetToken(parse_file_t * in);
char *getToken(parse_file_t *in);
char *getForwardslash(parse_file_t *in);
char *getLAnglebracket(parse_file_t *in);
char *getRAnglebracket(parse_file_t *in);
char *getQuotedString(parse_file_t *in);
char *getName(parse_file_t *in);
int   getNumber(parse_file_t *in);
char *getEqual(parse_file_t *in);
char  seekNextChar(parse_file_t *in);

void skipComment(parse_file_t * in);
void skipEOL(parse_file_t * in);

/* Some constants */

static const int BUFFER_SIZE = 1024;


/* Events data types */

typedef enum _data_type {
  INT_FIXED,
  UINT_FIXED,
	POINTER,
	CHAR,
	UCHAR,
	SHORT,
	USHORT,
  INT,
  UINT,
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
} data_type_t;

typedef struct _type_descriptor {
  char * type_name; //used for named type
  data_type_t type;
  char *fmt;
  size_t size;
  sequence_t labels; // for enumeration
  sequence_t labels_values; // for enumeration
	sequence_t labels_description;
	int	already_printed;
  sequence_t fields; // for structure, array and sequence (field_t type)
  int custom_write;  /* Should we use a custom write function ? */
	int network;	/* Is the type a in network byte order ? */
} type_descriptor_t;



/* Fields within types or events */
typedef struct _field{
  char *name;
  char *description;
  type_descriptor_t *type;
} field_t;


/* Events definitions */

typedef struct _event {  
  char *name;
  char *description;
  //type_descriptor_t *type; 
	sequence_t fields;	/* event fields */
  int  per_trace;   /* Is the event able to be logged to a specific trace ? */
  int  per_tracefile;  /* Must we log this event in a specific tracefile ? */
} event_t;

typedef struct _facility {
  char * name;
	char * capname;
	char * arch;
  char * description;
  sequence_t events;
  sequence_t unnamed_types; //FIXME : remove
  table_t named_types;
	unsigned int checksum;
	int	user;		/* Is this a userspace facility ? */
} facility_t;

int getSizeindex(unsigned int value);
unsigned long long int getSize(parse_file_t *in);
unsigned long getTypeChecksum(unsigned long aCrc, type_descriptor_t * type);

void parseFacility(parse_file_t *in, facility_t * fac);
void parseEvent(parse_file_t *in, event_t *ev, sequence_t * unnamed_types,
    table_t * named_types);
void parseTypeDefinition(parse_file_t *in,
    sequence_t * unnamed_types, table_t * named_types);
type_descriptor_t *parseType(parse_file_t *in,
    type_descriptor_t *t, sequence_t * unnamed_types, table_t * named_types);
void parseFields(parse_file_t *in, field_t *f,
    sequence_t * unnamed_types,
		table_t * named_types,
		int tag);
void checkNamedTypesImplemented(table_t * namedTypes);
type_descriptor_t * find_named_type(char *name, table_t * named_types);
void generateChecksum(char * facName,
    unsigned int * checksum, sequence_t * events);


/* get attributes */
char * getNameAttribute(parse_file_t *in);
char * getFormatAttribute(parse_file_t *in);
int    getSizeAttribute(parse_file_t *in);
int	getValueAttribute(parse_file_t *in, long long *value);

char * getDescription(parse_file_t *in);


/* Dynamic memory allocation and freeing */

void * memAlloc(int size);
char *allocAndCopy(char * str);
char *appendString(char *s, char *suffix);
void freeTypes(sequence_t *t);
void freeType(type_descriptor_t * td);
void freeEvents(sequence_t *t);
void freeNamedType(table_t * t);
void error_callback(parse_file_t *in, char *msg);


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


extern char *intOutputTypes[];

extern char *uintOutputTypes[];

extern char *floatOutputTypes[];




#endif // PARSER_H

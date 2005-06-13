
void generateFile(char * file);
void generateHfile(FILE *);
void generateCfile(FILE * fp, char * facName);
void generateTypeDefs(FILE *, char *facName);
void generateEnumEvent(FILE * fp,char * facName, int * nbEvent, unsigned long checksum);
void generateStructFunc(FILE * fp, char * facName, unsigned long checksum);
void generateLoaderfile(FILE * fp, char * facName, int nbEvent, unsigned long checksum, char *capname);
void generateEnumDefinition(FILE * fp, type_descriptor * fHead);
char * getTypeStr(type_descriptor * td);



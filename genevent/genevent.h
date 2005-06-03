
void generateFile(char * file);
void generateHfile(FILE *);
void generateTypeDefs(FILE *);
void generateEnumEvent(FILE * fp,char * facName, int * nbEvent, unsigned long checksum);
void generateStructFunc(FILE * fp, char * facName, unsigned long checksum);
void generateCfile(FILE * fp, char * facName, int nbEvent, unsigned long checksum);
void generateEnumDefinition(FILE * fp, type_descriptor * fHead);
char * getTypeStr(type_descriptor * td);



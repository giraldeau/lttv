
void generateFile(char * file);
void generateEnumEvent(FILE * fp,char * facName, int * nbEvent);
void generateStructFunc(FILE * fp, char * facName);
void generateCfile(FILE * fp, char * facName, int nbEvent, unsigned long checksum);
void generateEnumDefinition(FILE * fp, type_descriptor * fHead);
char * getTypeStr(type_descriptor * td);



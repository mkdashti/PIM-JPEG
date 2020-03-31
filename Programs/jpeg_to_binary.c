#include <stdio.h>
#include <stdlib.h>
int main(void){
	FILE * fp;
	fp = fopen("../wall-murals-lale.jpg", "r+b");
 	fseek(fp,0,SEEK_END); //go to end
	int len = ftell(fp); //get position at end (length)
	fseek(fp,0,SEEK_SET); //go to beg.
	char *buf = (char *)malloc(len); //malloc buffer
	fread(buf,len,1,fp); //read into buffer
	fclose(fp);
	for(int i = 0; i<len; i++){
		printf("%x",buf[i]);
	}
}

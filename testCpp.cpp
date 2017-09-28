#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  

#define ARR_LEGNTH(array,length){length=sizeof(array)/sizeof(array[0]);}

int main(int argc,char* argv[]){

	int array[3] = {1,2,3};
	int *p;


 	p = array;

	int length;
	ARR_LEGNTH(array,length);

printf("\n-----%d",length);

	printf("%d",sizeof(array)/sizeof(array[0]));
	
	printf("\n-----%d",p[1]);
	return 0;

}

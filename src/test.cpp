#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sstream>
using namespace std;

char * get_helper(char * path, int max_slash){
	int l = strlen(path);
	int n=1;
	int n_slash = 0;
	int i;
	char * dst;
	while(n<l){
		if(path[n]=='/')n_slash++;
		if(n_slash>=max_slash)break;
		n++;
	}
	dst = (char*)malloc((n+1)*sizeof(char));
	strncpy(dst,path,n);
	dst[n]='\0';
	return dst;
}

char * get_patient(char * path){
	return get_helper(path,1);
}

char * get_study(char * path){
	return get_helper(path,2);
}

char * get_series(char * path){
	return get_helper(path,3);
}

int main(int argc, char * argv[]){
	int a = 42;
	stringstream ss;
	for(int i=0;i<10;i++){
		ss << i;
		cout << ss.str() << endl;
		ss.str(string());
	}
	
	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>


int main(){
	char buf[1];
	int fd;
	int n;
	char *myfifo = "/tmp/myfifo";
	char quit[1]="q";
	int ch;
	mkfifo(myfifo, 0666);
	fd = open(myfifo, O_WRONLY);
	sleep(1);
	printf("Ready\n");
	printf("q-exit;r-reread\n");
	fflush(stdout);
	do{
		n = fread(buf,1,1,stdin);
		if(n>0){
			write(fd, buf, n);
		}
	}while(strncmp(buf,quit,1));
	close(fd);
	return 0;
}

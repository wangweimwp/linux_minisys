#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
int main()
{
	int fd,ret;
	fd_set ascdev_set;
	char Buf[128];
	struct timeval ti={
		.tv_sec=0,		/* seconds */
		.tv_usec=0, /* microseconds */
	};
	
	
	
	/*��ʼ��Buf*/
	strcpy(Buf,"memdev is char dev!");
	printf("BUF: %s\n",Buf);
	
	/*���豸�ļ�*/
	fd = open("/dev/ascdev0",O_RDWR|O_NOCTTY|O_NONBLOCK);
	if (fd == 0)
	{
		printf("Open memdev0 Error!\n");
		return -1;
	}
	
	/*���Buf*/
	strcpy(Buf,"Buf is NULL!");
	printf("Read BUF1: %s\n",Buf);
	
	FD_ZERO(&ascdev_set); //��ռ���
	FD_SET(fd,&ascdev_set); //����������
	ret = select(fd + 1, &ascdev_set, NULL, NULL, &ti);
	if (ret <= 0) 
	{
	    printf("select error!\n");
	    exit(1);
	}
	if (FD_ISSET(fd, &ascdev_set)) 
	/*��������*/
		read(fd,Buf,sizeof(Buf));
	
	/*�����*/
	printf("Read BUF2: %s\n",Buf);
	
	close(fd);
	
	return 0;	

}

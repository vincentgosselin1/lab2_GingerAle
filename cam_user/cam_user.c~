//cam_user.c by Vincent Gosselin, 2017.

//User program to interact with Char_driver.
#include <stdio.h>
#include <string.h>

//Required to interact with Linux core.
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//for IOCTL
#include <asm/ioctl.h>
#include "camera_ioctl.h"

//used for user interaction.
void display_welcome();
void display_menu();
int scan_input();
void execute(int choice);
char user_text_input[256];	//used to put data into driver.
char user_text_output[256]; //used to display data retrieved from driver.

//operation_done set to 1 when user is done with requesting.
int operation_done = 0;

int main()
{	
   display_welcome();
		
	while(operation_done != 1)
	{
	display_menu();
	int choice = scan_input();
	printf("You entered: %d\n", choice);
	execute(choice);
	}
   return 0;
}

void display_welcome(){
 	printf("*******************************\r\n");
	printf("*********Lab #2, ELE784********\r\n");
	printf("*******************************\r\n");
	printf("***************by**************\r\n");
	printf("*******************************\r\n");
	printf("********Vincent Gosselin*******\r\n");
	printf("*******************************\r\n");
}


void display_menu(){
	printf("Camera driver to use QuickCam® Orbit MP Logitech\r\n");
	printf("What do you want to do?\r\n");
	printf("1.  Open driver in READ\r\n");
	printf("2.  IOCTL_GET\r\n");
	printf("3.  IOCTL_SET\r\n");
	printf("4.  IOCTL_STREAMON\r\n");
	printf("5.  IOCTL_STREAMOFF\r\n");
	printf("6.  IOCTL_GRAB\r\n");
	printf("7.  IOCTL_PANTILT RIGHT\r\n");
	printf("8.  IOCTL_PANTILT LEFT\r\n");
	printf("9.  IOCTL_PANTILT UP\r\n");
	printf("10. IOCTL_PANTILT DOWN\r\n");
	printf("11. IOCTL_PANTILT_RESET\r\n");
	printf("12. Exit the program \r\n");
	
}

int scan_input(){
	int a;
	scanf("%d", &a);
	return a;
}

void execute(int choice){
	switch (choice) {
		//1 for READ
		case 1 : 
		{
			//READING
			printf("Testing file_operation READ \r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDONLY);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			int ret;//return value of every read.
			ret = read(fd, user_text_output, 1);
			if(ret>0){ 
				printf("Success in READING %d bytes\r\n",ret); 
			} else {
				printf("ERROR in READING\r\n");
			}

			//Close the file now.
			//int ret;
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 2 : 
		{
			//IOCTL_GET
			printf("Testing file_operation IOCTL_GET \r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDONLY);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;
			ret = ioctl(fd,CAMERA_IOCTL_GET,&value);//2nd parameter is the command associated, 3rd is pointer t
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 4 : 
		{
			//IOCTL_STREAMON
			printf("Testing file_operation IOCTL_STREAMON\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			//fd = open("/dev/etsele_cdev", O_RDONLY);
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0xffff;
			ret = ioctl(fd,CAMERA_IOCTL_STREAMON ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 5 : 
		{
			//IOCTL_STREAMOff
			printf("Testing file_operation IOCTL_STREAMOFF\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			//fd = open("/dev/etsele_cdev", O_RDONLY);
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0xaaaa;
			ret = ioctl(fd,CAMERA_IOCTL_STREAMOFF ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 6 : 
		{
			//IOCTL_GRAB
			printf("Testing file_operation IOCTL_GRAB\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0xaaaa;
			ret = ioctl(fd,CAMERA_IOCTL_GRAB ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
			
			
		}
		case 7 : 
		{
			//IOCTL_PANTILT RIGHT
			printf("Testing file_operation IOCTL_PANTILT RIGHT\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			//fd = open("/dev/etsele_cdev", O_RDONLY);
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0x0000ff80;
			ret = ioctl(fd,CAMERA_IOCTL_PANTILT ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 8 : 
		{
			//IOCTL_PANTILT
			printf("Testing file_operation IOCTL_PANTILT LEFT\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0x00000080;
			ret = ioctl(fd,CAMERA_IOCTL_PANTILT ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 9 : 
		{
			//IOCTL_PANTILT
			printf("Testing file_operation IOCTL_PANTILT UP\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0xff800000;
			ret = ioctl(fd,CAMERA_IOCTL_PANTILT ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 10 : 
		{
			//IOCTL_PANTILT
			printf("Testing file_operation IOCTL_PANTILT DOWN\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			unsigned long value;//4 bytes -> 32bits.
			value = 0x00800000;
			ret = ioctl(fd,CAMERA_IOCTL_PANTILT ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		case 11 : 
		{
			//IOCTL_PANTILT_RESET
			printf("Testing file_operation IOCTL_PANTILT_RESET\r\n");
			//file descriptor used for driver interaction. 
			int fd;
			//Driver is called "etsele_cdev". 
			fd = open("/dev/etsele_cdev", O_RDWR);
			if(fd<0){
				printf("ERROR in OPENNING\r\n");
			}
			//Close the file now.
			int ret;
			//direction droite 0x80,0xff,0x00,0x00
			//                 0x00,0x00,0xff,0x80
			unsigned long value;//4 bytes -> 32bits.
			value = 0x03;
			ret = ioctl(fd,CAMERA_IOCTL_PANTILT_RESET ,&value);//2nd parameter is the command associated, 3rd is pointer to unsigned long
			printf("Return value is : %d\r\n",(int)value);
			
			ret = close(fd);
			if(ret<0){
				printf("ERROR in closing\r\n");
			}
			break;
		}
		// to Exit the program
		case 12 :	
		{
			operation_done = 1;
			break;	
		}
		default : 
		{
			printf("THIS IS NOT A VALID CHOICE\r\n");
			break;
		}
	}
}




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cstring>
#include <iostream>
using namespace std;

#define BUF_SIZE 50
#define MAX_CLNT 256

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(const char * msg);
void exception_handling(const char * msg);


int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

typedef struct menu {
	int menuNumber; // menu number;
	int amount; //food amount

	// operation overloading
	bool operator==(const int &inputMenuNumber) {
		return menuNumber == inputMenuNumber;
	}
}menu;

map<int, vector<menu>*> customerMap;
map<int, vector<menu>*>::iterator customerMapIter;

vector<menu>::iterator menuIter;
int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, (socklen_t*)&clnt_adr_sz);

		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sock;
		pthread_mutex_unlock(&mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}

void * handle_clnt(void * arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char origStr[BUF_SIZE];

	str_len = read(clnt_sock, origStr, sizeof(origStr));

	if (str_len == 0)
		exception_handling("read func : recive no data!");

	char order_type = origStr[0];


	int current_process = 0;
	char customerNumber_str[10];
	int customerNumber_idx = 0;
	char menuNumber_str[10];
	int menuNumber_idx = 0;
	char amountNumber_str[10];
	int amountNumber_idx = 0;

	for (i = 1; i < str_len; i++) {
		if (origStr[i] == ' ') {
			current_process++;
		}
		else {
			switch (current_process) {
			case 0:
				customerNumber_str[customerNumber_idx++] = origStr[i];
				break;
			case 1:
				menuNumber_str[menuNumber_idx++] = origStr[i];
				break;
			case 2:
				amountNumber_str[amountNumber_idx++] = origStr[i];
				break;
			}
		}
	}

	customerNumber_str[customerNumber_idx] = '\0';
	menuNumber_str[menuNumber_idx] = '\0';
	amountNumber_str[amountNumber_idx] = '\0';

	int rcvCustomerNumber = atoi(customerNumber_str);
	int rcvMenuNumber = atoi(menuNumber_str);
	int rcvAmount = atoi(amountNumber_str);

	if ((rcvCustomerNumber == 0 || rcvMenuNumber == 0 || rcvAmount == 0) && order_type != 'c') {
		exception_handling("recive data processing : input data error!");
	}
	else {
		printf("current customer Number : %d\n", rcvCustomerNumber);
		printf("recived menu Number : %d\n", rcvMenuNumber);
		printf("recived amount : %d\n", rcvAmount);
		pthread_mutex_lock(&mutx);
		if (order_type == 'n') {
			//type = create order
			puts("create order!");
			map<int, vector<menu>*>::iterator insertIter;
			vector<menu>::iterator menuVecIter;
			if (customerMap.find(rcvCustomerNumber) == customerMap.end()) {
				//new customer!
				puts("new Customer!");
				customerMap[rcvCustomerNumber] = new vector<menu>();


				insertIter = customerMap.find(rcvCustomerNumber);

				//new customer, so, we don't have any order.
				puts("new Customer, so we don't have any order");
				menu tmp;
				tmp.menuNumber = rcvMenuNumber;
				tmp.amount = rcvAmount;
				insertIter->second->push_back(tmp);
			}
			else {
				//already exist!
				puts("already exist!");
				insertIter = customerMap.find(rcvCustomerNumber);

				//already exist customer, so, we check duplicate order.
				vector<menu>::iterator menuVecIter;
				menuVecIter = std::find(insertIter->second->begin(), insertIter->second->end(), rcvMenuNumber);


				if (menuVecIter == insertIter->second->end()) {
					//new menu!
					puts("new menu!");
					menu tmp;
					tmp.menuNumber = rcvMenuNumber;
					tmp.amount = rcvAmount;
					insertIter->second->push_back(tmp);
				}
				else {
					//already exist order!!, so, add amount!
					menuVecIter->amount += rcvAmount;
				}

			}

		}
		else if (order_type == 'd') {
			//type = delete order
			puts("type = delete order");
			map<int, vector<menu>*>::iterator deleteIter;
			if (customerMap.find(rcvCustomerNumber) == customerMap.end()) {
				//new customer!, but it is delete operation. so, it is error.
				puts("new customer!, but it is delete operation. so, it is error");
				exception_handling("delete order : wrong customer number!");
			}
			else {
				//already exist!, so, we can delete order!
				puts("already exist!, so, we can delete order!");
				deleteIter = customerMap.find(rcvCustomerNumber);

				//already exist customer, so, we check duplicate order.
				puts("already exist customer, so, we check duplicate order");
				vector<menu>::iterator menuVecIter;
				menuVecIter = std::find(deleteIter->second->begin(), deleteIter->second->end(), rcvMenuNumber);


				if (menuVecIter == deleteIter->second->end()) {
					//new menu!, but it is delete operation. so, it is error.
					puts("new menu!, but it is delete operation. so, it is error");
					exception_handling("delete order : wrong menu number!");
				}
				else {
					//already exist order!!, so, check amount! (amount must not be negative number)
					puts("already exist order!!, so, check amount! (amount must not be negative number");
					if (menuVecIter->amount - rcvAmount >= 0) {
						//OK. delete order
						puts("OK. delete order");
						menuVecIter->amount -= rcvAmount;

						if (menuVecIter->amount == 0) {
							//menu must be fully deleted.
							puts("menu must be fully deleted.");

							deleteIter->second->erase(menuVecIter);

							if (deleteIter->second->size() == 0) {
								//this customer has no order!
								puts("this customer has no order!");

								customerMap.erase(deleteIter);
							}

						}
					}
					else {
						//error, amount is wrong!!
						puts("error, amount is wrong!!");
						exception_handling("delete order : wrong amount! ");
					}
				}

			}

		}
		else if (order_type == 'c') {
			//type = confirm order
			puts("type = confirm order");

			map<int, vector<menu>*>::iterator confirmIter;
			if (customerMap.find(rcvCustomerNumber) == customerMap.end()) {
				//there is no customer like recieve customer number
				puts("there is no customer like recieve customer number!! so, it is error");
				exception_handling("confirm order : wrong customer number!");
			}
			else {
				//already exist!, so, we can delete order!
				puts("ok. we find customer.");
				confirmIter = customerMap.find(rcvCustomerNumber);

				int orderLength = confirmIter->second->size();

				std::string confirmStringTemp;

				confirmStringTemp += std::to_string(orderLength);
				confirmStringTemp += ' ';
				confirmStringTemp += std::to_string(rcvCustomerNumber);
				confirmStringTemp += ' ';


				vector<menu>::iterator confirmMenuVecIter;
				confirmMenuVecIter = confirmIter->second->begin();

				while (confirmMenuVecIter != confirmIter->second->end()) {
					confirmStringTemp += std::to_string(confirmMenuVecIter->menuNumber);
					confirmStringTemp += ' ';
					confirmStringTemp += std::to_string(confirmMenuVecIter->amount);
					confirmStringTemp += ' ';
				
				confirmMenuVecIter++;
				}


				char *confirmSendCharArray = new char[confirmStringTemp.size() + 1];
				strncpy(confirmSendCharArray, confirmStringTemp.c_str(), confirmStringTemp.size() + 1);

				printf("%s\n",  confirmSendCharArray);

				write(clnt_sock, confirmSendCharArray, confirmStringTemp.size() + 1);

				delete[] confirmSendCharArray;

			}
		}
	}



	for (i = 0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if (clnt_sock == clnt_socks[i])
		{
			while (i++<clnt_cnt - 1)
				clnt_socks[i] = clnt_socks[i + 1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);

	//test Code
	for (customerMapIter = customerMap.begin(); customerMapIter != customerMap.end(); customerMapIter++) {
		puts("----------------------------------------");
		printf("customer Number : %d\n", customerMapIter->first);
		puts("--------------order ---------------");

		for (menuIter = customerMapIter->second->begin(); menuIter != customerMapIter->second->end(); menuIter++) {
			printf("menu Number : %d ,\t amount : %d\n", menuIter->menuNumber, menuIter->amount);
		}

		puts("----------------------------------------");
	}


	return NULL;
}
void send_msg(char * msg, int len)   // send to all
{
	int i;
	pthread_mutex_lock(&mutx);
	for (i = 0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, len);
	pthread_mutex_unlock(&mutx);
}
void error_handling(const char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void exception_handling(const char *msg) {
	puts(msg);
}

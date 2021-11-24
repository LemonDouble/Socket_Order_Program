#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 50
#define NAME_SIZE 20

void * send_msg(void * arg);
void * recv_msg(void * arg);
void * display(void * arg);
void error_handling(const char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
void make_msg(char type, char* userID, char* order, char* amount);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    
    // 데이터를 보내는 쓰레드, 받는 쓰레드, 화면을 출력하는 쓰레드
    pthread_t snd_thread, rcv_thread, display_thread;
    void * thread_return;
    if(argc!=3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    

    // 1 : PF_INET : IPv4 사용
    // 2 : SOCK_STREAM : TCP 사용, (UDP : SOCK_DGRAM)
    // 3 : IPPROTO_TCP / UDP 사용 가능, 0 넣을시 2번 파라미터랑 자동으로 맞춘다.
    // 성공시 0이상, 실패시 -1 반환
    sock=socket(PF_INET, SOCK_STREAM, 0);

    // sockaddr_in 구조체 동적 할당
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    // Address Family : AF_INET (IPv4 사용)
    serv_addr.sin_family=AF_INET;
    // sin_addr.s_addr : Client의 경우, 연결하고 싶은 HOST의 주소
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    // PORT : 연결에 사용할 PORT
    serv_addr.sin_port=htons(atoi(argv[2]));

    // 0 리턴시 실패, 1 리턴시 성공
    // Parameter : (sock 함수를 통한 sock 지정 번호, serv_addr : 연결할 서버 정보, server_addr의 크기)
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
        error_handling("connect() error");
    }

    
    // Parameter : (thread ID 저장할 포인터 (pthread_t), Thread Attribute, thread가 수행할 함수, 함수에 전달할 인자)
    pthread_create(&display_thread, NULL, display, (void*)&sock);

    // Thread가 종료할때까지 기다림, parameter : (thread 식별자, 해당 thread의 return값)
    pthread_join(display_thread, &thread_return);
    close(sock);  
    return 0;
}

void * display(void * arg) // display menu
{
    // menu.txt를 읽어온다. (메뉴판 보여주기 위해)
    FILE *fmenu = fopen("menu.txt", "r");
    int menuchoice;
    char menuname[20][20], userID[10] = {'\0',}, confirm, menuId[10]={'\0', }, amount[10] = {'\0', };
    int menunum[20], price[20], i = 1, order;

    // 유저로부터 Client ID (숫자) 받아온다.
    printf("---------------------------------------\n");
    printf("Member ID (please input number): ");
    scanf("%s", userID);
    fflush(stdin);

        // 이후 menu.txt 읽으며 메뉴 출력해서 유저에게 보여준다.
        printf("---------------------------------------\n");
        printf("<Main Menu>\n");
        while( fscanf(fmenu, "%d %s %d", &menunum[i], menuname[i], &price[i]) != -1 ){
            printf("%3d %20s %7d\n", menunum[i], menuname[i], price[i]);
            i++;
        }
        
        // 이후 create order, delete order, confirm order 메뉴 보여준다.
        printf("---------------------------------------\n");
        printf("1. Create order\n");
        printf("2. Delete order\n");
        printf("3. Confirm order\n");
        printf("please select action : ");
        scanf("%d", &menuchoice);fflush(stdin);
        printf("---------------------------------------\n");

        // Create Order 케이스
        if (menuchoice == 1 ){
            printf("<Create order>\n");     
            printf("please choice menu(please input menu number) : " );
            // 어떤 음식 시킬지 입력 받는다.
            scanf("%s", menuId);
            order = atoi(menuId);
            // 몇 개나 시킬지 입력 받는다.
            printf("please input amount of food : ");
            scanf("%s", amount);
            // 주문한 음식과 개수 보여준다.
            printf("your order is %s %s\n", menuname[order], amount);
            printf("price : %d\n", price[order] * atoi(amount));
            printf("this order is right?(y/n)"); scanf("%s", &confirm); fflush(stdin);
            if( confirm == 'y' || confirm == 'Y'){
                // make_msg 이용해서 전송할 데이터 작성한 후
                make_msg('n', userID, menuId, amount);
                // 데이터 전송 
                send_msg(arg);
            }
        }
        // Delete Order 케이스
        if (menuchoice == 2){
        // 주문 개수, 주문 수량 받아서
        printf("<Delete order>\n");
        printf("please input your ordered menu number : ");
        scanf("%s", menuId);fflush(stdin);
        order = atoi(menuId);
        printf("please input amount of food : ");
        scanf("%s", amount);fflush(stdin);
        printf("please wait moment\n");
        // 마찬가지로 Format 맞춰 데이터 만든 후
        make_msg('d', userID, menuId, amount);
        // 데이터 전송
        send_msg(arg);
        }
        // Confirm Order 케이스
        if (menuchoice == 3){
        printf("<Confirm order>\n");
        // Confirm Order 경우, Order type('c') 제외하곤 menuNumber, d
        memset(menuId, '\0', sizeof(char)* 10);
        memset(amount, '\0', sizeof(char)* 10);
        make_msg('c', userID, menuId, amount);
        send_msg(arg);
        printf("please wait moment\n");
        printf("orderamount ID menunumber amount ...\n");
        recv_msg(arg);
        printf("\n");
        }
      
}
void make_msg(char type, char* userID, char* order, char* amount)
{
    // msg : char[50] Array
    // 데이터 포맷 :
    // type(1byte) + userID (10byte) + SPACE(1byte) + Menu ID (10byte) + SPACE(1byte)
    // + Amount(10byte) + BLANK(17byte)

    // type(1byte) : (n : new Menu(Create), d : delete Menu, c: confirm Menu)
    // userID : 유저 식별번호 (1,2,3..)
    // Menu ID : 메뉴 번호 (1,2,3...)
    // Amount : 수량 (1,2,3...)
    memset(msg, type, 1); strcat(msg, userID);strcat(msg," ");
    strcat(msg, order);strcat(msg," ");
    strcat(msg, amount);strcat(msg, " ");
}

void * send_msg(void * arg)   // send thread main
{
    int sock=*((int*)arg);
    // 해당 Socket으로 메세지 전송
    write(sock, msg, strlen(msg));
    // 전송 후엔 msg Array 초기화
    memset(msg, '\0', BUF_SIZE);
    return NULL;
}

void * recv_msg(void * arg)   // read thread main
{
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    int str_len = 0;
    while(!str_len)
    {

        str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);

    if(str_len==-1) 
        return (void*)-1;
        name_msg[str_len]=0;
        if(fputs(name_msg, stdout) != 0)
        {
            break;
        }
         
    }
    return NULL;
}

void error_handling(const char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

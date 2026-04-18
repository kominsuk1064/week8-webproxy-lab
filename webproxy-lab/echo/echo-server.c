// CSAPP 제공 소켓 함수
#include "../csapp.h"

// echo 함수 선언
static void echo(int connfd);

int main(int argc, char **argv)
{
    // 연결을 기다리는 소켓(가게 입구 문 번호)
    int listenfd;

    // 실제 통신용 소켓(손님 한 명과 대화하는 상담 창구 번호)
    int connfd;

    // 인자 개수 검사 (argc != 2 의미 : 프로그램 이름 + 포트 = 총 2개여야 정상)
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 전달받은 포트로 서버용 listening socket을 연다.
    listenfd = Open_listenfd(argv[1]);

    // 클라이언트를 계속 받아서 처리
    while (1) {

        // 클라이언트 연결 요청 하나를 받아들임
        connfd = Accept(listenfd, NULL, NULL);
        
        // 실제 에코 기능을 수행하는 함수 호출
        echo(connfd);

        // 연결 소켓 닫기
        Close(connfd);
    }
}

static void echo(int connfd)
{
    // 읽은 바이트 수
    size_t n;

    // 데이터 저장 버퍼
    char buf[MAXLINE];
    
    // CSAPP의 buffered I/O 구조체
    rio_t rio;

    // connfd에서 안전하게 읽기 위해 Rio 버퍼 초기화
    Rio_readinitb(&rio, connfd);

    // 클라이언트가 보낸 한 줄 읽기. 0이 나오면 클라이언트가 연결을 끊은 뜻
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        // 방금 읽은 데이터를 그대로 클라이언트에게 다시 보냄
        Rio_writen(connfd, buf, n);
    }
}

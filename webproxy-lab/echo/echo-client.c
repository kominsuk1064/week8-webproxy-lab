#include "../csapp.h"

int main(int argc, char **argv) {
    // 클라이언트 소켓의 파일 드시크립터
    int clientfd;

    // 데이터를 잠깐 저장하는 버퍼
    char buf[MAXLINE];
    rio_t rio;

    // 실행 인자가 3개가 아니면 에러 처리(프로그램 이름, host, post)
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }

    // 서버에 연결하고, 응답을 읽기 위한 Rio 버퍼 준비
    clientfd = Open_clientfd(argv[1], argv[2]);
    Rio_readinitb(&rio, clientfd);

    // 사용자 입력 한 줄을 보내고, 서버가 돌려준 한 줄을 출력
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        // 입력한 문장을 서버로 전송
        Rio_writen(clientfd, buf, strlen(buf));
        // 클라이언트가 응답을 읽음
        Rio_readlineb(&rio, buf, MAXLINE);
        // 화면에 출력
        Fputs(buf, stdout);
    }

    Close(clientfd);
    return 0;
}

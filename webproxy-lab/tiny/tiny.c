/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// 프로그램 시작점 argc는 인자 개수, argv는 인자 배열
int main(int argc, char **argv)
{
  // listenfd는 연결을 기다리는 소켓, connfd는 실제 통신하는 소켓
  int listenfd, connfd;
  // 접속한 클라이언트의 주소/포트 문자열 저장용 버퍼
  char hostname[MAXLINE], port[MAXLINE];
  // 클라이언트 주소 구조체 길이
  socklen_t clientlen;
  // 클라이언트 주소 저장용 구조체
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

// 브라우저 요청 1개를 처리하는 함수
void doit(int fd)
{
  // 정적 콘텐츠면 1, 동적 콘텐츠면 0
  int is_static;
  // 파일 상태 정보 구조체
  struct stat sbuf;
  // 요청 한줄, HTTP 메서드, URI, HTTP 버전 저장
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  // 실제 파일 경로와 CGI 인자 문자열 저장용
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  
  // HTTP 요청 첫 줄 읽기
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  // 메서드가 GET이 아닐경우 에러
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
    return;
  }

  // request line 아래 헤더들을 빈 줄 나올 때까지 읽기
  read_requesthdrs(&rio);

  // URI를 분석해서 정적/동적 구분, 파일 경로, CGI 인자 얻기
  is_static = parse_uri(uri, filename, cgiargs);

  // 파일이 없거나 접근 불가면 404 응답
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  // 정적 콘텐츠 분기
  if (is_static)
  {
    // 일반 파일이 아니거나(S_ISREG) 읽기 권한이 없으면(S_IRUSR) 에러
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // 정적 파일 전송 함수 호출(sbuf.st_size는 파일 크기)
    serve_static(fd, filename, sbuf.st_size);
  }
  // 동적 콘텐츠 분기
  else
  {
    // 일반 파일이 아니거나(S_ISREG) 읽기 권한이 없으면(S_IXUSR) 에러
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    // CGI 실행 결과를 보내는 함수 호출
    serve_dynamic(fd, filename, cgiargs);
  }
}

// 헤더 읽기 함수
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    printf("%s", buf);
    Rio_readlineb(rp, buf, MAXLINE);
  }
}

// URI 해석 함수, 반환값으로 정적/동적 여부를 알려줌
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // URI에 cgi-bin이 없으면 정적 콘텐츠
  if (!strstr(uri, "cgi-bin"))
  {
    // 정적이면 CGI 인자는 빈 문자열
    strcpy(cgiargs, "");

    // 현재 디렉터리부터 시작
    strcpy(filename, ".");
    
    // 뒤에 URI를 붙여 실제 경로로 만들기
    strcat(filename, uri);

    // URI가 '/'로 끝나면 기본 페이지 요청
    if (uri[strlen(uri) - 1] == '/')
    {
      // 기본 파일을 home.html로 붙이기
      strcat(filename, "home.html");
    }
    // 정적 콘텐츠
    return 1;
  }

  // URI에서 '?' 위치 찾기
  ptr = index(uri, '?');

  // '?'가 있으면 CGI 인자가 있다는 뜻
  if (ptr)
  {
    // '?'뒤 문자열을 CGI 인자로 복사
    strcpy(cgiargs, ptr + 1);

    // '?'자리에 문자열 끝 문자를 넣어 URI를 둘로 끊기
    *ptr = '\0';
  }
  // '?'가 없으면 CGI는 빈 문자열
  else
  {
    strcpy(cgiargs, "");
  }

  // 실제 프로그램 파일 경로를 만들 준비
  strcpy(filename, ".");

  // ./cgi-bin/adder 같은 실제 경로를 만들기
  strcat(filename, uri);

  // 동적 콘텐츠라는 뜻
  return 0;
}

// 정적 파일을 HTTP 응답으로 보내는 함수
void serve_static(int fd, char *filename, int filesize)
{
  // 열 파일의 파일 디스크립터
  int srcfd;
  int n;

  // srcp는 mmap된 파일 메모리 시작 주소, filetype은 MIME 타입 문자열, buf는 응답 헤더 버퍼
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // 파일 확장자 보고 타입 정하기
  get_filetype(filename, filetype);
  
  // HTTP 응답 헤더를 한 번에 문자열로 만들기, snprintf는 길이 제한 있는 문자열 생성 함수
  n = snprintf(buf, MAXBUF, "HTTP/1.0 200 OK\r\n" "Server: Tiny Web Server\r\n" 
    "Connection: close\r\n" "Content-length: %d\r\n" 
    "Content-type: %s\r\n\r\n", filesize, filetype);

  // 응답 헤더를 브라우저로 보내기
  Rio_writen(fd, buf, n);
  printf("Response headers:\n");
  printf("%s", buf);

  // 파일을 읽기 전용으로 열기
  srcfd = Open(filename, O_RDONLY, 0);
  
  // 파일을 메모리에 매핑
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

  // malloc
  /*srcp = (char*)Malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);*/\

  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  
  // 메모리 매핑 해제
  Munmap(srcp, filesize); 

  //free(srcp);
}

// 파일 확장조로 HTTP content-type 정하는 함수
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4"))
  {
    strcpy(filetype, "video/mp4");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }
}

// CGI 프로그램 실행 결과를 보내는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  // 응답 헤더 버퍼와 빈 인자 리스트
  char buf[MAXLINE], *emptylist[] = {NULL};

  // 상태 줄 작성
  snprintf(buf, MAXLINE, "HTTP/1.0 200 OK\r\n"); 
  /// 브라우저로 보내기
  Rio_writen(fd, buf, strlen(buf));
  // 서버 헤더 작성
  snprintf(buf, MAXLINE, "Server: Tiny Web Server\r\n");
  // 다시 보내기
  Rio_writen(fd, buf, strlen(buf));

  // 자식 프로세스 만들기
  if (Fork() == 0)
  {
    // CGI 인자를 환경변수에 넣기. CGI 프로그램은 이걸 읽음
    setenv("QUERY_STRING", cgiargs, 1);

    // 자식의 표준 출력을 브라우저 소켓으로 연결, 이제 CGI가 printf 하면 그 내용이 브라우저로 감
    Dup2(fd, STDOUT_FILENO);
    
    // 현재 자식 프로세스를 CGI 프로그램으로 바꿈
    Execve(filename, emptylist, environ);
  }
  // 부모는 자식 CGI가 끝날 때까지 기다림
  Wait(NULL);
}

// 에러 코드
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  int n;
  // buf는 HTTP에러 헤더, body는 HTML 에러 페이지 본문
  char buf[MAXLINE], body[MAXBUF];

  // HTML 에러 페이즈 본문
  n = snprintf(body, MAXBUF, "<html><title>Tiny Error</title>" "<body bgcolor=\"ffffff\">\r\n"
    "%s: %s\r\n" "<p>%s: %s\r\n" "<hr><em>The Tiny Web server</em>\r\n", errnum, shortmsg, longmsg, cause);

  // HTTP 상태 줄과 헤더
  n = snprintf(buf, MAXLINE, "HTTP/1.0 %s %s\r\n" "Content-type: text/html\r\n"
    "Content-length: %d\r\n\r\n", errnum, shortmsg, n);
  
  // 에러 응답 헤더 전송
  Rio_writen(fd, buf, n);
  // 에러 HTML 본문 전송
  Rio_writen(fd, body, strlen(body));
}

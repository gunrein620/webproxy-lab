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
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;                    // listenfd는 연결 대기용, connfd는 실제 통신용 소켓이다.
  char hostname[MAXLINE], port[MAXLINE];  // 접속한 클라이언트의 호스트명과 포트를 문자열로 저장한다.
  socklen_t clientlen;                     // clientaddr 구조체의 길이를 Accept에 넘기기 위해 사용한다.
  struct sockaddr_storage clientaddr;      // IPv4/IPv6 어떤 주소든 담을 수 있는 클라이언트 주소 구조체다.

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);  // 포트 번호를 안 주면 사용법을 출력한다.
    exit(1);                                         // 서버를 시작할 수 없으므로 종료한다.
  }

  listenfd = Open_listenfd(argv[1]);  // argv[1] 포트에서 연결 요청을 기다릴 리슨 소켓을 연다.
  while (1)
  {
    clientlen = sizeof(clientaddr);  // Accept가 주소 정보를 채워 넣을 구조체 크기를 알려준다.
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // 새 클라이언트 연결을 받아 실제 통신용 소켓 connfd를 만든다.
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);  // 클라이언트 주소를 사람이 읽기 쉬운 호스트명과 포트 문자열로 바꾼다.
    printf("Accepted connection from (%s, %s)\n", hostname, port);  // 누가 접속했는지 로그를 남긴다.
    doit(connfd);                                                    // 연결 하나에 대한 HTTP 요청 처리를 맡긴다.
    Close(connfd);                                                   // 요청 처리가 끝난 연결을 닫는다.
  }
}
/* $end tinymain */

void doit(int fd)
{
  int is_static;                                                        // 요청이 정적 콘텐츠면 1, 동적 콘텐츠면 0을 저장한다.
  struct stat sbuf;                                                     // stat 함수가 채워줄 파일 정보(종류, 권한, 크기 등)를 담는다.
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];   // HTTP 요청줄 전체와 method/uri/version 분리 결과를 저장한다.
  char filename[MAXLINE], cgiargs[MAXLINE];                             // uri를 서버 파일 경로(filename)와 CGI 인자(cgiargs)로 나눠 저장한다.
  rio_t rio;                                                            // 클라이언트 소켓 fd에서 줄 단위로 안전하게 읽기 위한 RIO 상태다.

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);                       // 현재 클라이언트 연결 fd를 RIO 버퍼와 연결한다.
  Rio_readlineb(&rio, buf, MAXLINE);             // HTTP 요청의 첫 줄(request line)을 읽는다. 예: "GET / HTTP/1.1"
  printf("Request headers:\n");                  // 서버 터미널에 요청 로그 제목을 출력한다.
  printf("%s", buf);                             // 방금 읽은 요청줄을 로그로 출력한다.
  sscanf(buf, "%s %s %s", method, uri, version); // 요청줄을 공백 기준으로 method, uri, version 세 부분으로 나눈다.
  if (strcasecmp(method, "GET"))                 // Tiny는 GET만 지원하므로, GET이 아니면 에러 응답을 보낸다.
  {
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method"); // 지원하지 않는 HTTP 메서드라는 501 에러 페이지를 클라이언트에 보낸다.
    return;                                             // 에러를 보냈으니 이 요청 처리를 끝낸다.
  }
  read_requesthdrs(&rio); // Tiny는 헤더 내용을 쓰지 않지만, 빈 줄까지 읽어서 HTTP 요청을 끝까지 소비해야 한다.

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs); // URI를 분석해서 정적/동적 여부, 실제 파일 경로, CGI 인자를 구한다.
  if (stat(filename, &sbuf) < 0)                 // 파일 시스템에서 filename이 존재하는지 확인하고, 존재하면 sbuf에 정보를 채운다.
  {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file"); // 요청한 파일이 없다는 404 에러 페이지를 클라이언트에 보낸다.
    return;                                      // 더 처리할 파일이 없으므로 요청 처리를 끝낸다.
  }

  if (is_static)
  {
    /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) // 정적 콘텐츠는 일반 파일이어야 하고, 서버가 읽을 수 있어야 한다.
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file"); // 파일은 있지만 읽을 수 없다는 403 에러 페이지를 보낸다.
      return;                                     // 읽을 수 없으므로 요청 처리를 끝낸다.
    }
    serve_static(fd, filename, sbuf.st_size); // 파일 크기를 포함한 응답 헤더를 만들고, 파일 내용을 클라이언트에 보낸다.
  }
  else
  {
    /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) // 동적 콘텐츠는 CGI 실행 파일이어야 하므로 실행 권한이 있어야 한다.
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program"); // CGI 파일은 있지만 실행할 수 없다는 403 에러 페이지를 보낸다.
      return;                                           // 실행할 수 없으므로 요청 처리를 끝낸다.
    }
    serve_dynamic(fd, filename, cgiargs); // CGI 프로그램을 실행하고, 그 출력이 클라이언트로 가게 만든다.
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body + strlen(body), "<body bgcolor=\"ffffff\">\r\n");
  sprintf(body + strlen(body), "%s: %s\r\n", errnum, shortmsg);
  sprintf(body + strlen(body), "<p>%s: %s\r\n", longmsg, cause);
  sprintf(body + strlen(body), "<hr><em>The Tiny Web server</em>\r\n");

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  {
    /* Static content */
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  {
    /* Dynamic content */
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf + strlen(buf), "Server: Tiny Web Server\r\n");
  sprintf(buf + strlen(buf), "Connection: close\r\n");
  sprintf(buf + strlen(buf), "Content-length: %d\r\n", filesize);
  sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n", filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  {
    /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    /* Redirect stdout to client */
    Execve(filename, emptylist, environ);
    /* Run CGI program */
  }
  Wait(NULL);
  /* Parent waits for and reaps child */
}

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
  char buf[MAXLINE], body[MAXBUF]; // buf는 응답 헤더 한 줄씩 만들 때 쓰고, body는 브라우저에 보여줄 HTML 본문이다.

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");                    // 에러 페이지 HTML의 시작 부분을 만든다.
  sprintf(body + strlen(body), "<body bgcolor=\"ffffff\">\r\n");       // 기존 body 뒤에 body 태그를 이어 붙인다.
  sprintf(body + strlen(body), "%s: %s\r\n", errnum, shortmsg);        // 예: "404: Not found" 같은 에러 제목을 넣는다.
  sprintf(body + strlen(body), "<p>%s: %s\r\n", longmsg, cause);       // 사람이 읽을 수 있는 자세한 에러 설명을 넣는다.
  sprintf(body + strlen(body), "<hr><em>The Tiny Web server</em>\r\n"); // 에러 페이지 하단에 서버 이름을 넣는다.

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // HTTP 응답의 상태줄을 만든다. 예: "HTTP/1.0 404 Not found"
  Rio_writen(fd, buf, strlen(buf));                     // 상태줄을 클라이언트 소켓으로 보낸다.
  sprintf(buf, "Content-type: text/html\r\n");          // 에러 본문이 HTML이라고 브라우저에 알려주는 헤더를 만든다.
  Rio_writen(fd, buf, strlen(buf));                     // Content-type 헤더를 클라이언트에 보낸다.
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // HTML 본문 길이를 알리고, 빈 줄로 헤더 끝을 표시한다.
  Rio_writen(fd, buf, strlen(buf));                             // Content-length 헤더와 빈 줄을 클라이언트에 보낸다.
  Rio_writen(fd, body, strlen(body));                           // 실제 에러 HTML 본문을 클라이언트에 보낸다.
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE]; // 요청 헤더 한 줄을 임시로 담는 버퍼다.

  Rio_readlineb(rp, buf, MAXLINE); // 요청줄 다음의 첫 번째 헤더 줄을 읽는다.
  while (strcmp(buf, "\r\n"))      // 빈 줄("\r\n")이 나올 때까지, 즉 헤더가 끝날 때까지 반복한다.
  {
    Rio_readlineb(rp, buf, MAXLINE); // 다음 헤더 줄을 읽어서 소켓 입력에서 소비한다.
    printf("%s", buf);               // 읽은 헤더를 서버 로그에 출력한다.
  }
  return; // 헤더 끝까지 읽었으므로 호출한 함수로 돌아간다.
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr; // 동적 요청에서 '?' 위치를 가리키기 위한 포인터다.

  if (!strstr(uri, "cgi-bin")) // URI에 "cgi-bin"이 없으면 정적 콘텐츠 요청으로 판단한다.
  {
    /* Static content */
    strcpy(cgiargs, "");                // 정적 콘텐츠는 CGI 인자가 없으므로 빈 문자열로 둔다.
    strcpy(filename, ".");              // 서버의 현재 디렉터리를 기준으로 파일 경로를 만들기 시작한다.
    strcat(filename, uri);              // 예: uri "/godzilla.gif" -> filename "./godzilla.gif"
    if (uri[strlen(uri) - 1] == '/')     // URI가 "/"로 끝나면 디렉터리 요청으로 보고 기본 파일을 붙인다.
      strcat(filename, "home.html");    // 예: uri "/" -> filename "./home.html"
    return 1;                           // 정적 콘텐츠임을 호출자에게 알린다.
  }
  else
  {
    /* Dynamic content */
    ptr = index(uri, '?'); // 동적 URI에서 프로그램 경로와 인자를 나누는 '?'를 찾는다.
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1); // '?' 뒤 문자열을 CGI 인자로 저장한다. 예: "a=1&b=2"
      *ptr = '\0';              // '?' 자리를 문자열 끝으로 바꿔 uri를 프로그램 경로까지만 남긴다.
    }
    else
      strcpy(cgiargs, "");      // '?'가 없으면 CGI 인자는 빈 문자열이다.
    strcpy(filename, ".");      // 서버의 현재 디렉터리를 기준으로 실행 파일 경로를 만들기 시작한다.
    strcat(filename, uri);      // 예: uri "/cgi-bin/adder" -> filename "./cgi-bin/adder"
    return 0;                   // 동적 콘텐츠임을 호출자에게 알린다.
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;                                     // 정적 파일을 열었을 때 얻는 파일 디스크립터다.
  char *srcp, filetype[MAXLINE], buf[MAXBUF];   // srcp는 mmap된 파일 시작 주소, filetype은 MIME 타입, buf는 응답 헤더 버퍼다.

  /* Send response headers to client */
  get_filetype(filename, filetype);                              // 파일 확장자를 보고 브라우저에 알려줄 MIME 타입을 정한다.
  sprintf(buf, "HTTP/1.0 200 OK\r\n");                           // 요청 성공을 알리는 HTTP 응답 상태줄을 만든다.
  sprintf(buf + strlen(buf), "Server: Tiny Web Server\r\n");     // 응답을 보낸 서버 이름을 헤더에 추가한다.
  sprintf(buf + strlen(buf), "Connection: close\r\n");           // 응답 후 연결을 닫겠다고 클라이언트에게 알린다.
  sprintf(buf + strlen(buf), "Content-length: %d\r\n", filesize); // 응답 본문이 몇 바이트인지 알려준다.
  sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n", filetype); // 본문 타입을 알려주고, 빈 줄로 헤더 끝을 표시한다.
  Rio_writen(fd, buf, strlen(buf));                              // 완성된 응답 헤더를 클라이언트 소켓으로 보낸다.
  printf("Response headers:\n");                                  // 서버 터미널에 응답 헤더 로그 제목을 출력한다.
  printf("%s", buf);                                              // 실제로 보낸 응답 헤더를 로그로 출력한다.

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);                         // 정적 파일을 읽기 전용으로 연다.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);  // 파일 내용을 메모리에 매핑해서 srcp로 접근할 수 있게 한다.
  Close(srcfd);                                                // mmap 후에는 파일 디스크립터가 필요 없으므로 닫는다.
  Rio_writen(fd, srcp, filesize);                              // 매핑된 파일 내용을 그대로 클라이언트 소켓에 보낸다.
  Munmap(srcp, filesize);                                      // 사용한 메모리 매핑을 해제한다.
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))       // 파일 이름에 .html이 들어 있으면 HTML 문서로 판단한다.
    strcpy(filetype, "text/html");     // 브라우저가 HTML로 렌더링하도록 MIME 타입을 설정한다.
  else if (strstr(filename, ".gif"))   // 파일 이름에 .gif가 들어 있으면 GIF 이미지로 판단한다.
    strcpy(filetype, "image/gif");     // 브라우저가 GIF 이미지로 처리하도록 MIME 타입을 설정한다.
  else if (strstr(filename, ".png"))   // 파일 이름에 .png가 들어 있으면 PNG 이미지로 판단한다.
    strcpy(filetype, "image/png");     // 브라우저가 PNG 이미지로 처리하도록 MIME 타입을 설정한다.
  else if (strstr(filename, ".jpg"))   // 파일 이름에 .jpg가 들어 있으면 JPEG 이미지로 판단한다.
    strcpy(filetype, "image/jpeg");    // 브라우저가 JPEG 이미지로 처리하도록 MIME 타입을 설정한다.
  else
    strcpy(filetype, "text/plain");    // 모르는 확장자는 일반 텍스트로 처리한다.
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

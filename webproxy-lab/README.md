####################################################################
# CS:APP Proxy Lab
#
# Student Source Files
####################################################################

This directory contains the files you will need for the CS:APP Proxy
Lab.

이 디렉터리에는 CS:APP Proxy Lab에 필요한 파일들이 들어 있습니다.

proxy.c
csapp.h
csapp.c
    These are starter files.  csapp.c and csapp.h are described in
    your textbook. 

    이 파일들은 시작용(starter) 파일입니다. csapp.c와 csapp.h는 교과서에서 설명합니다.

    You may make any changes you like to these files.  And you may
    create and handin any additional files you like.

    이 파일들은 원하는 대로 수정할 수 있으며, 필요한 추가 파일을 만들어 제출해도 됩니다.

    Please use `port-for-user.pl' or 'free-port.sh' to generate
    unique ports for your proxy or tiny server. 

    프록시나 tiny 서버에 쓸 고유 포트는 `port-for-user.pl` 또는 `free-port.sh`로 생성하세요.

Makefile
    This is the makefile that builds the proxy program.  Type "make"
    to build your solution, or "make clean" followed by "make" for a
    fresh build. 

    프록시 프로그램을 빌드하는 Makefile입니다. `make`로 솔루션을 빌드하고, 깨끗이 다시 빌드하려면 `make clean` 다음에 `make`를 실행하세요.

    Type "make handin" to create the tarfile that you will be handing
    in. You can modify it any way you like. Your instructor will use your
    Makefile to build your proxy from source.

    `make handin`으로 제출할 tar 파일을 만듭니다. Makefile은 원하는 대로 수정할 수 있으며, 교수님은 이 Makefile로 소스에서 프록시를 빌드합니다.

port-for-user.pl
    Generates a random port for a particular user
    usage: ./port-for-user.pl <userID>

    특정 사용자에 대해 임의의 포트를 생성합니다.
    사용법: ./port-for-user.pl <userID>

free-port.sh
    Handy script that identifies an unused TCP port that you can use
    for your proxy or tiny. 
    usage: ./free-port.sh

    프록시나 tiny에 쓸 수 있는 사용 중이 아닌 TCP 포트를 찾아 주는 편리한 스크립트입니다.
    사용법: ./free-port.sh

driver.sh
    The autograder for Basic, Concurrency, and Cache.        
    usage: ./driver.sh

    Basic, Concurrency, Cache에 대한 자동 채점기입니다.
    사용법: ./driver.sh

nop-server.py
     helper for the autograder.         

     자동 채점기용 보조 스크립트입니다.

tiny
    Tiny Web server from the CS:APP text

    CS:APP 교재의 Tiny 웹 서버입니다.

---

## 통합 번역본

**CS:APP Proxy Lab — 학생용 소스 파일**

이 디렉터리에는 CS:APP Proxy Lab에 필요한 파일들이 들어 있습니다.

**proxy.c, csapp.h, csapp.c** — 이 파일들은 시작용(starter) 파일입니다. `csapp.c`와 `csapp.h`는 교과서에서 설명합니다. 이 파일들은 원하는 대로 수정할 수 있으며, 필요한 추가 파일을 만들어 제출해도 됩니다. 프록시나 tiny 서버에 쓸 고유 포트는 `port-for-user.pl` 또는 `free-port.sh`로 생성하세요.

**Makefile** — 프록시 프로그램을 빌드하는 Makefile입니다. `make`로 솔루션을 빌드하고, 깨끗이 다시 빌드하려면 `make clean` 다음에 `make`를 실행하세요. `make handin`으로 제출할 tar 파일을 만듭니다. Makefile은 원하는 대로 수정할 수 있으며, 교수님은 이 Makefile로 소스에서 프록시를 빌드합니다.

**port-for-user.pl** — 특정 사용자에 대해 임의의 포트를 생성합니다. 사용법: `./port-for-user.pl <userID>`

**free-port.sh** — 프록시나 tiny에 쓸 수 있는 사용 중이 아닌 TCP 포트를 찾아 주는 편리한 스크립트입니다. 사용법: `./free-port.sh`

**driver.sh** — Basic, Concurrency, Cache에 대한 자동 채점기입니다. 사용법: `./driver.sh`

**nop-server.py** — 자동 채점기용 보조 스크립트입니다.

**tiny** — CS:APP 교재의 Tiny 웹 서버입니다.


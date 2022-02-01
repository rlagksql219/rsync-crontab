 >⚠️ 본 프로젝트는 명령어 관리 프로그램 `ssu_crontab`과 동기화 프로그램 `ssu_rsync`로 나뉘어집니다.
 ><br/>ssu_crontab ▶️ ssu_rsync 순서로 설명하겠습니다.
 <br/>

시스템 함수를 사용하여 새로운 명령어를 구현해봄으로써 쉘의 원리를 이해하고, 리눅스 시스템에서 제공하는 자료구조를 이용해 프로그램을 작성함으로써 시스템 프로그래밍 설계 및 응용 능력을 향상시키기 위한 목적으로 프로그램을 개발하게 되었습니다.
<br/> <br/>


# 🔍 ssu_crontab 프로젝트 개요

> 리눅스 시스템 상에서 사용자가 주기적으로 실행하고자 하는 명령어를 등록하고 실행시켜주는 프로그램입니다.


## 프로그램 기본 사항

- **사용자가 주기적으로 실행하는 명령어를 `ssu_crontab_file`에 저장 및 삭제하는 프로그램**
    - 프로그램 실행 시, 명령어를 수행 할 프롬프트 출력
    - add: `ssu_crontab_file`에 주기적으로 실행할 명령어 등록
    - remove: `ssu_crontab_file`에서 입력한 번호의 명령어 제거
    - exit: 프로그램 종료

- **주기적으로 `ssu_crontab_file`에 저장된 명령어를 실행시킬 `ssu_crond` 디몬 프로그램**
    - `ssu_crontab_file`을 읽어 주기적으로 명령어 실행
    -  `ssu_crond`를 통해 명령어가 수행되면 `ssu_crontab_log` 로그 파일에 로그를 남김
<br/>


# 🔍 설계

프로그램은 주기적으로 실행할 명령어를 등록하는 `ssu_crontab`과 등록된 명령어를 주기에 맞게 실행하는 `ssu_crond`로 나뉘어집니다. 명령어를 주기에 맞게 실행하는 `ssu_crond` 프로그램은 디몬 프로세스로 구현하였으며 프롬프트에서 실행할 수 있는 명령어는 add, remove, exit이 있습니다.


## 함수 간 Call Graph
`ssu_crontab`

![tab](https://user-images.githubusercontent.com/69866091/152023717-4bf74b04-bdde-4277-91b7-567f570bb7d2.png)

`ssu_crond`

![dd](https://user-images.githubusercontent.com/69866091/152023734-6ddc3d3f-fbe0-49a5-97fe-249c3ec30c8b.png)

  
## 함수 기능별 흐름도
`ssu_crontab`

![tab_main](https://user-images.githubusercontent.com/69866091/152024822-5a968dd3-4e65-4674-afd7-7eea1c4303f1.png)

- void cmd_add(char execution_cycle[ ], char *instructions); <br/>
`ssu_crontab_file`에 입력받은 명령어를 추가하는 함수

![add](https://user-images.githubusercontent.com/69866091/152025611-98fb8139-aa1d-490d-8531-713ba9cbacfa.png)

- void cmd_remove(char *command_number); <br/>
`ssu_crontab_file`에서 인자로 입력받은 번호의 명령어를 삭제하는 함수

![remove](https://user-images.githubusercontent.com/69866091/152025754-7dcf2e25-d069-4059-8def-70ec4bd6b3eb.png)

`ssu_crond`

![d main](https://user-images.githubusercontent.com/69866091/152025889-c2c546c9-197b-4c5e-b26b-fe146b457a25.png)
<br/>
<br/>


# 🔍 테스트 및 결과

> ✨[테스트 결과](https://www.notion.so/crontab-e5f0696188f64771831292e2e822b086)
***
<br/>

# 🔍 ssu_rsync 프로젝트 개요

> 리눅스 시스템 상에서 사용자가 동기화를 원하는 파일이나 디렉토리를 동기화하는 프로그램입니다.


## 프로그램 기본 사항

- **인자로 주어진 src 파일 혹은 디렉토리를 dst 디렉토리에 동기화**

- **동기화가 모두 완료되면 `ssu_rsync_log` 로그 파일에 기록**
<br/>


# 🔍 설계

동일한 파일에 대해서는 동기화를 진행하지 않도록 했습니다. 이때, 동일한 파일의 기준은 파일 이름, 파일 크기, 수정 시간입니다.
<br/>동기화 작업 도중에 SIGINT 시그널이 발생하면 동기화를 취소하도록 하였고, 동기화 도중에 파일을 open하지 못하도록 했습니다.


## 함수 간 Call Graph
![call](https://user-images.githubusercontent.com/69866091/152036109-a68a6221-87d2-4023-8dcf-c1ba69931e0c.png)

  
## 함수 기능별 흐름도
![main](https://user-images.githubusercontent.com/69866091/152036185-47d45b1e-cc99-4d09-a25c-332eaceb78d2.png)

- void do_rsync(char *argv[ ]); <br/>
src가 파일인 경우와 디렉토리인 경우로 나누어서 dst 디렉토리에 파일을 동기화하는 함수. 단, dst에 동일한 파일이 있는 경우에는 동기화를 진행하지 않는다.

![do_rsync](https://user-images.githubusercontent.com/69866091/152036566-2b3149e8-dbaa-485c-827b-84adef13a7c4.png)

- void remove_copy_dst(char *argv[ ]); <br/>
동기화 도중에 SIGINT 시그널을 받지 않아 정상적으로 동기화를 진행했을 경우, copy해뒀던 dst 디렉토리를 삭제하는 함수

![remove_copy](https://user-images.githubusercontent.com/69866091/152036813-6c3afca8-08af-4b5a-be99-cb3b267e6542.png)

- static void ssu_signal_handler(int signo); <br/>
동기화 도중에 SIGINT 시그널을 받았을 경우, dst 디렉토리의 내용을 전부 삭제하고 copy 해뒀던 디렉토리의 파일을 dst 디렉토리로 옮기는 함수

![signal](https://user-images.githubusercontent.com/69866091/152036983-190f14ca-cb1e-42b6-a50b-0f18e7901a57.png)
<br/>
<br/>


# 🔍 테스트 및 결과

> ✨[테스트 결과](https://www.notion.so/rsync-5b21b9edccd948cd8529b07eff17f2d3)

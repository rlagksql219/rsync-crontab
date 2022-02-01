 >⚠️ 본 프로젝트는 명령어 관리 프로그램 `ssu_crontab`과 동기화 프로그램 `ssu_rsync`로 나뉘어집니다. ssu_crontab ▶️ ssu_rsync 순서로 설명하겠습니다.
 <br/>

시스템 함수를 사용하여 새로운 명령어를 구현해봄으로써 쉘의 원리를 이해하고, 리눅스 시스템에서 제공하는 자료구조를 이용해 프로그램을 작성함으로써 시스템 프로그래밍 설계 및 응용 능력을 향상시키기 위한 목적으로 프로그램을 개발하게 되었습니다.
<br/> <br/>


# 🔍 ssu_ crontab 프로젝트 개요

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

- void cmd_add(char execution_cycle[], char *instructions); <br/>
`ssu_crontab_file`에 입력받은 명령어를 추가하는 함수

![add](https://user-images.githubusercontent.com/69866091/152025611-98fb8139-aa1d-490d-8531-713ba9cbacfa.png)

- void cmd_remove(char *command_number); <br/>
`ssu_crontab_file`에서 인자로 입력받은 번호의 명령어를 삭제하는 함수

![remove](https://user-images.githubusercontent.com/69866091/152025754-7dcf2e25-d069-4059-8def-70ec4bd6b3eb.png)

`ssu_crond`

![d main](https://user-images.githubusercontent.com/69866091/152025889-c2c546c9-197b-4c5e-b26b-fe146b457a25.png)
<br/>
<br/>


# 🥣 테스트 및 결과

> ✨[테스트 결과](https://www.notion.so/crontab-e5f0696188f64771831292e2e822b086)

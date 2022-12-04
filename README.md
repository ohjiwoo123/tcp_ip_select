# Select 함수를 활용한 Server Client 모델
0. 사용환경
- OS : Linux  
- 형상관리 : git  
- 컴파일러 : gcc
  
1. 프로그램 설명  
- 서버는 최대 1024명의 Client를 수용할 수 있다.  
- 서버는 클라이언트의 요청을 받고, 데이터를 수집하고 가공하여 다시 클라이언트에게 보내는 역할을 한다.  
- 클라이언트는 서버를 통하여 다른 클라이언트에게 메세지를 주고 받을 수 있고, 쉘 커맨드 명령어를 보낼 수 있다.  
## 서버의 콘솔창 기능
1. 유저목록보기  
2. 강퇴기능  
3. 명령어 기록보기  

![서버](https://user-images.githubusercontent.com/80387186/204316584-36d7d508-f9c8-42b4-a1ba-39d8961672c8.PNG)

## 클라이언트의 콘솔창 기능
1. 유저목록보기  
2. 메세지보내기  
3. 명령어보내기  
4. 명령어기록보기  
5. 연결종료하기  
6. Online/Offline 전환  

![리얼클라이언트](https://user-images.githubusercontent.com/80387186/204316271-e610cdc3-e86c-4197-aa3b-e2700e53181f.PNG)

## 프로젝트 주요 특징
1. 구조체 통신을 통하여 원하는 값들을 한 번에 송신, 수신 가능하게 함.  
2. 서버에서 모든 기록을 관리하고, 클라이언트는 서버를 통해 요청한 값을 받는다.  
3. Select 함수를 통해 각 FD를 효율적으로 관리한다.  
4. 특정 클라이언트가 리눅스 명령어를 다른 클라이언트에게 명령하고 그 결과 값을 얻어올 수 있다.  
5. Thread를 활용하여 계속해서 콘솔창의 출력문을 나타낸다.  

## 주요 기능 변경사항
1. 배열로 명령어 기록을 관리하다가, 링크드리스트로 명령어 기록 관리 변경.  
2. 유저목록을 보낼 때, 1024크기 버퍼로 보냈었는데 유저가 30명 정도 되면 버퍼가 OverFlow가 나서,
파일로 저장 후, while문으로 계속해서 반복해서 보내게끔 수정함.  
3. file_Path 크기를 100으로 증가하여, 현재 절대경로 길이가 큰 경우 에러가 나는 사항을 해결.  

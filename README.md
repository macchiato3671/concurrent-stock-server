# Concurrent Stock Server

다중 클라이언트 주식 거래 서버 구현 프로젝트

## 개발 환경
- Language: C
- OS: Linux
- Build: make

## 구현 내용

### Event-driven Approach (select 기반)
- select()로 다수의 클라이언트 소켓을 단일 스레드에서 관리
- pool 구조체로 클라이언트 fd 및 rio 버퍼 통합 관리

### Thread-based Approach (pthread 기반)
- sbuf(환형 버퍼) 기반 스레드 풀 구조 구현
- pthread_rwlock으로 readers-writers 동기화 처리

### 성능 비교
- 두 방식을 동일한 조건에서 직접 측정
- 클라이언트 수 10~1000 구간에서 response time, throughput 비교

### 공통
- 주식 데이터를 이진 트리 구조로 관리
- show(조회), buy(매수), sell(매도) 명령어 처리

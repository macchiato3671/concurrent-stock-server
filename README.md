# Concurrent Stock Server
다중 클라이언트 주식 거래 서버 구현 프로젝트

## 구현 내용
- select() 기반 이벤트 처리 방식
- pthread 기반 멀티스레드 방식
- readers-writers 동기화
- 이진 트리 기반 데이터 관리

## 성능 비교
두 방식을 동일 조건에서 측정하여 트레이드오프 분석

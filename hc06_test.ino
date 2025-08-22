#include <SoftwareSerial.h>

// HC-06 모듈을 아두이노의 2번(RX)과 3번(TX) 핀에 연결하세요.
// HC-06 TX -> Arduino Pin 2 (RX)
// HC-06 RX -> Arduino Pin 3 (TX)
SoftwareSerial hc06(2, 3);

String command = ""; // 웹 앱으로부터 수신한 명령어를 저장할 변수

unsigned long lastMessageTime = 0;
const long messageInterval = 5000; // 5초 간격

void setup() {
  // PC와의 시리얼 통신 시작 (아두이노 IDE의 시리얼 모니터용)
  Serial.begin(9600);
  // HC-06 블루투스 모듈과의 시리얼 통신 시작 (기본 통신 속도 9600)
  hc06.begin(9600);
  
  Serial.println("Arduino is ready.");
  hc06.println("Arduino is ready. Send a command!");
  
  command.reserve(64); // 메모리 효율을 위해 문자열 공간 미리 할당
}

void loop() {
  // HC-06로부터 데이터가 들어오는지 확인
  while (hc06.available()) {
    char c = hc06.read();
    
    // 웹 앱에서 문자열을 한 번에 보내므로, 간단히 수신하여 처리합니다.
    // 실제 환경에 맞게 개행문자(\n) 등으로 명령어 단위를 구분할 수 있습니다.
    command += c;
    delay(2); // 수신 안정성을 위한 짧은 지연
  }

  // 수신된 명령어가 있다면 처리
  if (command.length() > 0) {
    Serial.print("Received from Web: ");
    Serial.println(command);
    
    // 수신한 명령어를 다시 웹 앱으로 보내서 확인
    hc06.print("Arduino received: ");
    hc06.println(command);
    
    command = ""; // 명령어 변수 초기화
  }

  // 5초마다 웹 앱으로 하트비트 메시지 전송
  unsigned long currentTime = millis();
  if (currentTime - lastMessageTime >= messageInterval) {
    lastMessageTime = currentTime;
    hc06.println("Arduino heartbeat");
    Serial.println("Sent heartbeat to web app.");
  }
}
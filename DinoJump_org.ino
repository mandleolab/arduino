#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 디스플레이 해상도 설정
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // 리셋 핀은 사용하지 않음
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 게임 설정
#define BUTTON 2               // 점프 버튼 핀
#define DINO_Y_GROUND 20       // 공룡이 땅에 있을 때 Y 좌표
#define DINO_JUMP_HEIGHT 35    // (사용되지 않음, 참조용)
#define OBSTACLE_WIDTH 7       // 장애물 너비
#define OBSTACLE_HEIGHT 7      // 장애물 높이

// 게임 상태 변수
int dinoY = DINO_Y_GROUND;     // 공룡 현재 Y 위치
bool isJumping = false;        // 점프 중인지 여부
int jumpVelocity = 0;          // 점프 속도 (위로 올라가는 힘)
int obstacleX = SCREEN_WIDTH;  // 장애물 X 좌표
bool gameOver = false;         // 게임 오버 상태
int score = 0;                 // 점수
int gameSpeed = 3;             // 장애물 이동 속도
int lastSpeedUpScore = 0;      // 마지막 속도 증가 시점 기록

const unsigned char dino_frame1[] PROGMEM = {
  0x00, 0x0F, 0xF8, 0x00, 0x1F, 0xFE, 0x00, 0x1D, 0xFE, 0x00, 0x1F, 0xFE, 0x00, 0x1F, 0xFE, 0x00, 
  0x1F, 0xFE, 0x00, 0x1F, 0x80, 0x00, 0x1F, 0x80, 0x00, 0x1F, 0xF8, 0x80, 0x3F, 0x00, 0x80, 0xFF, 
  0x00, 0xC1, 0xFF, 0xC0, 0xE7, 0xFF, 0x40, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 
  0x7F, 0xFE, 0x00, 0x3F, 0xFE, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x07, 0x9E, 0x00, 0x07, 
  0x9E, 0x00, 0x07, 0x00, 0x00, 0x06, 0x00, 0x00, 0x07, 0x00, 0x00
};

const unsigned char dino_frame2[] PROGMEM = {
  0x00, 0x0F, 0xF8, 0x00, 0x1F, 0xFE, 0x00, 0x1D, 0xFE, 0x00, 0x1F, 0xFE, 0x00, 0x1F, 0xFE, 0x00, 
  0x1F, 0xFE, 0x00, 0x1F, 0x80, 0x00, 0x1F, 0x80, 0x00, 0x1F, 0xF8, 0x80, 0x3F, 0x00, 0x80, 0xFF, 
  0x00, 0xC1, 0xFF, 0xC0, 0xE7, 0xFF, 0x40, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 
  0x7F, 0xFE, 0x00, 0x3F, 0xFE, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x07, 0x30, 0x00, 0x07, 
  0x30, 0x00, 0x01, 0xB0, 0x00, 0x00, 0x30, 0x00, 0x00, 0x3C, 0x00
};

// 애니메이션 프레임 배열
const unsigned char* dino_frames[] = { dino_frame1, dino_frame2 };
int currentFrame = 0;                    // 현재 프레임 인덱스
unsigned long lastFrameChange = 0;      // 마지막 프레임 변경 시간
const int frameInterval = 200;          // 프레임 변경 주기 (미사용됨)

// 타이머 관련 변수 (애니메이션 & 장애물 이동 시간 제어용)
unsigned long lastDinoFrameTime = 0;
unsigned long lastObstacleMoveTime = 0;
const int dinoFrameInterval = 150;      // 공룡 애니메이션 속도
const int obstacleMoveInterval = 50;    // 장애물 이동 주기

// 함수 프로토타입 선언
void drawGame();
void displayGameOver();
void resetGame();

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);  // 버튼 입력 설정 (내부 풀업 저항)

  // OLED 디스플레이 초기화
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 초기화 실패"));
    for (;;);  // 무한 대기
  }

  display.clearDisplay();  // 화면 지우기
  display.display();       // 변경 사항 적용

  startScreen(); 
}

void startScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(45, 5);   // "OO의"
  display.println("MANDLEO");

  display.setCursor(24, 25);  // "DINO JUMP GAME"
  display.println("DINO JUMP GAME");

  display.setCursor(51, 50);  // "START"
  display.println("START");

  display.display();
  delay(3000); // 3초 대기
  display.clearDisplay();
  display.display();

}



void loop() {
  // 게임 오버 상태일 경우 재시작 대기
  if (gameOver) {
    displayGameOver();
    if (digitalRead(BUTTON) == LOW) {
      resetGame();
    }
    return;
  }

  // 버튼을 누르면 점프 시작
  if (digitalRead(BUTTON) == LOW && !isJumping) {
    isJumping = true;
    jumpVelocity = -5;  // 음수일수록 더 높이 뜀
  }

  // 점프 동작 처리 (중력 반영)
  if (isJumping) {
    dinoY += jumpVelocity;  // 위로 또는 아래로 이동
    jumpVelocity += 1;      // 중력 효과로 점차 떨어지게 함

    if (dinoY >= DINO_Y_GROUND) {
      dinoY = DINO_Y_GROUND;
      isJumping = false;    // 땅에 닿으면 점프 종료
    }
  }

  // 장애물 이동
  obstacleX -= gameSpeed;
  if (obstacleX < 0) {
    obstacleX = SCREEN_WIDTH;  // 장애물 재생성 위치
    score++;                   // 점수 증가

    // 점수가 10단위가 될 때마다 한 번만 속도 증가
    if (score % 10 == 0 && score != lastSpeedUpScore) {
      gameSpeed++;
      lastSpeedUpScore = score;
    }
  }

  // 충돌 판정 (공룡과 장애물 겹침 여부 확인)
  if (obstacleX < 27 && obstacleX + OBSTACLE_WIDTH > 13 &&
      dinoY + OBSTACLE_HEIGHT >= DINO_Y_GROUND) {
    gameOver = true;
  }

  // 게임 그리기
  drawGame();
}

void drawGame() {
  display.clearDisplay();  // 화면 초기화

  // 점수 출력
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(70, 3);
  display.print("Score: ");
  display.print(score);

  // 공룡 애니메이션 프레임 교체
  if (millis() - lastDinoFrameTime > dinoFrameInterval) {
    lastDinoFrameTime = millis();
    currentFrame = (currentFrame + 1) % 2;  // 프레임 0 ↔ 1
  }

  // 공룡 비트맵 그리기
  display.drawBitmap(10, dinoY, dino_frames[currentFrame], 23, 25, SSD1306_WHITE);

  // 장애물 이동 속도 제어 (시간 기반)
  if (millis() - lastObstacleMoveTime > obstacleMoveInterval) {
    lastObstacleMoveTime = millis();
    obstacleX -= gameSpeed;

    if (obstacleX < 0) {
      obstacleX = SCREEN_WIDTH;
      score++;

      if (score % 10 == 0 && score != lastSpeedUpScore) {
        gameSpeed++;
        lastSpeedUpScore = score;
      }
    }
  }

  // 장애물 그리기 (원형)
  int obstacleRadius = OBSTACLE_WIDTH / 2;
  display.fillCircle(obstacleX + obstacleRadius,
                     DINO_Y_GROUND + 15 + obstacleRadius,
                     obstacleRadius, SSD1306_WHITE);

  display.display();  // 화면 업데이트
}

void displayGameOver() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 최종 점수 표시
  display.setCursor(20, 5);
  display.print("Final Score: ");
  display.print(score);

  // Game Over 메시지
  display.setCursor(39, 25);
  display.print("Game Over");

  // 재시작 안내
  display.setCursor(16, 45);
  display.print("Press to Restart");

  display.display();
}

void resetGame() {
  // 모든 게임 상태 초기화
  dinoY = DINO_Y_GROUND;
  isJumping = false;
  jumpVelocity = 0;
  obstacleX = SCREEN_WIDTH;
  gameOver = false;
  score = 0;
  gameSpeed = 3;
  lastSpeedUpScore = 0;
}
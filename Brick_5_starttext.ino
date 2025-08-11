#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 3
#define BALL_SIZE 3

#define MAX_BRICKS 24

int paddleX;
int ballX = 64, ballY = 32;
int ballSpeedX = 1, ballSpeedY = 1;

int stage = 1;

struct Brick {
  int x, y;
  int width, height;
  bool visible;
};

Brick bricks[MAX_BRICKS];
int brickCount = 0;

void generateBricks() {
  brickCount = 0;
  int rows = 3;
  int brickHeight = 6;
  int minWidth = 10;
  int maxWidth = 20;

  for (int row = 0; row < rows; row++) {
    int x = 0;
    while (x < SCREEN_WIDTH - minWidth) {
      int w = random(minWidth, maxWidth + 1);
      if (x + w > SCREEN_WIDTH) break;

      bricks[brickCount].x = x;
      bricks[brickCount].y = row * (brickHeight + 2);
      bricks[brickCount].width = w;
      bricks[brickCount].height = brickHeight;
      bricks[brickCount].visible = true;

      brickCount++;
      x += w + 2;

      if (brickCount >= MAX_BRICKS) break;
    }
  }
}

void resetBall() {
  ballX = 64;
  ballY = 32;

  int speed = min(stage, 3); // 최대 속도 3까지 제한
  ballSpeedX = speed;        // 항상 오른쪽으로
  ballSpeedY = speed;        // 항상 아래로
}

void showStartScreen() {
  display.clearDisplay();
  display.setTextSize(1);  // 작은 글자
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(46, 4);
  display.println("MANDLEO");

  display.setCursor(27, 25);  // "BRICK BREAKER"
  display.println("BRICK BREAKER");

  display.setCursor(51, 50);  // "START"
  display.println("START");


  display.display();
  delay(3000); // 3초 대기
}

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED 초기화 실패"));
    while (true);
  }

  // 시작 화면 표시
  showStartScreen();

  display.clearDisplay();
  display.display();

  generateBricks();
  resetBall();
}

void loop() {
  int analogValue = analogRead(A0);
  paddleX = map(analogValue, 0, 1023, 0, SCREEN_WIDTH - PADDLE_WIDTH);

  ballX += ballSpeedX;
  ballY += ballSpeedY;

  // 벽 충돌
  if (ballX <= 0) {
    ballX = 0;
    ballSpeedX *= -1;
  }
  if (ballX >= SCREEN_WIDTH - BALL_SIZE) {
    ballX = SCREEN_WIDTH - BALL_SIZE;
    ballSpeedX *= -1;
  }
  if (ballY <= 0) {
    ballY = 0;
    ballSpeedY *= -1;
  }

  // 바닥에 닿으면 리셋
  if (ballY >= SCREEN_HEIGHT) {
    resetBall();
  }

  // 패들과 충돌
  int paddleY = SCREEN_HEIGHT - 10;
  if (ballY + BALL_SIZE >= paddleY && ballY + BALL_SIZE <= paddleY + PADDLE_HEIGHT &&
      ballX + BALL_SIZE >= paddleX && ballX <= paddleX + PADDLE_WIDTH) {
    ballSpeedY = -abs(ballSpeedY);
    int hitPos = ballX - (paddleX + PADDLE_WIDTH / 2);
    if (hitPos < 0) {
      ballSpeedX = -1;
    } else if (hitPos > 0) {
      ballSpeedX = 1;
    } else {
      ballSpeedX = 0;
    }
    ballY = paddleY - BALL_SIZE - 1;
  }

  // 벽돌 충돌
  for (int i = 0; i < brickCount; i++) {
    if (bricks[i].visible &&
        ballX + BALL_SIZE >= bricks[i].x &&
        ballX <= bricks[i].x + bricks[i].width &&
        ballY + BALL_SIZE >= bricks[i].y &&
        ballY <= bricks[i].y + bricks[i].height) {
      bricks[i].visible = false;
      ballSpeedY *= -1;
      break;
    }
  }

  // 모든 벽돌 클리어 체크
  bool allCleared = true;
  for (int i = 0; i < brickCount; i++) {
    if (bricks[i].visible) {
      allCleared = false;
      break;
    }
  }

  if (allCleared) {
    stage++;
    generateBricks();
    resetBall();
  }

  // 화면 출력
  display.clearDisplay();

  // 공
  display.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, SSD1306_WHITE);

  // 패들
  display.fillRect(paddleX, SCREEN_HEIGHT - 10, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);

  // 벽돌
  for (int i = 0; i < brickCount; i++) {
    if (bricks[i].visible) {
      display.fillRect(bricks[i].x, bricks[i].y, bricks[i].width, bricks[i].height, SSD1306_WHITE);
    }
  }

  display.display();
  delay(10);
}

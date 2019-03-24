#include <SPI.h>

/* Arduino Uno, connected to Sparkfun RG LED Matrix, using SPI
 * https://www.arduino.cc/en/Main/ArduinoBoardUnoSMD
 * https://www.sparkfun.com/products/retired/759
 *
 * LED matrix is mislabeled. "SPI Output" side, with female headers is actually the input.
 *
 * LED Connections:
 * VCC: 5v
 * Gnd: Gnd
 *
 * MOSI -> pin 11
 * SCLK -> pin 13
 * CS -> pin 10 (set below with csPin. Using p10 as output protects against its use as SS, which would cause arduino to listen for SPI)
 *    When CS is low, the LED matrix pays attention to the signal being sent on MOSI, and will display that data. Set to high when
 *    not transmitting data for the matrix (allows sending data to other SPI things attached to MOSI/SCLK).
 *
 * Updated to support two buttons, to turn left/right. These are connected to pin 2 & pin 3,
 * because those two pins are configurable for external interrupts.
 *
 * p2 & p3 are configured with a pull-up resistor, so one side of button is connected
 * directly to that pin, and the other side of the button is connected straight to GND.
 *
 * Idea: what about different levels of brightness by cycling a pixel that's "dim" on/off/on/off at
 * some duty cycle to have it be less than 100% bright. Same idea as PWM, but driving it through the
 * SPI protocol? IDK if SPI is fast enough that this is feasible without visible flickering, but I bet
 * there's some refresh rate/duty cycle combos that'd work. It could be cool to have the head of the
 * snake be brighter than the tail, and have food fade out instead of disappearing all at once.
 *
 * This would require bit-banging on the SPI bus, and the game loop timing can't be driven off
 * calls to `delay()` anymore, but `millis()` and the technique used for debouncing is probably
 * reasonable.
 */

enum PixelType {
  blank = 0,
  greenSnake = 1,
  redRat = 2,
  orangeFruit = 3
};

struct PixelState {
  PixelType type;
  int lifetime;
};

PixelState boardState[][8] = {
  {  {},{},{},{},{},{},{},{} },
  {  {},{redRat,100},{},{},{},{},{},{} },
  {  {},{},{},{},{},{},{},{} },
  {  {},{},{},{},{},{},{},{} },
  {  {},{},{},{},{},{},{},{} },
  {  {},{},{},{},{},{},{},{} },
  {  {},{},{},{},{},{},{},{} }/*,
  {  {},{},{},{},{},{},{},{} }*/
};

struct Position {
  int row;
  int col;
};

enum Direction {
  Up,
  Left,
  Down,
  Right
};


struct Snake {
  Position head;
  volatile Direction dir;
  int length;
};
  
  
const int csPin = 10;

Snake snake = {
  { 4, 5},
  Right,
  0
};

int score = 0;

const int sensorPin = A0;
const int leftButtonPin = 2;
const int rightButtonPin = 3;
volatile unsigned long lastDirChange;

void setup() {
  lastDirChange = millis();
  pinMode(csPin, OUTPUT);
  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(leftButtonPin), debounceTurnLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(rightButtonPin), debounceTurnRight, RISING);
  SPI.begin(); 
  SPI.setClockDivider(SPI_CLOCK_DIV128);
}

void loop() {
    moveSnake();
    moveRats();
    addFood();
    
    drawBoard();
    
    delay(300); // length of time between steps in the game.
}

struct Position movePosition(struct Position pos, int dir) {
  if (dir == Down) {
    pos.row = (pos.row + 1) % 7;
  } else if (dir == Up) {
    if (pos.row == 0) {
      pos.row = 6;
    } else {
      pos.row = (pos.row - 1);
    }
  } else if (dir == Right) {
    pos.col = (pos.col + 1) % 8;
  } else {
    if (pos.col == 0) {
      pos.col = 7;
    } else {
      pos.col = (pos.col - 1);
    }
  }
  
  return pos;
}


void debounceTurnLeft() {
  if (abs(millis() - lastDirChange) > 75) {
    turnLeft();
    lastDirChange = millis();
  }
}

void debounceTurnRight() {
  if (abs(millis() - lastDirChange) > 75) {
    turnRight();
    lastDirChange = millis();
  }
}

void turnLeft() {
  if (snake.dir == Down) {
      snake.dir = Right;
    } else if (snake.dir == Right) {
      snake.dir = Up;
    } else if (snake.dir == Up) {
      snake.dir = Left;
    } else {
      snake.dir = Down;
    }
}

void turnRight() {
  if (snake.dir == Down) {
      snake.dir = Left;
    } else if (snake.dir == Left) {
      snake.dir = Up;
    } else if (snake.dir == Up) {
      snake.dir = Right;
    } else {
      snake.dir = Down;
    }
}

void moveSnake() {
  snake.head = movePosition(snake.head, snake.dir);
  
  if (boardState[snake.head.row][snake.head.col].type == blank || boardState[snake.head.row][snake.head.col].type == greenSnake) {
    decrementLifetimes();
  } else {
    if (snake.length < 5) {
      ++snake.length;
    }
    ++score;
  }
  boardState[snake.head.row][snake.head.col].type = greenSnake;
  boardState[snake.head.row][snake.head.col].lifetime = snake.length;
}

void decrementLifetimes() {
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 8; ++col) {
      if (boardState[row][col].lifetime > 0) {
        --boardState[row][col].lifetime;
      } else if (boardState[row][col].lifetime == 0) {
        boardState[row][col].type = blank;
      }
    }
  }
}

void drawBoard() {
  digitalWrite(csPin,LOW);
    
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 8; ++col) {
      SPI.transfer(boardState[row][col].type);
    }
  }
  
  for (int col = 0; col < 8; ++col) {
    SPI.transfer(((score >> (7 - col)) % 2));
  }
  
  digitalWrite(csPin,HIGH);
}

void moveRats() {
  if (random(100) < 5) {
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 8; ++col) {
        if (boardState[row][col].type == redRat) {
          if (random(100) < 50) {
            Position oldPosition = {row, col};
            Position newPosition = movePosition(oldPosition, random(4));
            
            if (boardState[newPosition.row][newPosition.col].type == blank) {
              boardState[newPosition.row][newPosition.col].type = redRat;
              boardState[newPosition.row][newPosition.col].lifetime = boardState[oldPosition.row][oldPosition.col].lifetime;
              boardState[oldPosition.row][oldPosition.col].type = blank;
            }
          }
        }
      }
    }
  }
}

void addFood() {
  if (random(100) < 5) {
    int newRow = random(7);
    int newColumn = random(8);
    
    if (boardState[newRow][newColumn].type == blank) {
      boardState[newRow][newColumn].type = (random(2) == 1) ? redRat : orangeFruit;
      boardState[newRow][newColumn].lifetime = random(20,100);
    }
  }
}

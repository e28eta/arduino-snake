#include <SPI.h>

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
  Direction dir;
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
int inputState;

void setup() {
  pinMode (csPin, OUTPUT);
  SPI.begin(); 
  SPI.setClockDivider(SPI_CLOCK_DIV128);
}

void loop() {
    digitalWrite(csPin,LOW);
    
    moveSnake();
    moveRats();
    addFood();
    
    drawBoard();
    
    digitalWrite(csPin,HIGH);
    
    delay(200);
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

void moveSnake() {
  int newInputState = analogRead(sensorPin);
  
  if (newInputState > 512 && inputState < 512) {
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
  inputState = newInputState;
  
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




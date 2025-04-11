#include <TFT_eSPI.h>
#include "Wire.h"
#include "PCKeyboard.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 320
#define LINE_SPACE 2

PCKeyboard pc_kbd;
TFT_eSPI tft;

uint8_t columns;
uint8_t lines;

// Current position in the buffer
uint8_t line = 0;
uint8_t col = 0;

// Index of the first screen line
uint8_t screen_line = 0;

bool bottom;

// Buffer for the text on the screen
char *scroll_buf;

int16_t char_height = 8 * 2;
int16_t char_width = 6 * 2;

void setup() {
  // Init I2C keyboard
  Wire1.setSDA(6);                                                                                                     
  Wire1.setSCL(7);                                                                                                     
  Wire1.setClock(10000);
  Wire1.begin();  
  pc_kbd.begin(PCKEYBOARD_DEFAULT_ADDR, &Wire1);                                                                                         
  
  // Init display
  tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(1);
  tft.fillScreen(TFT_BLACK); 
  // Disable text wrapping
  tft.setTextWrap(false, false); 


  // Allocate text buffer for scrolling
  //int16_t char_height = tft.fontHeight();
  //int16_t char_width = tft.textWidth(" "); // assuming monospace font
  lines = SCREEN_HEIGHT / (LINE_SPACE + char_height);
  columns = SCREEN_WIDTH / char_width;
  scroll_buf = (char*)malloc(sizeof(char)*lines*columns);
  memset(scroll_buf, 0, sizeof(char)*lines*columns);
  bottom = false;
}

void drawBuffer() {
  tft.fillScreen(TFT_BLACK);

  uint8_t cur = screen_line;
  for (int l = 0; l < lines; l++) {
    for (int c = 0; c < columns; c++) {
      char sym =  scroll_buf[c + cur*columns];
      if (sym == 0) {
        break;
      }

      int32_t pos_x = c*char_width;
      int32_t pos_y = l*(char_height+LINE_SPACE);
      tft.drawChar(pos_x, pos_y, sym, TFT_GREEN/*text*/, TFT_BLACK/*background*/, 2/*font*/);
    }

    cur = (cur+1) % lines;
  }
}

void clear_line(int line) {
  for (int c = 0; c < columns; c++) {
    scroll_buf[columns*line +c] = 0;
  }
}

void write(char c) { 
  // Position on the screen
  int32_t pos_x, pos_y;
  pos_x = col*char_width;
  if (bottom) {
    pos_y = (lines-1)*(char_height+LINE_SPACE);  
  } else {
    pos_y = line*(char_height+LINE_SPACE);
  }

  // New line
  if (c == '\n') {
    line++;
    col = 0;
  } else {
    scroll_buf[columns*line + col] = c;
    col++;
  }
 
  // Line overflow 
  if (col >= columns) {
    line++;
    col=0;
  }

  // Screen overflow
  if (line >= lines) {
    bottom = true; // set the flag on the first overflow
    // Wrap line buffer around 
    line = line % lines;
  }
  
  // Do we need to scroll?
  if (line == screen_line && bottom) {
    // Moving the first screen line and redraw the screen
    clear_line(line);
    screen_line = (screen_line+1) % lines;
    drawBuffer();

  } else {
    // Print char on the screen
    if (c == '\n') {
      return;
    }
    tft.drawChar(pos_x, pos_y, c, TFT_GREEN/*text*/, TFT_BLACK/*background*/, 2/*font*/);
  }
}

// Printing number with scrolling
void print_byte(char i) {
  char buf[4]; // 3 symbols + \0
  char *str = buf + 3; //end of array

  *str = '\0';

   do {
    char c = i % 10;
    i /= 10;

    *--str = c + '0';
  } while(i);

  while (*str) {
    write(*str);
    str++;
  }
}


void loop() {
  if (pc_kbd.keyCount() > 0) {
    PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
    if (event.state ==  PCKeyboard::StatePress) {
        if (event.key < 0x7f) {
          write(event.key);
        } else {
          write('<');
          print_byte(event.key);
          write('>');
        }
    }
  }
}
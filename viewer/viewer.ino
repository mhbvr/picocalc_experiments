#include <TFT_eSPI.h>
#include "Wire.h"
#include "PCKeyboard.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 320

// Keycodes
#define ARROW_UP    0xB5
#define ARROW_DOWN  0xB6
#define ARROW_LEFT  0xB4
#define ARROW_RIGHT 0xB7
#define HOME        0xD2
#define END         0xD5

PCKeyboard pc_kbd;
TFT_eSPI tft;

// Viewer

struct line {
  char *buffer;
  int size;
};

struct viewer {
  // Array of lines
  line *line_buff;

  // Total number of lines in the array
  uint32_t lines;

  // Top left screen position
  uint32_t screen_x;
  uint32_t screen_y;

  // Relative cursor position
  uint8_t cursor_x;
  uint8_t cursor_y;

  // Font dimentions in pixels
  int16_t char_height;
  int16_t char_width;

  // Total number of lines and colunms on the screen
  uint8_t screen_lines;
  uint8_t screen_columns;

  // Distance between lines in pixels
  uint16_t line_dist;

  // Font for TFT_eSPI
  int font_id;

  uint32_t text_color;
  uint32_t bg_color;
};

struct line test_lines[] = {
  {"", 0}, //0
  {"aaaaabbbbbcccccdddddeeeeefffffggggg", 35}, //1
  {"aaaaabbbbbcccccdddddeeeeefffffgggggaaaaabbbbbcccccdddddeeeeefffffggggg", 70}, //2
  {"a", 1}, //3
  {"aaaaabbbbbCCCCC", 15}, //4
  {"aaaaaBBBBB", 10}, //5
  {"aaaa", 4}, //6
  {"", 0}, //7
  {"", 0}, //8
  {"aaaaabbbbbcccccdddddeeeeefffffgggggaaaaa", 40}, //9
  {"aaaaabbbbbcccccdddddeeeeefffffgggggaaaaaaaaaabbbbbcccccdddddeeeeefffffgggggaaaaa", 80}, //10
  {"aaaaabbbbbcccccdddddeeeeefffff", 30}, //11
  {"aaaaabbbbbcccccdddddeeeeefffffggggg", 35}, //12
  {"aaaaaDDDDD", 10}, //13
  {"", 0}, //14
  {"aaaaabbbbbcccccdddddeeeeefffffggggg", 35}, //15
  {"aaaaabbbbbcccccddddde", 21}, //16
  {"aaa", 3}, //17
  {"aa", 2}, //18
  {"a", 1}, //19
  {"aaaaabbbbbcccccdddddeeeeefffffgggggaaaaa", 40}, //20
  {"aaaaaDDDDD", 10}, //21
  {"aaaaabbbbbcccccdddddeeeeefffffgggggaaaaa", 40}, //22
  {"aaaaabbbbbcccccdddddeeeeefffff", 30}, //23
  {"", 0}, //24
  {"aaaaabbbbbcccccdddddeeeeefffff", 30}, //25
  {"aaaa", 4}, //26
  {"aaaaabbbbbcccccdddddeeeeefffffggggg", 35}, //27
  {"end", 3}, //28
};

struct viewer V;

void initViewer() {
  V.line_buff = test_lines;
  V.lines = 29;
  V.screen_x = 0;
  V.screen_y = 0;
  V.cursor_x = 0;
  V.cursor_y = 0;
  
  V.char_height = 8 * 2;
  V.char_width = 6 * 2;
  V.line_dist = 2;

  V.screen_lines = SCREEN_HEIGHT / (V.line_dist + V.char_height);
  V.screen_columns = SCREEN_WIDTH / V.char_width;
  V.font_id = 2;
  V.text_color = TFT_GREEN;
  V.bg_color = TFT_BLACK;
  refresh_screen();
}

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

  initViewer();
  cursor(true);
}

void refresh_screen() {
  tft.fillScreen(V.bg_color);

  for (int l = 0; l < V.screen_lines; l++) {
    
    if (V.screen_y + l >= V.lines) {
      // All lines printed
      return;
    }

    if (&V.line_buff[V.screen_y + l] == NULL || V.line_buff[V.screen_y + l].size <= V.screen_x) {
      // Empty line or line out of the screen
      continue;
    }

    int line_len = V.line_buff[V.screen_y + l].size;

    for (int c = 0; c < V.screen_columns; c++) {
      if (V.screen_x + c >= line_len) {
        break;
      }

      char sym = (V.line_buff[V.screen_y + l].buffer)[V.screen_x + c];

      int32_t pos_x = c * V.char_width;
      int32_t pos_y = l * (V.char_height + V.line_dist);
      tft.drawChar(pos_x, pos_y, sym, V.text_color, V.bg_color, V.font_id);
    }
  }
}

void cursor(bool show) {
  // Current symbol under cursor. Space by default.
  char c = ' ';

  line *current_line = NULL;
  if (V.screen_y + V.cursor_y < V.lines) {
    current_line = &(V.line_buff[V.screen_y + V.cursor_y]);
  }

  if (current_line != NULL && V.cursor_x + V.screen_x < current_line->size) {
    c = current_line->buffer[V.cursor_x + V.screen_x];
  }

  int32_t pos_x = V.cursor_x * V.char_width;
  int32_t pos_y = V.cursor_y * (V.char_height + V.line_dist);

  // Default value to clear cursor;
  uint32_t bg_color = V.bg_color;
  uint32_t text_color = V.text_color;

  if (show) {
    // revert colors for the cursor
    bg_color = V.text_color;
    text_color = V.bg_color;    
  }

  tft.drawChar(pos_x, pos_y, c, text_color, bg_color, V.font_id);
}

uint32_t line_lenght() {
  if (V.screen_y + V.cursor_y < V.lines /*&& V.line_buff[V.screen_y + V.cursor_y] != NULL*/) {
    return V.line_buff[V.screen_y + V.cursor_y].size;
  }
  return 0;
}

bool fix_cursor_position() {
  // Check cursor position. It should be inside on in the end of the current line.
  uint32_t line_len = line_lenght();
  
  if (V.screen_x + V.cursor_x <= line_len) {
    // Cursor is inside or at the end of line
    return false;
  }

  if (V.screen_x < line_len) {
    // Line at least partially on the screen. Move cursor to the end of line.
    V.cursor_x = line_len - V.screen_x;
    return false;
  }

  // Current line is not on the screen. Move screen and cursor. Screen needs refresh.
  if (line_len >= V.screen_columns) {
    // Long line show end of the line on the screen with cursor in the end
    V.screen_x = line_len - V.screen_columns + 1;
    V.cursor_x = V.screen_columns - 1; // Cursor in the end of line
  } else {
    // Short line, show it fully
    V.screen_x = 0;
    V.cursor_x = line_len; 
  }
  
  return true;
}

void move_cursor(char c) {
  bool need_refresh = false;
  bool more_refresh = false;
  uint32_t line_len = line_lenght();
  cursor(false); // Hide cursor

  switch(c) {
  case ARROW_UP:
    if (V.cursor_y + V.screen_y == 0) {
      // Already on top
      break;
    }

    if (V.cursor_y > 0) {
      V.cursor_y--;
    } else if (V.screen_y > 0) {
      // Scroll up
      V.screen_y--;
      need_refresh = true;
    }

    more_refresh = fix_cursor_position();
    need_refresh = need_refresh || more_refresh;

    break;
  case ARROW_DOWN:
    if (V.cursor_y + V.screen_y == V.lines - 1) {
      // At the bottom
      break;
    }

    if (V.cursor_y < V.screen_lines - 1) {
      V.cursor_y++;
    } else if (V.lines > V.screen_y + V.screen_lines) {
      // Scroll down
      V.screen_y++;
      need_refresh = true;
    }
    
    more_refresh = fix_cursor_position();
    need_refresh = need_refresh || more_refresh;

    break;
  case ARROW_LEFT:
    if (V.cursor_x > 0) {
      V.cursor_x--;
      break;
    }

    // Scroll left
    if (V.screen_x > 0) {
      V.screen_x--;
      need_refresh = true;
    }
    break;
  case ARROW_RIGHT:
    // Checking the length of the current line
    
   
    // Checking end of line
    if (V.screen_x + V.cursor_x >= line_len) {
      break;
    }

    if (V.cursor_x < V.screen_columns - 1) {
      V.cursor_x++;
      break;
    }

    // Scroll right 
    if (line_len >= V.screen_x + V.screen_columns) {
      V.screen_x++;
      need_refresh = true;
    }
    break;
  case HOME:
    need_refresh = (V.screen_x != 0);
    V.screen_x = 0;
    V.cursor_x = 0;
    break;
  case END:
    if (V.screen_x + V.screen_columns <= line_len) {
      // Move screen to the end of line
      V.screen_x = line_len - V.screen_columns + 1;
      need_refresh = true;
    }

    V.cursor_x = line_len - V.screen_x;
  }

  if (need_refresh) {
    refresh_screen();
  }
  cursor(true);
}

void loop() {
  if (pc_kbd.keyCount() > 0) {
    PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
    if (event.state ==  PCKeyboard::StatePress) {
      switch(event.key) {
      case ARROW_UP:
      case ARROW_DOWN:
      case ARROW_LEFT:
      case ARROW_RIGHT:
      case HOME:
      case END:
          move_cursor(event.key);
          break;
      }
    }
  }
}
#include <Arduino.h>
#line 1 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
#include <TFT_eSPI.h>
#include "Wire.h"
#include "PCKeyboard.h"

// Screen size in pixels
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 320

// Keycodes

// Cursor movement
#define ARROW_UP    0xB5
#define ARROW_DOWN  0xB6
#define ARROW_LEFT  0xB4
#define ARROW_RIGHT 0xB7
#define HOME        0xD2
#define END         0xD5

// Edit
#define DELETE      0xD4
#define BACKSPACE   0x08 
#define TAB         0x09
#define ENTER       0x0A

// Managment
#define INSERT      0xD1
#define BREAK       0xD0
#define ESC         0xB1
#define F1          0x81
#define F3          0x83
#define F2          0x82
#define F4          0x84
#define F5          0x85
#define F6          0x86
#define F7          0x87
#define F8          0x88
#define F9          0x89
#define F10         0x90

PCKeyboard pc_kbd;
TFT_eSPI tft;

// Viewer

struct line {
  char *buffer;
  int size;
  int cap;
};

struct viewer {
  // Array of lines
  line **line_buff;

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

char *text = 
  "\n"
  "aaaaabbbbbcccccdddddeeeeefffffggggg\n"
  "aaaaabbbbbcccccdddddeeeeefffffgggggaaaaabbbbbcccccdddddeeeeefffffggggg\n"
  "a\n"
  "aaaaabbbbbCCCCC\n"
  "aaaaaBBBBB\n"
  "aaaa\n"
  "\n"
  "\n"
  "aaaaabbbbbcccccdddddeeeeefffffgggggaaaaa\n"
  "aaaaabbbbbcccccdddddeeeeefffffgggggaaaaaaaaaabbbbbcccccdddddeeeeefffffgggggaaaaa\n"
  "aaaaabbbbbcccccdddddeeeeefffff\n"
  "aaaaabbbbbcccccdddddeeeeefffffggggg\n"
  "aaaaaDDDDD\n"
  "\n"
  "aaaaabbbbbcccccdddddeeeeefffffggggg\n"
  "aaaaabbbbbcccccddddde\n"
  "aaa\n"
  "aa\n"
  "a\n"
  "aaaaabbbbbcccccdddddeeeeefffffgggggaaaaa\n"
  "aaaaaDDDDD\n"
  "aaaaabbbbbcccccdddddeeeeefffffgggggaaaaa\n"
  "aaaaabbbbbcccccdddddeeeeefffff\n"
  "\n"
  "aaaaabbbbbcccccdddddeeeeefffff\n"
  "aaaa\n"
  "aaaaabbbbbcccccdddddeeeeefffffggggg\n"
  "end\n";

struct viewer V;


#line 118 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
line* new_line(char *buf, int len);
#line 134 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void read_str(char *str);
#line 156 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void initViewer();
#line 180 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void setup();
#line 201 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void refresh_screen();
#line 232 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void update_line();
#line 246 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void cursor(bool show);
#line 275 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
uint32_t line_length();
#line 282 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
bool fix_cursor_position();
#line 311 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
bool move_cursor(char c);
#line 402 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
bool move_to_x(int x);
#line 414 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void add_char(char c);
#line 446 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void handle_enter();
#line 482 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void merge_lines(int first);
#line 509 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void handle_delete();
#line 530 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void handle_backspace();
#line 569 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
bool is_printable(char c);
#line 575 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
void loop();
#line 118 "/home/mih/projects/picocalc_experiments/editor/editor.ino"
line *new_line(char *buf, int len) {
  line *res = (line *)malloc(sizeof(line));

  res->size = len;
  res->cap = len;
  res->buffer = NULL;

  if (len > 0) {
    res->buffer = (char *)malloc(sizeof(char)*len);
    // TODO: do we need to check correctness of the data?
    memcpy(res->buffer, buf, len);   
  }
  
  return res;
}

void read_str(char *str) {
  char *start = str;
  char *end = str;

  while (*end != 0) {
    if (*end == '\n') {
      V.lines++;
      V.line_buff = (line**)realloc(V.line_buff, sizeof(line *) * V.lines);
      V.line_buff[V.lines-1] = new_line(start, end - start);
      start = ++end;
      continue;
    }
    end++;
  }  

  if (end - start > 0) {
    V.lines++;
    V.line_buff = (line**)realloc(V.line_buff, sizeof(line *) * V.lines);
    V.line_buff[V.lines-1] = new_line(start, end - start);
  }
}

void initViewer() {
  V.line_buff = NULL;
  V.lines = 0;
  V.screen_x = 0;
  V.screen_y = 0;
  V.cursor_x = 0;
  V.cursor_y = 0;
  
  V.char_height = 8;
  V.char_width = 6;
  V.line_dist = 1;

  V.screen_lines = SCREEN_HEIGHT / (V.line_dist + V.char_height);
  V.screen_columns = SCREEN_WIDTH / V.char_width;
  V.font_id = 1;
  V.text_color = TFT_GREEN;
  V.bg_color = TFT_BLACK;

  // For test only
  read_str(text);

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

    if (V.line_buff[V.screen_y + l] == NULL || V.line_buff[V.screen_y + l]->size <= V.screen_x) {
      // Empty line or line out of the screen
      continue;
    }

    int line_len = V.line_buff[V.screen_y + l]->size;

    for (int c = 0; c < V.screen_columns; c++) {
      if (V.screen_x + c >= line_len) {
        break;
      }

      char sym = V.line_buff[V.screen_y + l]->buffer[V.screen_x + c];

      int32_t pos_x = c * V.char_width;
      int32_t pos_y = l * (V.char_height + V.line_dist);
      tft.drawChar(pos_x, pos_y, sym, V.text_color, V.bg_color, V.font_id);
    }
  }
}

void update_line() {
  int len = line_length();
  
  for (int i = 0; i < V.screen_columns; i++) {
    int32_t x = i * V.char_width;
    int32_t y = V.cursor_y * (V.char_height + V.line_dist);
    tft.fillRect(x, y, V.char_width, V.char_height, V.bg_color);
    if (V.screen_x + i < len) {
      char sym = (V.line_buff[V.screen_y + V.cursor_y]->buffer)[V.screen_x + i];
      tft.drawChar(x, y, sym, V.text_color, V.bg_color, V.font_id);
    }
  }  
}

void cursor(bool show) {
  // Current symbol under cursor. Space by default.
  char c = ' ';

  line *current_line = NULL;
  if (V.screen_y + V.cursor_y < V.lines) {
    current_line = V.line_buff[V.screen_y + V.cursor_y];
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

uint32_t line_length() {
  if (V.screen_y + V.cursor_y < V.lines && V.line_buff[V.screen_y + V.cursor_y] != NULL) {
    return V.line_buff[V.screen_y + V.cursor_y]->size;
  }
  return 0;
}

bool fix_cursor_position() {
  // Check cursor position. It should be inside on in the end of the current line.
  uint32_t line_len = line_length();
  
  if (V.screen_x + V.cursor_x <= line_len) {
    // Cursor is inside or at the end of line
    return false;
  }

  if (V.screen_x < line_len || (line_len + V.screen_x) == 0) {
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

bool move_cursor(char c) {
  bool need_refresh = false;
  bool more_refresh = false;
  uint32_t line_len = line_length();
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
    need_refresh = move_to_x(line_len);
  }

  if (need_refresh) {
    refresh_screen();
  }
  cursor(true);
  return need_refresh;
}

bool move_to_x(int x) {
  bool need_refresh = false;
  if (V.screen_x + V.screen_columns <= x) {
      // Move screen right until desired position will visible
      V.screen_x = x - V.screen_columns + 1;
      need_refresh = true;
  }

  V.cursor_x = x - V.screen_x;
  return need_refresh;
}

void add_char(char c) {
  line *cur_line = V.line_buff[V.screen_y + V.cursor_y];
  if (cur_line == NULL) {
    return; // should not happen
  } 

  cur_line->size++;
  cur_line->cap++;
  cur_line->buffer = (char *)realloc(cur_line->buffer, cur_line->size);

  // Shift all symbols right by 1 from the cursor
  for (int i = cur_line->size - 1; i > V.screen_x + V.cursor_x; i--) {
    cur_line->buffer[i] = cur_line->buffer[i-1];
  }

  cur_line->buffer[V.screen_x + V.cursor_x] = c;

  // Moving cursor  
  if (V.cursor_x != V.screen_columns - 1) {
      // Cursor not at right border
      V.cursor_x++;
      update_line();
      cursor(true);
      return;
  }

  // Oh, need to scroll right
  V.screen_x++;
  refresh_screen();
  cursor(true);
}

void handle_enter() {
  // Add new line to the list
  V.lines++;
  V.line_buff = (line **)realloc(V.line_buff, sizeof(line*)*V.lines);
  for (int i = V.lines - 1; i > V.screen_y + V.cursor_y + 1; i--) {
    V.line_buff[i] = V.line_buff[i-1];
  }

  // Create new line from the data from the current one
  line *current_line = V.line_buff[V.screen_y + V.cursor_y];
  V.line_buff[V.screen_y+V.cursor_y+1] = new_line(&current_line->buffer[V.screen_x + V.cursor_x], current_line->size - V.screen_x - V.cursor_x);

  // Shrink current line
  // TODO: reallocate in the case of significant difference between cap and size
  current_line->size = V.screen_x + V.cursor_x;
  current_line->cap = V.screen_x + V.cursor_x;
  current_line->buffer = (char *)realloc(current_line->buffer, current_line->cap);

  // Move cursor to the beginning of the line
  V.cursor_x = 0;

  if (V.screen_x > 0) {
    // Need scroll left to the beginning
    V.screen_x = 0;
  }

  if (V.cursor_y < V.screen_lines - 1) {
    V.cursor_y++;
  } else {
    V.screen_y++;
  }

  refresh_screen();
  cursor(true);
}

void merge_lines(int first) {
  if (first < 0 || first > V.lines - 2) {
    return;
  }
  
  line *current = V.line_buff[first];
  line *next = V.line_buff[first+1];

  current->buffer = (char *)realloc(current->buffer, sizeof(char)*(current->size + next->size));
  for (int i = 0; i < next->size; i++) {
    current->buffer[current->size+i] = next->buffer[i];
  }
  current->size = current->size + next->size;
  current->cap = current->size;


  for (int i = first+1; i < V.lines-1; i++) {
      V.line_buff[i] = V.line_buff[i+1];
  }

  free(next->buffer);
  free(next);

  V.lines--;
  V.line_buff = (line **)realloc(V.line_buff, sizeof(line*)*V.lines);
}

void handle_delete() {
  if (V.screen_x + V.cursor_x == line_length()) {
    // End line. Merge next line in this one.
    merge_lines(V.screen_y + V.cursor_y);
    refresh_screen();
    cursor(true);
    return;
  }

  // delete symbol on cursor
  line *current_line = V.line_buff[V.screen_y + V.cursor_y];
  for (int i = V.screen_x + V.cursor_x; i < current_line->size - 1; i++) {
    current_line->buffer[i] = current_line->buffer[i+1];
  }
  current_line->size--;

  // TODO: reallocate when it will be big difference between size and cap
  update_line();
  cursor(true);
}

void handle_backspace() {
  if (V.screen_x + V.cursor_x == 0) {
    if (V.screen_y + V.cursor_y == 0) return;

    // Begining of the line. Merge prev line with this one.
    int prev_line_size = V.line_buff[V.screen_y + V.cursor_y - 1]->size;
    merge_lines(V.screen_y + V.cursor_y - 1);

    if (V.cursor_y > 0) {
      V.cursor_y--;
    } else {
      V.screen_y--;  
    }

    move_to_x(prev_line_size);
    
    refresh_screen();  
    cursor(true);
    return;
  }

  // delete symbol on cursor
  line *current_line = V.line_buff[V.screen_y + V.cursor_y];
  for (int i = V.screen_x + V.cursor_x-1; i < current_line->size - 1; i++) {
    current_line->buffer[i] = current_line->buffer[i+1];
  }
  current_line->size--;

  // TODO: reallocate when it will be big difference between size and cap
  
  if (V.cursor_x > 0) {
    V.cursor_x--;
  } else {
    V.screen_x--;
  }
  refresh_screen();
  cursor(true);
}

bool is_printable(char c) {
  if (c >= 0x20 && c <= 0x7e) return true;
  return false;
}


void loop() {
  if (pc_kbd.keyCount() > 0) {
    PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
    if (event.state ==  PCKeyboard::StatePress) {
      if (is_printable(event.key)) {
        add_char(event.key);
        return;
      }

      switch(event.key) {
      case ARROW_UP:
      case ARROW_DOWN:
      case ARROW_LEFT:
      case ARROW_RIGHT:
      case HOME:
      case END:
        move_cursor(event.key);
        break;
      case DELETE:
        handle_delete();
        break;
      case BACKSPACE:
        handle_backspace();
        break;
      case ENTER:
        handle_enter();
        break;
      }
    }
  }
}

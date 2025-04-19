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

// REPL

struct line {
  char *buffer;
  int size;
  line *next;
  line *prev;
};

// Add empty line to after the curr one.
line *new_line(line *curr) {
  line *res = (line *)malloc(sizeof(line))
  memset(res, 0, sizeof(line));
  res->prev = curr;
  res->next = curr->next;
  curr->next = res;
  if (curr->next != NULL) {
    curr->next->prev = res;
  }
  return res;
}

void del_line(line *curr) {
  if (curr->prev != NULL) curr->prev->next = curr->next;
  if (curr->next != NULL) curr->next->prev = curr->prev;
  free(curr->buffer);
  free(curr);
}

line *first(line *curr) {
  while (curr->prev != NULL) curr = curr->prev;
  return curr;
}

line *last(line *curr) {
  while (curr->next != NULL) curr = curr->next;
  return curr;
}

void add_char(line *l, char c, int pos) {
  if (pos < 0 || pos > l->size) return;
  l->size++;
  l->buffer = (char *)realloc(l->buffer, sizeof(char) * l->size);
  
  // Shift symbols right by 1 starting from pos
  for (int i = l->size - 1; i > pos; i--) {
    l->buffer[i] = l->buffer[i-1];
  }

  l->buffer[pos] = c;
}

void del_char(line *l, int pos) {
  if (pos < 0 || pos >= l->size) return;

  for (int i = pos; i < l->size - 1; i++) {
    l->buffer[i] = l->buffer[i+1];
  }

  l->size--;
  l->buffer = (char *)realloc(l->buffer, sizeof(char) * l->size);
}

char get_char(int pos) {
  if (pos < 0 || pos >= l->size) return 0;
  return l->buffer[pos];
}

struct repl {
  // Current line in the linked list
  line *l
  
  // Max number of lines in the array
  uint32_t max_lines;

  char *prompt;
  uint16_t prompt_len;

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
  uint32_t bracket_color;
  uint32_t completion_color;
};


struct repl R;

void initViewer() {
  V.line_buff = NULL;
  V.lines = 0;
  V.screen_x = 0;
  V.screen_y = 0;
  V.cursor_x = 0;
  V.cursor_y = 0;
  
  V.char_height = 8 * 2;
  V.char_width = 6 * 2;
  V.line_dist = 1;
  V.line_width = 2;
  V.border = 0;
  V.space = 2;

  V.screen_lines = (SCREEN_HEIGHT - V.line_width - V.line_dist) / (V.line_dist + V.char_height) - 1;
  V.screen_columns = (SCREEN_WIDTH - 2 * (V.border + V.line_width + V.space)) / V.char_width;
  V.font_id = 1;
  V.text_color = TFT_GREEN;
  V.bg_color = TFT_BLACK
  V.bracket_color = TFT_ORANGE;
  V.competion_color = TFT_LIGHTGRAY;

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
#include <TFT_eSPI.h>
#include "Wire.h"
#include "PCKeyboard.h"

// Screen size in pixels
#define SCREEN_WIDTH  320
#define SCREEN_ILI9488_HEIGHT 480
#define SCREEN_HEIGHT 320

// Display commands

// Vertical Scrolling Start Address
#define VSCRSADD 0x37
// Vertical Scrolling Definition
#define VSCRDEF 0x33

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

// REPL

PCKeyboard pc_kbd;
TFT_eSPI tft;

struct line {
  char *buffer;
  int size;
  line *next;
  line *prev;
};

// Add empty line to after the curr one.
struct line *new_line(line *curr) {
  line *res = (line *)malloc(sizeof(line));
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

char get_char(line *l, int pos) {
  if (pos < 0 || pos >= l->size) return 0;
  return l->buffer[pos];
}

struct screen {
  // Total number of character lines and colunms on the screen
  uint8_t lines;
  uint8_t columns;

  // Distance between lines in pixels
  uint16_t line_dist;

  // Font for TFT_eSPI
  int font_id;

  // Font dimentions in pixels
  int16_t char_height;
  int16_t char_width;

  // Colors
  uint32_t text_color;
  uint32_t bg_color;

  // Current text position in characters;
  uint8_t x;
  uint8_t y;

  // Number of the top pixelline. 
  uint16_t y_start;

  // Number of scrolls
  uint16_t scrolls;

  // Hardware scrolling parameters
  uint16_t scroll_area;
  uint16_t bfa;
};

struct screen *S;

struct screen *new_screen() {
  screen *s = (screen *)malloc(sizeof(screen));
  memset(s, 0, sizeof(screen));

  s->char_height = 14;
  s->char_width = 8;
  s->line_dist = 0;
  s->font_id = 1;
  
  s->text_color = TFT_GREEN;
  s->bg_color = TFT_BLACK;

  s->lines = SCREEN_HEIGHT / (s->char_height + s->line_dist);
  s->columns = SCREEN_WIDTH / s->char_width;

  // Init display
  tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(1);
  tft.setTextWrap(false, false);     // Disable text wrapping

  tft.setFreeFont(GOHU_FONT);
 // tft.setTextDatum(TL_DATUM);

  // Configure vertical scrolling
  s->scroll_area = s->lines * (s->char_height + s->line_dist);
  //s->scroll_area = SCREEN_HEIGHT;
  s->bfa = SCREEN_ILI9488_HEIGHT - s->scroll_area;
  tft.writecommand(VSCRDEF);         // Vertical scroll
  tft.writedata(0);                  // Top Fixed Area line count
  tft.writedata(0);
  tft.writedata((s->scroll_area)>>8);    // Vertical Scrolling Area line count
  tft.writedata((s->scroll_area) & 0xFF);
  tft.writedata((s->bfa)>>8);                  // Bottom Fixed Area line count
  tft.writedata((s->bfa) & 0xFF);

  tft.fillScreen(s->bg_color);
  return s;
}

int set_position(struct screen *s, int x, int y) {
  if (x < 0 || x >= s->columns) return -1;
  if (y < 0 || y >= s->lines) return -1;

  s->x = x;
  s->y = y;
  return 0;
}

void set_colors(struct screen *s, uint32_t text, uint32_t bg) {
  s->text_color = text;
  s->bg_color = bg;
}

void scroll(screen *s) {
  // Clear top line
  tft.fillRect(0, s->y_start, SCREEN_WIDTH, s->char_height + s->line_dist, s->bg_color);
  
  s->y_start = s->y_start + s->char_height + s->line_dist;

  // Wrapping arond the scroll
  // Will be more complecated with non zero TFA and BFA
  if (s->y_start >= s->scroll_area) s->y_start = s->y_start - s->scroll_area;
  //if (s->y_start >= s->scroll_area) s->y_start = 0;

  tft.writecommand(VSCRSADD); // Vertical scrolling pointer
  tft.writedata((s->y_start)>>8);
  tft.writedata((s->y_start) & 0xFF);
}

void next_line(struct screen *s, bool scrl) {
  if (s->y == s->lines - 1) {
    if (!scrl) return; 
    scroll(s);
    s->scrolls++;
  } else {
    s->y++;   
  }

  s->x = 0;
}

void write(struct screen *s, char c, bool move, bool scrl) {
  int pos_x = s->x * s->char_width;
  int pos_y = (s->y_start + s->y * (s->char_height + s->line_dist)) % s->scroll_area;
  // Need to make a background 
  tft.fillRect(pos_x, pos_y, s->char_width, s->char_height, s->bg_color);
  tft.drawChar(pos_x, pos_y, c, s->text_color, s->bg_color, 1);

  if (!move) return;

  if (s->x < s->columns - 1) {
    s->x++;
    return;
  }

  next_line(s, scrl);
}

void cursor(struct screen *s, char c, bool show) {
  uint32_t text_color = s->text_color;
  uint32_t bg_color = s->bg_color;

  if (show) {
    set_colors(s, s->bg_color, s->text_color);
  }

  write(s, c, false, false);

  if (show) {
    set_colors(s, text_color, bg_color);
  }
}

struct repl {
  line * current_line;
  uint8_t num_lines;

  // Cursor position in the line
  int pos;

  // Position of the line start on the screen.
  // start_y should be updated after scrolling.
  uint8_t start_x;
  uint8_t start_y;
};


void setup() {
  // Init I2C keyboard
  Wire1.setSDA(6);                                                                                                     
  Wire1.setSCL(7);                                                                                                     
  Wire1.setClock(10000);
  Wire1.begin();  
  pc_kbd.begin(PCKEYBOARD_DEFAULT_ADDR, &Wire1);
  S = new_screen();
  cursor(S, ' ', true);
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
        // cursor(S, ' ', false);
        write(S, event.key, true, true);
        cursor(S, ' ', true);
        return;
      }

      switch(event.key) {
      case ENTER:
        cursor(S, ' ', false);
        next_line(S, true);
        cursor(S, ' ', true);
        break;
      case ESC:
        scroll(S);
      }
    }
  }
}
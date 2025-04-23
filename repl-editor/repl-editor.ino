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


// Linked list of text lines.
// Used to store history of inputs

struct line {
  char *buffer;
  int size;
  line *next;
  line *prev;
};

// Initialize new line with (zeroed).
struct line *init_line() {
  struct line *l = (struct line *)malloc(sizeof(struct line));
  memset(l, 0, sizeof(struct line));
  return l;
}

void free_line(struct line *l) {
  free(l->buffer);
  free(l);
}

// Insert line to the list after current and return poiter to the new line
struct line *insert_line(struct line *l, struct line *n) {
  if (l == NULL) return n;

  n->next = l->next;
  n->prev = l;

  if (l->next != NULL) l->next->prev = n;
  l->next = n;
  
  return n;
}

// Remove line from the list.
struct line *extract_line(struct line *l) {
  struct line *res = NULL;
  if (l->prev != NULL) {
    l->prev->next = l->next;
    res = l->prev;
  }
  if (l->next != NULL) {
    l->next->prev = l->prev;
    res = l->next;
  }
  l->next = NULL;
  l->prev = NULL;
  return res;
}

void delete_line(struct line *l) {
  extract_line(l);
  free_line(l);
}

struct line *first(struct line *l) {
  while (l->prev != NULL) l = l->prev;
  return l;
}

struct line *last(struct line *l) {
  while (l->next != NULL) l = l->next;
  return l;
}

void add_char(struct line *l, char c, int pos) {
  if (pos < 0 || pos > l->size) return;
  l->size++;
  l->buffer = (char *)realloc(l->buffer, sizeof(char) * l->size);
  
  // Shift symbols right by 1 starting from pos
  for (int i = l->size - 1; i > pos; i--) {
    l->buffer[i] = l->buffer[i-1];
  }
 
  l->buffer[pos] = c;
}

void del_char(struct line *l, int pos) {
  if (pos < 0 || pos >= l->size) return;

  for (int i = pos; i < l->size - 1; i++) {
    l->buffer[i] = l->buffer[i+1];
  }

  l->size--;
  l->buffer = (char *)realloc(l->buffer, sizeof(char) * l->size);
}


// Text based screen interface

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

  // Current cursor position (column, lines) from the top left corner
  uint8_t x;
  uint8_t y;

  // The number of the top pixelline. It is used to implement hardware
  // vertical scrolling
  uint16_t y_start;

  // Number of scrolls
  uint16_t scrolls;

  // Hardware scrolling parameters
  uint16_t scroll_area; // Number of pixellines
  uint16_t bfa;         // Bottom fixed area in pixellines
};

struct screen *S;

struct screen *init_screen() {
  struct screen *s = (screen *)malloc(sizeof(screen));
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

void scroll_up(screen *s) {
  // Clear top line
  tft.fillRect(0, s->y_start, SCREEN_WIDTH, s->char_height + s->line_dist, s->bg_color);
  
  s->y_start = s->y_start + s->char_height + s->line_dist;

  // Wrapping around the scroll
  // Will be more complicated with non zero TFA and BFA
  if (s->y_start >= s->scroll_area) s->y_start = s->y_start - s->scroll_area;
  //if (s->y_start >= s->scroll_area) s->y_start = 0;

  tft.writecommand(VSCRSADD); // Vertical scrolling pointer
  tft.writedata((s->y_start)>>8);
  tft.writedata((s->y_start) & 0xFF);
}

// Move cursor to the prev line, 
// TODO: support scroll down
int move_prev_line(struct screen *s, int allow_scroll) {
  if (s->y > 0) { //
    s->y--;
  }
  return 0;
}

// Move cursor to the next line, scroll screen up if necessary and allowed.
int move_next_line(struct screen *s, int allow_scroll) {
  int scroll = 0;
  if (s->y == s->lines - 1) { //
    if (!allow_scroll) return 0; 
    scroll_up(s);
    scroll = -1;
  } else {
    s->y++;
  }
  return scroll;
}

// Move cursor right with horizontal wrapping.
// Scroll screen up if necessary and allowed.
int move_right(struct screen *s, int allow_scroll) {
  if (s->x < s->columns - 1) {
    s->x++;
    return 0;
  }

  s->x = 0;
  return move_next_line(s, allow_scroll);
}

// Move cursor left with horizontal wrapping.
// TODO: Support scroll down. 
int move_left (struct screen *s, int allow_scroll /* Not implemented */) {
  if (s->x > 0) {
    s->x--;
    return 0;
  }

  s->x = s->columns - 1;
  return move_prev_line(s, allow_scroll);
}

// Write character at the cursor position. If move != 0 move cursor left.
// Go to the next line at the end of the line when allow_scroll != 0.
// It returns change of Y coodinate in the case of scrolling. Y < 0 means scrolling up.
int write(struct screen *s, char c, int move, int allow_scroll) {
  int pos_x = s->x * s->char_width;
  int pos_y = (s->y_start + s->y * (s->char_height + s->line_dist)) % s->scroll_area;
  // Need to make a background as drawChar with GFX font ignores background color.
  tft.fillRect(pos_x, pos_y, s->char_width, s->char_height, s->bg_color);
  tft.drawChar(pos_x, pos_y, c, s->text_color, s->bg_color, 1);

  if (!move) return 0;

  return move_right(s, allow_scroll);
}

struct repl {
  struct line *line;
  uint8_t num_lines;

  // Cursor position in the line
  int pos;

  // Position of the line start on the screen.
  // start_y should be updated after scrolling.
  uint8_t start_x;
  uint8_t start_y;
};

struct repl *init_repl() {
  struct repl *r = (struct repl*)malloc(sizeof(struct repl));
  if (r == NULL) return r;
  memset(r, 0, sizeof(struct repl));

  return r;
}

// Cursor is an inverse symbol. 
void draw_cursor(struct screen *s, char c, bool show) {
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

// Used to print data on screen.
// Return space char when line is NULL or position out of bound.
char get_c(struct repl *r) {
  if (r->line == NULL || r->pos >= r->line->size || r->pos < 0) return ' ';
  return r->line->buffer[r->pos];
}

// Returns X position on the screen of the current text symbol.
int get_x(struct repl *r, struct screen *s) {
  return (r->start_x + r->pos) % s->columns;
}

// Returns Y position on the screen of the current text symbol.
int get_y(struct repl *r, struct screen *s) {
  return r->start_y + (r->start_x + r->pos) / s->columns;
}

// Length of the current line
int get_len(struct repl *r) {
  if (r->line == NULL) return 0;
  return r->line->size;
}

void move_cursor(struct repl *r, struct screen *s, char key) {
  draw_cursor(s, get_c(r), 0);
  
  switch(key) {
  case ARROW_LEFT:
    if (r->pos == 0) break;  // line start
    r->pos--;
    move_left(s, 0);  // Scrolling not allowed
    break;
  case ARROW_RIGHT:
    if (r->pos == get_len(r)) break; // position after line end
    r->pos++;
    move_right(s, 0); // Scrolling not allowed
    break;
  case HOME:
    r->pos = 0; 
    set_position(s, r->start_x, r->start_y);
    break;
  case END:
    r->pos = get_len(r);
    set_position(s, get_x(r, s), get_y(r, s));
    break;
  }

  draw_cursor(s, get_c(r), 1);
}

void print_line(struct repl *r, struct screen *s, int size) {
  int x = get_x(r, s);
  int y = get_y(r, s);
  int pos = r->pos;
  int dy = 0;
  while (r->pos < size) {
    dy += write(s, get_c(r), 1, 1); 
    r->pos++;
  }
  // Adjust start of line because of scrolling
  r->start_y += dy;
  // Return cursor to the original positions
  r->pos = pos;
  set_position(s, x, y + dy);
}

void show_line(struct repl *r, struct screen *s, struct line *l) {
  // Go to the line start
  r->pos = 0;
  set_position(s, r->start_x, r->start_y);
  int old_len = get_len(r);
  r->line = l;
  // Clear the scroll counter to track line start
  // and print new line
  print_line(r, s, max(get_len(r), old_len) + 1);
}

bool is_printable(char c) {
  if (c >= 0x20 && c <= 0x7e) return true;
  return false;
}

char * read_line(struct repl *r, struct screen *s) {
  if (r->line == NULL || r->line->size > 0) {
    // First run on non empty prev input
    r->line = insert_line(r->line, init_line());
  }

  write(s, '>', 1, 1);
  write(s, ' ', 1, 1);
  draw_cursor(s, ' ', 1);
  
  r->pos = 0;
  r->start_x = s->x;
  r->start_y = s->y;

  while (1) {
    if (pc_kbd.keyCount() > 0) {
      PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
      if (event.state ==  PCKeyboard::StatePress) {
        
        if (is_printable(event.key)) {
          draw_cursor(s, get_c(r), 0);
          add_char(r->line, event.key, r->pos);
          print_line(r, s, get_len(r) + 1);
          r->pos++;
          move_right(s, 1);
          draw_cursor(s, get_c(r), 1);
          continue;
        }

        switch(event.key) {
        case ARROW_UP:
          // TODO: Prev input from history
          if (r->line->prev == NULL) break;
          show_line(r, s, r->line->prev);
          draw_cursor(s, get_c(r), 1);
          break;
        case ARROW_DOWN:
          // TODO: Next input from history
          if (r->line->next == NULL) break;
          show_line(r, s, r->line->next);
          draw_cursor(s, get_c(r), 1);
          break;
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case HOME:
        case END:
          move_cursor(r, s, event.key);
          break;
        case DELETE:
          // Delete symbol in the cursor position
          if (r->pos == get_len(r)) continue;
          del_char(r->line, r->pos);
          print_line(r, s, get_len(r) + 1);
          draw_cursor(s, get_c(r), 1); 
          break;
        case BACKSPACE:
          // Delete symbol before cursor
          if (r->pos == 0) continue;
          if (r->pos == get_len(r)) draw_cursor(s, ' ', 0);
          r->pos--;
          move_left(s, 1);
          del_char(r->line, r->pos);
          print_line(r, s, get_len(r) + 1);
          draw_cursor(s, get_c(r), 1); 
          break;
        case ENTER:
          // TODO: 0 at the end, history.
          // Clear cursor
          draw_cursor(s, get_c(r), 0);

          // Move to the end of line
          r->pos = get_len(r);
          set_position(s, get_x(r, s), get_y(r, s));

          // Move to the end of the next line
          move_next_line(s, 1);
          s->x = 0;

          if (r->line->next != NULL) {
            // Edited history line
            struct line *current = r->line;
            r->line = extract_line(current);
            r->line = last(r->line);
            r->line = insert_line(r->line, current);
          }
          return r->line->buffer;
          break;
        }
      }
    }
  }
}

struct repl *R;


void setup() {
  // Init I2C keyboard
  Wire1.setSDA(6);                                                                                                     
  Wire1.setSCL(7);                                                                                                     
  Wire1.setClock(10000);
  Wire1.begin();  
  pc_kbd.begin(PCKEYBOARD_DEFAULT_ADDR, &Wire1);
  S = init_screen();
  R = init_repl();
}

void loop() {
  read_line(R, S);
}
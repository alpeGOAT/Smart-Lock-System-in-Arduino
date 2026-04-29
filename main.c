#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <string.h>


#define LCD_RS PH4
#define LCD_E PH5
#define LCD_D4 PH6
#define LCD_D5 PB4
#define LCD_D6 PB5
#define LCD_D7 PB6

#define R1 PC2
#define R2 PC0
#define R3 PG2
#define R4 PG0

#define C1 PL4
#define C2 PL2
#define C3 PL0
#define C4 PB2

#define ROW_COUNT 4
#define COL_COUNT 4

#define BUZZER PE5

char system_password[5] = "1035";
uint8_t attempts = 0;

void keypad_init() {
  DDRG |= (1 << R3) | (1 << R4);
  DDRC |= (1 << R1) | (1 << R2);

  PORTC |= (1 << R1) | (1 << R2);
  PORTG |= (1 << R3) | (1 << R4);

  DDRL &= ~((1 << C1) | (1 << C2) | (1 << C3));
  DDRB &= ~(1 << C4);

  PORTL |= (1 << C1) | (1 << C2) | (1 << C3);
  PORTB |= (1 << C4);
}

char keypad_getkeys() {
  char keypad[ROW_COUNT][COL_COUNT] = { {'1','2','3','A'},
                                        {'4','5','6','B'},
                                        {'7','8','9','C'},
                                        {'*','0','#','D'}
};

  for(uint16_t row=0; row<ROW_COUNT; row++) {
    PORTC |= (1 << R1) | (1 << R2);
    PORTG |= (1 << R3) | (1 << R4);

    if (row == 0) PORTC &= ~(1 << R1);
    if (row == 1) PORTC &= ~(1 << R2);
    if (row == 2) PORTG &= ~(1 << R3);
    if (row == 3) PORTG &= ~(1 << R4);

    _delay_us(5);

    if(!(PINL & (1 << C1))) return keypad[row][0];
    if(!(PINL & (1 << C2))) return keypad[row][1];
    if(!(PINL & (1 << C3))) return keypad[row][2];
    if(!(PINB & (1 << C4))) return keypad[row][3];

  }
  return 0;                                    
}

void lcd_pulse_E() {
  PORTH |= (1 << LCD_E);
  _delay_us(5);
  PORTH &= ~(1 << LCD_E);
  _delay_us(100);
}

void lcd_send_nibble(uint16_t data) {
  PORTH &= ~(1 << LCD_D4);
  PORTB &= ~(1 << LCD_D5);
  PORTB &= ~(1 << LCD_D6);
  PORTB &= ~(1 << LCD_D7);

  if(data & 0x01) PORTH |= (1 << LCD_D4);
  if(data & 0x02) PORTB |= (1 << LCD_D5);
  if(data & 0x04) PORTB |= (1 << LCD_D6);
  if(data & 0x08) PORTB |= (1 << LCD_D7);

  lcd_pulse_E();
}

void lcd_command(uint16_t cmd) {
  PORTH &= ~(1 << LCD_RS);
  lcd_send_nibble(cmd >> 4);
  lcd_send_nibble(cmd & 0x0F);
  _delay_ms(2);
}

void lcd_data(uint16_t data) {
   PORTH |= (1 << LCD_RS);
   lcd_send_nibble(data >> 4);
   lcd_send_nibble(data & 0X0F);
   _delay_ms(2);
}

void lcd_init() {
  DDRH |= (1 << LCD_RS) | (1 << LCD_E) | (1 << LCD_D4);
  DDRB |= (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7);

  _delay_ms(20);

  lcd_send_nibble(0x03);
  _delay_ms(5);
  lcd_send_nibble(0x03);
  _delay_us(150);
  lcd_send_nibble(0x03);

  lcd_send_nibble(0x02);

lcd_command(0x28);
lcd_command(0x0C);
lcd_command(0x06);
lcd_command(0x01);

_delay_ms(2);
}

void lcd_print(char *str) {
  while(*str) {
    lcd_data(*str++);
  }
}

void buzzer_init() {
  DDRE |= (1 << BUZZER);
  PORTE &= ~(1 << BUZZER);
}

void buzzer_on() {
  PORTE |= (1 << BUZZER);
}

void buzzer_off() {
  PORTE &= ~(1 << BUZZER);
}

void buzzer_tone(uint16_t duration_ms) {
  for (uint16_t i = 0; i < duration_ms; i++) {
    PORTE |= (1 << BUZZER);
    _delay_us(500);
    PORTE &= ~(1 << BUZZER);
    _delay_us(500);
  }
}

void buzzer_beep() {
  buzzer_tone(150);
}

void buzzer_alarm() {
  for(uint16_t i=0; i<4; i++) {
    buzzer_tone(300);
    _delay_ms(300);
  }
}

void buzzer_beep_when_access_granted() {
  buzzer_tone(230);
}

void show_welcome() {
  lcd_command(0x01);
  lcd_print("Hello User,");
  lcd_command(0xC0);
  lcd_print("Enter  password ");
}

void read_password(char input[5]) {
  uint16_t j=0;

  while(j < 4){
    char key = keypad_getkeys();

    if(key >= '0' && key <= '9') {
      input[j] = key;
      lcd_data('*');
      buzzer_beep();
       
       j++;

       while(keypad_getkeys() != 0);
       _delay_ms(150);
    }
  }
  
  input[4] = '\0';
}

uint16_t check_password(char input[]) {
  if(strcmp(input,system_password) == 0) {
    return 1;
  } 
  else {
    return 0;
  }
}

void access_granted() {
  attempts = 0;

  lcd_command(0x01);
  lcd_print("Access Granted!");

  buzzer_beep_when_access_granted();
  _delay_ms(1000);

  lcd_command(0x01);
  lcd_print("Enter Password");

  show_welcome();

}

void access_denied() {
  attempts++;

  lcd_command(0x01);
  lcd_print("Wrong Password");

  if(attempts >= 3) {
    lcd_command(0x01);
    lcd_print("ALARM!");
    buzzer_alarm();
    _delay_ms(1500);
    attempts = 0;
  }

  show_welcome();
}

void change_password() {
  char new_system_password[5];

  lcd_command(0x01);
  lcd_print("New Password");

  lcd_command(0xc0);
  read_password(new_system_password);

  for(uint16_t k=0; k<5; k++) {
    system_password[k] = new_system_password[k];
  }

  lcd_command(0x01);
  lcd_print("Password Saved");
  buzzer_beep();
  _delay_ms(1000);

  show_welcome();
}

int main(void) {
  char input[5];

  lcd_init();
  keypad_init();
  buzzer_init();

  show_welcome();

  while(1) {
    read_password(input);

    if(check_password(input) == 1) {
      access_granted();

      lcd_command(0x01);
      lcd_print("A:Change Pass");
      lcd_command(0xC0);
      lcd_print("B:Lock");

      char choice = 0;

      while(choice == 0) {
        choice = keypad_getkeys();
      }

      while(keypad_getkeys() != 0);
        _delay_ms(150);
      
      if(choice == 'A') {
        change_password();
      }
      else if(choice == 'B') {
        show_welcome();
      }
      else {
        show_welcome();
      }
      }
      else {
        access_denied();
      }
      }
    }
  


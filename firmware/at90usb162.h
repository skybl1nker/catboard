#define LED_CONFIG	(DDRD	|= (1<<6))
#define LED_OFF		(PORTD	&= ~(1<<6))
#define LED_ON		(PORTD	|= (1<<6))

#define LED_RED_CONFIG	LED_CONFIG
#define LED_RED_OFF		LED_OFF
#define LED_RED_ON		LED_ON

#define LED_BLUE_CONFIG	LED_CONFIG
#define LED_BLUE_OFF	LED_ON
#define LED_BLUE_ON		LED_OFF

uint8_t *const col_pin_sw2 = _PIND;
const uint8_t col_bit_sw2 = (1<<7);

// Init ports
void init_ports(void) {
	DDRB	= 0x00; DDRC	= 0b11110100;	DDRD	= 0x80;
	PORTB	= 0xFF; PORTC	= 0b11110100; 	PORTD	= 0xFF;
}

// Pins keyboard matrix (have to properly initialize ports)
uint8_t *const	row_port[ROWS]	= { _PORTC,	_PORTC,	_PORTC,	_PORTC,	_PORTC};
const uint8_t	row_bit[ROWS]	= { (1<<7),	(1<<6),	(1<<5),	(1<<4),	(1<<2)};
uint8_t *const	col_pin[COLS]	= {_PIND, _PIND, _PIND, _PIND, _PIND, _PIND, _PINB,	_PINB,	_PINB, _PINB, _PINB, _PINB};
const uint8_t	col_bit[COLS]	= { (1<<6),	(1<<5),	(1<<3),	(1<<2),	(1<<1),	(1<<0),	(1<<7),	(1<<6),	(1<<5),	(1<<4),	(1<<1),	(1<<0)};

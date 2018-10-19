#include <time.h>
#include "sys/alt_stdio.h"
#include "system.h"
#include "altera_avalon_timer_regs.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

volatile int edge_capture;
volatile int edge_capture_time;

volatile char *timer_status_ptr = (char *)( TIMER_0_BASE);
volatile char *timer_control_ptr = (char *)(TIMER_0_BASE + 4);
volatile char *timer_mask_ptr = (char *)(TIMER_0_BASE + 8);
volatile char *timer_edge_cap_ptr = (char *)(TIMER_0_BASE + 12);


volatile char *btn_mask_ptr = (volatile char *)(BTNS_BASE + 8);
volatile char *btn_edge_ptr = (volatile char *)(BTNS_BASE + 12);

volatile char *hex0 = (char *)SEVEN_SEG_0_BASE;
volatile char *hex1 = (char *)SEVEN_SEG_1_BASE;
volatile char *hex2 = (char *)SEVEN_SEG_2_BASE;
volatile char *hex3 = (char *)SEVEN_SEG_3_BASE;
volatile char *hex4 = (char *)SEVEN_SEG_4_BASE;
volatile char *hex5 = (char *)SEVEN_SEG_5_BASE;

volatile char *leds = (char *)LEDS_BASE;

volatile char *btns = (char *)BTNS_BASE;
char btns_prev = 15;

struct tm ts;
struct tm alarm_ts;
time_t now = 0;
time_t alarm = 0;
time_t new_time;

int state ; // Estado 0: normal, Estado 1: Set hora, Estado 2: Set Alarma, Estado diferente: Indefinido

char num_to_seven_seg(int num) {
	unsigned int result = 0;

	if (num == 0) { result = 0x7F - 0x40; }
	else if (num == 1) { result = 0x7F - 0x79; }
	else if (num == 2) { result = 0x7F - 0x24; }
	else if (num == 3) { result = 0x7F - 0x30; }
	else if (num == 4) { result = 0x7F - 0x19; }
	else if (num == 5) { result = 0x7F - 0x12; }
	else if (num == 6) { result = 0x7F - 0x02; }
	else if (num == 7) { result = 0x7F - 0x78; }
	else if (num == 8) { result = 0x7F - 0x00; }
	else if (num == 9) { result = 0x7F - 0x10; }

	return ~result;
}

void show_time (volatile char * dec, volatile char * unit, int n){
	*dec = num_to_seven_seg(n / 10);
	*unit = num_to_seven_seg(n % 10);
}


static void timer_handler(void * context){
	/*
	 *
	 */
	*timer_status_ptr = 0;
	now++;

	if (state == 0){
		ts = *localtime(&now);
		alarm_ts = *localtime(&alarm);
		if (ts.tm_sec % 2 == 0 ){
			if (ts.tm_min == alarm_ts.tm_min && ts.tm_hour == alarm_ts.tm_hour){
				*leds = 0xFF;
			}
			else{
				*leds = 0;
			}
		}
		else {
			*leds = 0;
		}
	}
	else if (state == 1){
		ts = *localtime(&new_time);
		new_time++;
	}
	else if (state == 2){
		ts = *localtime(&alarm);
	}

	show_time(hex1, hex0, ts.tm_sec);
	show_time(hex3, hex2, ts.tm_min);
	show_time(hex5, hex4, ts.tm_hour);


}

static void btns_handler(void * context){
	volatile int * edge_capture_ptr = (volatile int*) context;
	*edge_capture_ptr = *(volatile int *)(BTNS_BASE + 12);
	*btn_mask_ptr = 0xf;
	*btn_edge_ptr = *edge_capture_ptr;
	switch(*btns){
	case 8:       // set time
		if(btns_prev != *btns)
			if (state != 1){
				state = 1;
				new_time = now;
			}
			else {
				state = 0;
				now = new_time;
			}
		break;
	case 4:      // set alarm
		if(btns_prev != *btns)
		state = (state != 2) ? 2 : 0;
		break;
	case 2:      // up
		if(btns_prev != *btns)
		if (state == 1) new_time += 300;
		else if (state == 2) alarm += 300;
		break;
	case 1:     // down
		if(btns_prev != *btns)
		if (state == 1) if(new_time != 0) new_time -= 300;
		else if (state == 2) if(alarm != 0) alarm -= 300;
		break;
	case 0: break;
	default:
		state = 0;
		break;
	}
	btns_prev = *btns;
}

static void init_btns(void)
{
	void * edge_capture_ptr = (void*) &edge_capture;
	*btn_mask_ptr = 0xF;
	*btn_edge_ptr = 0xF;
	alt_ic_isr_register( BTNS_IRQ_INTERRUPT_CONTROLLER_ID,
			BTNS_IRQ,
			btns_handler,
			edge_capture_ptr, 0);
}

static void init_timer (){
	void * edge_capture_ptr = (void*) &edge_capture_time;
	*timer_mask_ptr = 1;
	*timer_edge_cap_ptr = 0xF;
	alt_ic_isr_register (TIMER_0_IRQ_INTERRUPT_CONTROLLER_ID,
			TIMER_0_IRQ,
			timer_handler,
			edge_capture_ptr,
			0);
	*timer_control_ptr = 7;
	*timer_status_ptr = 0;
}

int main()
{ 
    init_timer();
    init_btns();



  /* Event loop never exits. */
  while (1);

  return 0;
}



















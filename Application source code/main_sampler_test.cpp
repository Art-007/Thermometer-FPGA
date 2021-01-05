/*****************************************************************//**
 * @file main_sampler_test.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "sseg_core.h"
#include "i2c_core.h"

// noise
float noise_correction(float reading)
{
	static float ini_volt = 0;

	float diff;
	diff = (ini_volt >= reading) ? ini_volt-reading : reading-ini_volt;
	if(diff > 0.002)//if voltage changes more than .002 update new value
	{
		ini_volt = reading;
		return reading;
	}
	else
		return ini_volt;
}

void  seg_output(SsegCore *sseg_p, float reading, int pt, int tens_pos, int ones_pos, int tenths_pos, int hundths_pos)
{

   	   float frac, shift_left1, shift_left2;
   	   int i_part, tens, ones, tenths, hundredths;

   	   //integer portion
   	   i_part = (int) reading; // integer part of fa
   	   frac = reading - (float) i_part;

   	   //tens place
   	   tens = i_part/10;

   	   //ones place
   	   ones = i_part - (tens*10);

   	   //tenths place
   	   shift_left1 = frac*10.0;//shift tenths place number to ones place
  	   tenths = (int)shift_left1 ;

  	   //hundredths place
  	   shift_left2 = (shift_left1 - tenths)*10.0; //subtract shifted value with integer to isolate decimal part of number
   	   hundredths = (int)shift_left2 ;

       //turn on a decimal point
       sseg_p->set_dp(pt);

       // set tens place
       sseg_p->write_1ptn(sseg_p->h2s(tens) , tens_pos);

       // set ones place
       sseg_p->write_1ptn(sseg_p->h2s(ones) , ones_pos);

       // set thousands place
       sseg_p->write_1ptn(sseg_p->h2s(tenths) , tenths_pos);

       // set hundreds place
       sseg_p->write_1ptn(sseg_p->h2s(hundredths) , hundths_pos);

}


// Seven segments F or  C
void  seg_options(I2cCore *adt7420_p,  SsegCore *sseg_p, float temp, float temp2, int display) {

	int i;
	float reading, reading2;

	// reading temp
   	reading = noise_correction(temp);
   	reading2 =  noise_correction(temp2) ;

    if (display == 3) // decide what we want to display. If 0 = celsius, 1 = fahrenheit, 2 = both, 3 = no display
     {
    	for (i = 0; i < 8; i++)
    	{
    		sseg_p->write_1ptn(0xff, i);
    	}

    	sseg_p->set_dp(0x00);
     }
    else if(display == 2)// display both
     {
    	seg_output(sseg_p, reading, 68, 3, 2, 1, 0);
    	seg_output(sseg_p, reading2, 68, 7, 6, 5, 4);
     }
    else
     {
    	 for (i = 4; i < 8; i++)
    	  {
    	     sseg_p->write_1ptn(0xff, i);
    	  }

    	seg_output(sseg_p, reading, 8, 4, 3, 2, 1);

    	 //display digits
       if(display == 1)
        {
           sseg_p->write_1ptn(sseg_p->h2s(15), 0);
        }
       else if(display == 0)
        {
    	   sseg_p->write_1ptn(sseg_p->h2s(12), 0);
        }
      }
}

// Find Celsius
float temp_check(I2cCore *adt7420_p, int f)
{
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   uint16_t tmp;
   float tmpC, tmpF;

   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
   }

   if(f==0)//Celsius
    {

       uart.disp("temperature (C): ");
       uart.disp(tmpC);
       uart.disp("\n\r");

       return tmpC;
    }

   if(f==1)//Fahrenheit
    {
	   tmpF =  (float) ((tmpC * 9/5) + 32 );

	   uart.disp("temperature (F): ");
	   uart.disp(tmpF);
	   uart.disp("\n\r");

	   return tmpF;
    }
}


// Display
void thermometer(I2cCore *adt7420_p, SsegCore *sseg_p, DebounceCore *btn_p)
{
	// Variables
	int up_button, down_button, left_button, right_button;
	float Ctemp, Ftemp;

	down_button = btn_p->read_db(2); // read button
	up_button = btn_p->read_db(0); // read button
	right_button = btn_p->read_db(1); // read button
	left_button = btn_p->read_db(3); // read button


	if (up_button)  // display clesius with C
	 {
         do{
        	  down_button = btn_p->read_db(2); // read button
        	  right_button = btn_p->read_db(1); // read button
        	  left_button = btn_p->read_db(3); // read button

		      Ctemp = temp_check(adt7420_p, 0);
		      seg_options(adt7420_p, sseg_p, Ctemp, 0, 0);

    	   } while (!down_button && !right_button && !left_button);
	 }

	if (down_button)  // display freh with F
		{
		  do{
			  up_button = btn_p->read_db(0); // read button
			  right_button = btn_p->read_db(1); // read button
			  left_button = btn_p->read_db(3); // read button

			 Ftemp = temp_check(adt7420_p, 1);
			 seg_options(adt7420_p, sseg_p, Ftemp, 0, 1);

			} while (!up_button && !right_button && !left_button);
		}

	if (left_button)   // display both temperatures on the 8 digits
	   {
		 do{
			  down_button = btn_p->read_db(2); // read button
			  up_button = btn_p->read_db(0); // read button
			  right_button = btn_p->read_db(1); // read button

			  Ftemp = temp_check(adt7420_p, 1);
			  Ctemp = temp_check(adt7420_p, 0);
			  seg_options(adt7420_p, sseg_p, Ctemp, Ftemp, 2);

			} while (!down_button && !up_button && !right_button);
	   }

	if (right_button)  // trun off display
	{
	  do{
		    down_button = btn_p->read_db(2); // read button
		    up_button = btn_p->read_db(0); // read button
		    left_button = btn_p->read_db(3); // read button

		    seg_options(adt7420_p, sseg_p, 0, 0, 3);

		} while (!down_button && !up_button && !left_button);
	 }


}

GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));

int main() {

   while (1) {
	   thermometer(&adt7420, &sseg, &btn);

   } //while
} //main

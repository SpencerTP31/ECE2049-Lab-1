#include <msp430.h>
#include <stdint.h>
#include "inc\hw_memmap.h"
#include "driverlibHeaders.h"
#include "CTS_Layer.h"

#include "grlib.h"
#include "LcdDriver/Dogs102x64_UC1701.h"
#include "peripherals.h"
#include <stdio.h>
#include <stdlib.h>
#include "tones.h"

void swDelay(int numLoops);
int draw_sprites();
void refresh_display();
void destroy_sprite(int column);
void update_sprites();
void add_sprite(int col);
void create_sprites();
int get_touchpad();
void flash_lose();
void clear_display();
void clear_sprites();
void BuzzerOnFreq(int freq);

char sprite_mat[5][4];
char null_sprite = '0';
char sprite = 'X';
int level = 0;
int sprite_offset_x = 0, sprite_offset_y = 0, sprite_x_dir = 0;
int update_counter = 15;
int score = 0;

typedef enum {S_MENU, S_COUNTDOWN, S_PLAY, S_ADVANCE, S_LOSE} state_t;
state_t state = S_MENU;

/** 
 * @brief The main function for space invaders.
 * 
 * @return 0 if no errors, 1 if errors occurred at runtime.
 */
int main(void)
{
    // Stop WDT
    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer
    P1SEL = P1SEL & ~BIT0;          // Select P1.0 for digital IO
    P1DIR |= BIT0;          // Set P1.0 to output direction
    P1OUT &= ~BIT0;

    //Perform initializations (see peripherals.c)
    configTouchPadLEDs();
    configDisplay();
    configCapButtons();

    char level_str[9];
    int result;

    while (1)
    {
        switch (state)
        {
        case S_MENU: //Initial State
        	clear_display();
        	level = 0;
        	update_counter = 0;
        	score = 0;
        	P1OUT |= (BIT1);
            GrStringDrawCentered(&g_sContext, "SPACE INVADERS", AUTO_STRING_LENGTH,
                                 51, 20, TRANSPARENT_TEXT);
            GrStringDrawCentered(&g_sContext, "Press X to start", AUTO_STRING_LENGTH,
                                 51, 40, TRANSPARENT_TEXT);
            refresh_display();
            if (get_touchpad() == 0)
            {
                state = S_COUNTDOWN;
                BuzzerOnFreq(125);
            }
            break;
        case S_COUNTDOWN:
        	P1OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
        	clear_display();
            GrStringDrawCentered(&g_sContext, "3...", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            swDelay(1);
            clear_display();
            GrStringDrawCentered(&g_sContext, "2...", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            BuzzerOnFreq(150);
            swDelay(1);
            clear_display();
            GrStringDrawCentered(&g_sContext, "1...", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            BuzzerOnFreq(175);
            swDelay(1);
            state = S_ADVANCE;
            BuzzerOff();
            P1OUT |= (BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
            break;
        case S_PLAY: //In game
            //Calculate sprite movement.
        	update_sprites();
            destroy_sprite(get_touchpad());
            result = draw_sprites();
            if (result == 1) state = S_ADVANCE;
            if (result == -1) state = S_LOSE;
            BuzzerOff();
            break;
        case S_ADVANCE: //Advance Level
        	clear_sprites();
            level++;
            clear_display();
            snprintf(level_str, 9, "Level %d", level);
            GrStringDrawCentered(&g_sContext, level_str, AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            create_sprites();
            refresh_display();
            state = S_PLAY;
            swDelay(1);
            break;
        case S_LOSE: //You lose!
        	P1OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
        	clear_display();
            GrStringDrawCentered(&g_sContext, "You lose!", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            flash_lose();
            swDelay(1);
            state = S_MENU;
            break;
        }
    }
}

/** 
 * @brief Flash the LEDs and play tones when the player loses.
 */
void flash_lose()
{
    //Make annoying sounds and lights when player loses.
	int i;
	int note = 150;
	for (i = 0; i < 4; i++)
	{
		note = (note == 150) ? 175 : 150;
		BuzzerOnFreq(note);
		swDelay(1);
		P1OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
		swDelay(1);
		P1OUT |= (BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
	}
	BuzzerOff();
	P1OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
}

/** 
 * @brief Clear the sprite array and sprite offsets.
 */
void clear_sprites()
{
	int i, j;
	sprite_offset_y = 0;
	sprite_offset_x = 0;
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 4; j++)
		{
			sprite_mat[i][j] = null_sprite;
		}
	}
}

/** 
 * @brief Gets the touchpad button that's activated.
 * 
 * @return An int, the button pressed. Corresponds to the sprite column.
 */
int get_touchpad()
{
    //Get touchpad button pressed. Returns column that the button corresponds to.
	switch (CapButtonRead())
	{
	case BUTTON_NONE:
		return -1;
	case BUTTON_X:
		return 0;
	case BUTTON_SQ:
		return 1;
	case BUTTON_OCT:
		return 2;
	case BUTTON_TRI:
		return 3;
	case BUTTON_CIR:
		return 4;
	default:
		return -1;
	}
}

/** 
 * @brief Destroy a sprite in the specified column.
 * 
 * @param column The column to destroy a sprite in.
 */
void destroy_sprite(int column)
{
    if (column == -1) return;
    int i;
    for (i = 3; i >= 0; i--)
    {
        if (sprite_mat[column][i] != null_sprite)
        {
            sprite_mat[column][i] = null_sprite;
            BuzzerOnFreq(200);
            score++;
            return;
        }
    }
}

/** 
 * @brief Create and fill the sprite array.
 */
void create_sprites()
{
    unsigned int sprites_gen = (level * 2 > 20) ? 20 : level * 2;
    unsigned int i;
    for (i = 0; i < sprites_gen; i++)
    {
        int col = rand() % 5;
        add_sprite(col);
    }
}

/** 
 * @brief Add a sprite to the specified column.
 * 
 * @param col The column to add a sprite to.
 */
void add_sprite(int col)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        if (sprite_mat[col][i] == null_sprite)
        {
            sprite_mat[col][i] = sprite;
            return;
        }
    }
}

/** 
 * @brief Update the sprite offsets.
 */
void update_sprites()
{
    if (sprite_offset_x >= 5)
    {
        sprite_x_dir = 0;
        sprite_offset_y += 5;
        BuzzerOnFreq(100);
    }
    else if (sprite_offset_x <= -5)
    {
        sprite_x_dir = 1;
        sprite_offset_y += 5;
        BuzzerOnFreq(100);
    }
    sprite_offset_x = (sprite_x_dir == 1) ? sprite_offset_x + 2 + (level / 10) : sprite_offset_x - 2 - (level / 10);
    (sprite_offset_x > 5) ? sprite_offset_x = 5 : (sprite_offset_x < -5) ? sprite_offset_x = -5 : 0;
}

/** 
 * @brief Draw sprites onscreen.
 * 
 * @return 0 if no errors.
 */
int draw_sprites()
{
	clear_display();
    int i, j, empty_count = 0;
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (sprite_mat[i][j] != null_sprite)
            {
                //Draw sprite.
                int y_pos = 5 + (j * 10) + sprite_offset_y;
                int x_pos = 12 + (i * 20) + sprite_offset_x;
                GrStringDrawCentered(&g_sContext, "X", 1,
                                     x_pos, y_pos, TRANSPARENT_TEXT);
                if (y_pos >= 60) return -1;
            }
            else
            {
                empty_count++;
            }
        }
    }
    if (empty_count == 20)
    {
        //Advance level, all sprites destroyed.
        return 1;
    }
    refresh_display();
    return 0;
}

/** 
 * @brief Clear the display.
 */
void clear_display()
{
	GrClearDisplay(&g_sContext);
}

/** 
 * @brief Write all display changes to display.
 */
void refresh_display()
{
	GrFlush(&g_sContext);
}

/*
 * Enable a PWM-controlled buzzer on P7.5
 * This function makes use of TimerB0.
 * ACLK = 32768Hz
 */
void BuzzerOnFreq(int freq)
{
	// Initialize PWM output on P7.5, which corresponds to TB0.3
	P7SEL |= BIT5; // Select peripheral output mode for P7.5
	P7DIR |= BIT5;

	TB0CTL  = (TBSSEL__ACLK|ID__1|MC__UP);  // Configure Timer B0 to use ACLK, divide by 1, up mode
	TB0CTL  &= ~TBIE; 						// Explicitly Disable timer interrupts for safety

	// Now configure the timer period, which controls the PWM period
	// Doing this with a hard coded values is NOT the best method
	// I do it here only as an example. You will fix this in Lab 2.
	TB0CCR0   = freq; 					// Set the PWM period in ACLK ticks
	TB0CCTL0 &= ~CCIE;					// Disable timer interrupts

	// Configure CC register 3, which is connected to our PWM pin TB0.3
	TB0CCTL3  = OUTMOD_7;					// Set/reset mode for PWM
	TB0CCTL3 &= ~CCIE;						// Disable capture/compare interrupts
	TB0CCR3   = TB0CCR0/2; 					// Configure a 50% duty cycle
}

/** 
 * @brief Delay a specified number of loops/
 * 
 * @param numLoops The number of loops to delay.
 */
void swDelay(int numLoops)
{
    volatile unsigned long int i, j; // volatile to prevent optimization
    // by compiler

    for (j = 0; j < numLoops; j++)
    {
        i = 50000;                   // SW Delay
        while (i > 0)               // could also have used while (i)
            i--;
    }
}


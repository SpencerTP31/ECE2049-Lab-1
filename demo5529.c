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

void swDelay(char numLoops);
int draw_sprites();
void refresh_display();
void destroy_sprite(int column);
void update_sprites();
void add_sprite(int col);
void create_sprites();
int get_touchpad();
void flash_lose();
void clear_display();

char sprite_mat[5][4];
char null_sprite = '0';
char sprite = 'X';
int level = 0;
int sprite_offset_x, sprite_offset_y, sprite_x_dir = 0;

typedef enum {S_MENU, S_COUNTDOWN, S_PLAY, S_ADVANCE, S_LOSE} state_t;
state_t state = S_MENU;

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
        	P1OUT |= (BIT1);
            GrStringDrawCentered(&g_sContext, "SPACE INVADERS", AUTO_STRING_LENGTH,
                                 51, 20, TRANSPARENT_TEXT);
            GrStringDrawCentered(&g_sContext, "Press X to start", AUTO_STRING_LENGTH,
                                 51, 40, TRANSPARENT_TEXT);
            refresh_display();
            if (get_touchpad() == 1)
            {
                state = S_COUNTDOWN;
            }
            break;
        case S_COUNTDOWN:
        	clear_display();
            GrStringDrawCentered(&g_sContext, "3...", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            swDelay(1);
            clear_display();
            GrStringDrawCentered(&g_sContext, "2...", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            swDelay(1);
            clear_display();
            GrStringDrawCentered(&g_sContext, "1...", AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            refresh_display();
            swDelay(1);
            break;
        case S_PLAY: //In game
            //Calculate sprite movement.
            update_sprites();
            destroy_sprite(get_touchpad());
            result = draw_sprites();
            if (result == 1) state = S_ADVANCE;
            if (result == -1) state = S_LOSE;
            break;
        case S_ADVANCE: //Advance Level
            level++;
            snprintf(level_str, 9, "Level %d", level);
            GrStringDrawCentered(&g_sContext, level_str, AUTO_STRING_LENGTH,
                                 51, 36, TRANSPARENT_TEXT);
            create_sprites();
            refresh_display();
            state = S_PLAY;
            break;
        case S_LOSE: //You lose!
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

void flash_lose()
{
    //Make annoying sounds and lights when player loses.
}

int get_touchpad()
{
    //Get touchpad button pressed. Returns column that the button corresponds to.
	switch (CapButtonRead())
	{
	case BUTTON_NONE:
		return 0;
	case BUTTON_X:
		return 1;
	case BUTTON_SQ:
		return 2;
	case BUTTON_OCT:
		return 3;
	case BUTTON_TRI:
		return 4;
	case BUTTON_CIR:
		return 5;
	default:
		return 0;
	}
	return 0;
}

void destroy_sprite(int column)
{
    if (column == -1) return;
    int i;
    for (i = 3; i >= 0; i--)
    {
        if (sprite_mat[column][i] != null_sprite)
        {
            sprite_mat[column][i] = null_sprite;
            return;
        }
    }
}

void create_sprites()
{
    unsigned int sprites_gen = level * 2;
    unsigned int i;
    for (i = 0; i < sprites_gen; i++)
    {
        int col = rand() % 5;
        add_sprite(col);
    }
}

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

void update_sprites()
{
    if (sprite_offset_x > 10)
    {
        sprite_x_dir = 0;
        sprite_offset_y += 5;
    }
    else if (sprite_offset_x < -10)
    {
        sprite_x_dir = 0;
        sprite_offset_y += 5;
    }
    sprite_offset_x = (sprite_x_dir == 1) ? sprite_offset_x + 2 : sprite_offset_x - 2;
}

int draw_sprites()
{
	clear_display();
    int i, j, empty_count;
    //Draw sprites on screen.
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (sprite_mat[i][j] != null_sprite)
            {
                //Draw sprite.
                int x_pos = 5 + (j * 20) + sprite_offset_x;
                int y_pos = 5 + (i * 10) + sprite_offset_y;
                GrStringDrawCentered(&g_sContext, sprite_mat[i][j], 1,
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

void clear_display()
{
	GrClearDisplay(&g_sContext);
}

void refresh_display()
{
	GrFlush(&g_sContext);
}

void swDelay(char numLoops)
{
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2013

    volatile char i, j; // volatile to prevent optimization
    // by compiler

    for (j = 0; j < numLoops; j++)
    {
        i = (char)50000 ;                   // SW Delay
        while (i > 0)               // could also have used while (i)
            i--;
    }
}

#include <genesis.h>
#include <gfx.h>
#include <resources.h>
#include "writer.h"
#include "transition_helper.h"

#define	TABLE_LEN 220
#define MAX_DONUT 16

static u16 tileIndexes[64];

static void fastStarFieldFX();

s16 scroll_PLAN_B[TABLE_LEN];
u8 scroll_speed[TABLE_LEN];

int main(){
	fastStarFieldFX();
	return 0;
}

u16 vramIndex;
u8 figure_mode = 0;
s16 i, ns, s, ind, figure_counter = 0;
static Object objects[256];
Sprite *sprites[256];
u8 demo_phase = 0;
u16 phase_counter = 0;

static void fastStarFieldFX()
{
	DMA_setAutoFlush(FALSE);
	DMA_setMaxTransferSize(0);

	vramIndex = TILE_USERINDEX;
	RSE_clearAll();

	RSE_turn_screen_to_black();
	VDP_setPlanSize(64, 32);
	VDP_clearPlan(PLAN_A, TRUE);
	VDP_clearPlan(PLAN_B, TRUE);

	for(i = 0; i < 30; i++)
		VDP_waitVSync();

	VDP_waitDMACompletion();

	VDP_setEnable(0);

	/* Load the fond tiles */
	VDP_loadTileSet(amiga_font.tileset, vramIndex, TRUE);
	vramIndex += amiga_font.tileset->numTile;	

	/* Draw the foreground */
	VDP_drawImageEx(PLAN_B, &starfield, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, vramIndex), 0, 0, FALSE, TRUE);
	VDP_drawImageEx(PLAN_B, &starfield, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, vramIndex), 256 >> 3, 0, FALSE, TRUE);
	vramIndex += starfield.tileset->numTile; 	

	/* Draw the logo */
	VDP_drawImageEx(PLAN_A, &vip_logo, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, vramIndex), (320 - 256) >> 4, (224 - 144) >> 4, FALSE, TRUE);
	vramIndex += vip_logo.tileset->numTile; 	

	DMA_flushQueue();
	VDP_setEnable(1);

	/*	Set the proper scrolling mode (line by line) */
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

	/*	Create the scrolling offset table */
	s = 1;
	for(i = 0; i < TABLE_LEN; i++)
	{
		scroll_PLAN_B[i] = 0;
		do
		{
			ns = -((random() % 3) + 1);
		}
		while (ns == s);
		scroll_speed[i] = ns;
		s = ns;
	}

	/* Setup the sprites */
	SPR_init(0,0,0);

    // ind = vramIndex;
    for(i = 0; i < donut.animations[0]->numFrame; i++)
    {
        TileSet* tileset = donut.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, vramIndex, TRUE);
        tileIndexes[i] = vramIndex;
        vramIndex += tileset->numTile;
    }

	for(i = 0; i < MAX_DONUT; i++)
	{
	    sprites[i] = SPR_addSprite(&donut, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
		SPR_setAutoTileUpload(sprites[i], FALSE);
		SPR_setVRAMTileIndex(sprites[i], TILE_USERINDEX);
		sprites[i]->data = (u32) &objects[i];    
	}

	// SPR_update(sprites, MAX_DONUT);

	VDP_setPalette(PAL3, amiga_font.palette->data);

	VDP_setHilightShadow(0); 

	/* writer setup */
	current_string_idx = 0;
	current_char_idx = 0;
	current_char_x = 0;
	writer_timer = 0;
	writer_switch = FALSE;

	writer_state = WRT_CENTER_CUR_LINE;

	SND_startPlay_VGM(maak_music_2);

	/*	Start !!!! */
	s = 0;
	while (TRUE)
	{
		VDP_waitVSync();
		DMA_flushQueue();

		switch(demo_phase)
		{
			case 0:
				VDP_fadePalTo(PAL1, starfield.palette->data, 32, TRUE);
				phase_counter = 0;
				demo_phase++;
				break;

			case 1:
				phase_counter++;
				if (phase_counter > 32 + 16)
				{
					phase_counter = 0;
					demo_phase++;
					VDP_fadePalTo(PAL2, donut.palette->data, 32, TRUE);
				}
				break;

			case 2:
				phase_counter++;
				if (phase_counter > 32 + 16)
				{
					phase_counter = 0;
					demo_phase++;
					VDP_fadePalTo(PAL0, vip_logo.palette->data, 32, TRUE);					
				}
				break;

			case 3:		
				break;
		}

		if (demo_phase > 2)
		{
			if (writer_switch || writer_state == WRT_CLEAR_LINE)
				RSE_updateLineWriter();

			writer_switch = !writer_switch;					
		}

		/* 	Scroll the starfield */
		VDP_setHorizontalScrollLine(PLAN_B, 2, scroll_PLAN_B, TABLE_LEN, TRUE);
		for(i = 0; i < TABLE_LEN; i++)
			scroll_PLAN_B[i] = (scroll_PLAN_B[i] + scroll_speed[i]) & 0xFF;

		/*	Animate the donuts */
		for(i = 0; i < MAX_DONUT; i++)
		{
			switch(figure_mode)
			{
				case 0:
					SPR_setPosition(sprites[i], (cosFix16(s + (i << 5)) << 1) + 160 - 16, sinFix16(s + (i << 5)) + 112 - 16);
					break;

				case 1:
					SPR_setPosition(sprites[i], (cosFix16(s + (i << 6)) << 1) + 160 - 16, (sinFix16(s + (i << 5))) + 112 - 16);
					break;

				case 2:
					SPR_setPosition(sprites[i], ((sinFix16((s << 1) + (i << 6))) << 1) + 160 - 16, (cosFix16(s + (i << 5)) << 1) + 112 - 16);
					break;

				case 3:
					SPR_setPosition(sprites[i], (sinFix16(s + (i << 7))) + 160 - 16, (cosFix16((s >> 1) + (i << 4)) << 1) + 112 - 16);
					break;

				case 4:
					SPR_setPosition(sprites[i], (cosFix16((s << 1) + (i << 5)) << 1) + 160 - 16, (sinFix16((s >> 1) + (i << 5)) << 1) + 112 - 16);
					break;
			}
			SPR_setVRAMTileIndex(sprites[i], tileIndexes[((s >> 4) + i) & 0x7]);
		}

		SPR_update(sprites, MAX_DONUT);

		s += 4;
		figure_counter++;
		if (figure_counter > 512)
		{
			figure_mode++;
			if (figure_mode > 4) figure_mode = 0;

			figure_counter = 0;
		}		
	}
}

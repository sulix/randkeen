/*
 * Keen 1 Randomiser, by David Gow <david@davidgow.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>


// Options
int opt_seed = 0;
bool opt_useLevelNames = true;
bool opt_shuffleEnemies = true;
bool opt_startPogo = false;
int opt_startAmmo = 0;
int opt_extraPogo = 0;
bool opt_debug = false;

void debugf(const char *format, ...)
{
	if (!opt_debug)
		return;
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

uint16_t fread_u16(FILE *f)
{
	uint16_t u;
	fread(&u,2,1,f);
	return u;
}

uint32_t fread_u32(FILE *f)
{
	uint32_t u;
	fread(&u, 4, 1, f);
	return u;
}

void fwrite_u16(FILE *f, uint16_t val)
{
	fwrite(&val, 2, 1, f);
}

void fwrite_u32(FILE *f, uint16_t val)
{
	fwrite(&val, 4, 1, f);
}

typedef struct VorticonsMap
{
	uint16_t w;
	uint16_t h;
	uint16_t *planes[2];
} VorticonsMap;

#define VORT_MAP_RLE_TAG 0xFEFE

uint16_t *VORT_DecompressRLE(FILE *f, uint32_t decompSize)
{
	uint16_t *data = malloc(decompSize);
	
	for(uint16_t *ptr = data; decompSize;)
	{
		uint16_t val = fread_u16(f);
		if (val == VORT_MAP_RLE_TAG)
		{
			uint16_t length = fread_u16(f);
			val = fread_u16(f);
			while (length--)
			{
				*(ptr++) = val;
				decompSize -= 2;
			}
		}
		else
		{
			*(ptr++) = val;
			decompSize -= 2;
		}
	}
	return data;
}

void VORT_CompressRLE(uint16_t* data, uint32_t dataSize, FILE *f)
{
	uint16_t currentVal = 0xFFFF;
	uint16_t currentValCount = 0;
	while (dataSize)
	{
		uint16_t val = *(data++);
		if (val != currentVal)
		{
			if (currentValCount > 3)
			{
				fwrite_u16(f, VORT_MAP_RLE_TAG);
				fwrite_u16(f, currentValCount);
				fwrite_u16(f, currentVal);
			}
			else
			{
				for (int i = 0; i < currentValCount; ++i)
				{
					fwrite_u16(f, currentVal);
				}
			}
			currentVal = val;
			currentValCount = 1;
		}
		else
		{
			currentValCount++;
		}
		dataSize -= 2;
	}
	if (currentValCount)
	{
		if (currentValCount > 3)
		{
			fwrite_u16(f, VORT_MAP_RLE_TAG);
			fwrite_u16(f, currentValCount);
			fwrite_u16(f, currentVal);
		}
		else
		{
			for (int i = 0; i < currentValCount; ++i)
			{
				fwrite_u16(f, currentVal);
			}
		}
	}
}


VorticonsMap VORT_LoadMap(FILE *f)
{
	uint32_t decompSize = fread_u32(f);
	uint16_t *mapData = VORT_DecompressRLE(f, decompSize);
	VorticonsMap vMap;
	vMap.w = mapData[0];
	vMap.h = mapData[1];
	uint16_t mapPlaneSize = mapData[7];
	vMap.planes[0] = mapData + 16;
	vMap.planes[1] = mapData + 16 + (mapPlaneSize/2);
	
	return vMap;
}

uint16_t VORT_GetTileAtPos(VorticonsMap *vMap, int x, int y, int plane)
{
	return vMap->planes[plane][y * vMap->w + x];
}

void VORT_SetTileAtPos(VorticonsMap *vMap, int x, int y, int plane, uint16_t tile)
{
	vMap->planes[plane][y * vMap->w + x] = tile;
}

void VORT_SaveMap(VorticonsMap *vMap, FILE *f)
{
	uint16_t planeSize = (vMap->w * vMap->h * 2 + 15) & ~15;
	uint32_t dataLen = planeSize*2 + 32;
	uint16_t *data = calloc(dataLen,1);
	data[0] = vMap->w;
	data[1] = vMap->h;
	data[2] = 2; // Number of planes.
	data[7] = planeSize;

	memcpy(&data[16], vMap->planes[0], planeSize);
	memcpy(&data[16 + (planeSize / 2)], vMap->planes[1], planeSize);

	fwrite_u32(f, dataLen);
	VORT_CompressRLE(data, dataLen, f);
}


bool VORT_FindTile(VorticonsMap *vMap, uint16_t tile, int plane, int *x, int *y)
{
	for (int _y = *y; _y < vMap->h; ++_y)
	{
		for (int _x = (_y == *y)?*x:0; _x < vMap->w; ++_x)
		{
			if (VORT_GetTileAtPos(vMap, _x, _y, plane) == tile)
			{
				*x = _x;
				*y = _y;
				return true;
			}
		}
	}
	return false;
}

void VORT_ReplaceTiles(VorticonsMap *vMap, uint16_t tileFrom, uint16_t tileTo, int plane)
{
	int x = 0;
	int y = 0;
	while (VORT_FindTile(vMap, tileFrom, plane, &x, &y))
	{
		debugf("Found tile %d at (%d, %d)\n", tileFrom, x, y);
		VORT_SetTileAtPos(vMap, x, y, plane, tileTo);
	}
}

void PermuteArray(int *array, int count)
{
	for (int i = 0; i < count - 2; ++i)
	{
		int j = (rand() % (count - i)) + i;
		if (array[i] != array[j])
		{
			array[i] ^= array[j];
			array[j] ^= array[i];
			array[i] ^= array[j];
		}
	}

}

#define K1_T_GREYSKY 143
#define K1_T_POGOSTICK 176
#define K1_T_JOYSTICK 221
#define K1_T_BATTERY 237
#define K1_T_EVERCLEAR 245
#define K1_T_VACUUM 241
#define K1_T_EXITSIGN1 167
#define K1_T_EXITSIGN2 168

// NOTE: We don't shuffle the position of the Pogo Stick in Level 16, as it may
// be required to complete the level, and hence the game.
int slotsPerLevel[16] = {1, 1, 2, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1};
int itemsPerSlot[20] = {K1_T_POGOSTICK, K1_T_JOYSTICK, K1_T_BATTERY, K1_T_EVERCLEAR, K1_T_VACUUM,
	K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY,
	K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY,
	K1_T_GREYSKY, K1_T_GREYSKY, K1_T_GREYSKY};

void K1_AddExtraPogos()
{
	int freeSlot = 0;
	while (itemsPerSlot[freeSlot] != K1_T_GREYSKY)
		freeSlot++;

	for (int pogo = 0; pogo < opt_extraPogo; ++pogo)
		itemsPerSlot[freeSlot++] = K1_T_POGOSTICK;
}

void K1_SetSpecialItem(VorticonsMap *vMap, uint16_t item, int slot)
{
	// There are two types of special item slots:
	// - A place where a special item normally goes
	// - The place below the "exit" sign in a level
	
	if (item != K1_T_GREYSKY)
		debugf("\tItem %d in slot %d\n");

	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t tile = VORT_GetTileAtPos(vMap, x, y, 0);
			if (tile == K1_T_POGOSTICK ||
				(tile >= K1_T_JOYSTICK && tile < K1_T_JOYSTICK+4) ||
				(tile >= K1_T_BATTERY && tile < K1_T_EVERCLEAR+4) ||
				(tile == K1_T_EXITSIGN1 || tile == K1_T_EXITSIGN2))
			{
				if (!slot--)
				{
					// If it's the exit sign...
					if (tile == K1_T_EXITSIGN1 || tile == K1_T_EXITSIGN2)
					{
						y++;
						uint16_t underTile = VORT_GetTileAtPos(vMap, x, y, 0);
						// Check that there's a free space below the exit sign.
						if (underTile != K1_T_GREYSKY)
						{
							y--;
							slot++;
							continue;
						}
					}
					VORT_SetTileAtPos(vMap, x, y, 0, item);
				}
			}
		}
	}
}

int mapLocations[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9,
			10, 11, 12, 13, 14, 15, 16};

void K1_ShuffleLevelEntries(VorticonsMap *worldMap)
{

	PermuteArray(mapLocations, 16);

	for (int y = 0; y < worldMap->h; ++y)
	{
		for (int x = 0; x < worldMap->w; ++x)
		{
			uint16_t entry = VORT_GetTileAtPos(worldMap, x, y, 1);
			if (!entry)
				continue;
			int level = entry & 0x7F;
			if (level > 16)
				continue;
			debugf("Changing level %d (entry %x) ->", level, entry);
			level = mapLocations[level-1];
			entry = (entry & ~0x7F) | level;
			debugf(" level %d (entry %x)\n", level, entry);
			VORT_SetTileAtPos(worldMap, x, y, 1, entry);
		}
	}
}

// Enemy heights in tiles. We need to adjust enemy positions based on this.
int enemyHeights[5] = {2, 2, 3, 1, 2};

void K1_ShuffleEnemies(VorticonsMap *vMap)
{
	int totalEnemies = 0;
	int enemyCount[5] = {0};
	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t sprite = VORT_GetTileAtPos(vMap, x, y, 1);
			if (sprite >= 1 && sprite <= 5)
			{
				enemyCount[sprite-1]++;
				totalEnemies++;
			}
		}
	}


	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t sprite = VORT_GetTileAtPos(vMap, x, y, 1);
			if (sprite >= 1 && sprite <= 5)
			{
				int enemyIdx = rand() % totalEnemies;
				int enemyType = 0;
				while (enemyIdx > enemyCount[enemyType])
				{
					enemyIdx -= enemyCount[enemyType++];
				}
				int oldEnemyType = VORT_GetTileAtPos(vMap, x, y, 1) - 1;
				VORT_SetTileAtPos(vMap, x, y, 1, 0);
				// Avoid having the enemy stuck in the map.
				// Note that we only raise enemies off the ground, otherwise
				// we'd end up processing the enemy twice (and dividing by 0 as
				// a result)
				int ny = y;
				if (enemyHeights[enemyType] > enemyHeights[oldEnemyType])
				{
					ny -= enemyHeights[enemyType] - enemyHeights[oldEnemyType];
					debugf("Enemy %d (%d,%d) of height %d <-=-> Enemy %d (%d, %d) (height %d)\n", oldEnemyType, x, y, enemyHeights[oldEnemyType], enemyType, x, ny, enemyHeights[enemyType]);
				}
				VORT_SetTileAtPos(vMap, x, ny, 1, 1 + enemyType);
				enemyCount[enemyType]--;
				totalEnemies--;
			}
		}
	}
}

void K1_ShuffleLollies(VorticonsMap *vMap)
{
	int totalLollies = 0;
	int lollyCount[5] = {0};
	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t tile = VORT_GetTileAtPos(vMap, x, y, 0);
			if (tile >= 201 && tile <= 205)
			{
				lollyCount[tile-201]++;
				totalLollies++;
			}
		}
	}


	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t tile = VORT_GetTileAtPos(vMap, x, y, 0);
			if (tile >= 201 && tile <= 205)
			{
				int lollyIdx = rand() % totalLollies;
				int lollyType = 0;
				while (lollyIdx > lollyCount[lollyType])
				{
					lollyIdx -= lollyCount[lollyType++];
				}
				VORT_SetTileAtPos(vMap, x, y, 0, 201 + lollyType);
				lollyCount[lollyType]--;
				totalLollies--;
			}
		}
	}
}

void K1_MungeBlockColours(VorticonsMap *vMap)
{
	int blockMask = rand()&3;

	int blockToJumpThruMatrix[] = {1,3,0,2};
	int blockToJumpThruInvert[] = {2,0,3,1};

	debugf("MungeBlockColours: mask = %d\n", blockMask);

	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t tile = VORT_GetTileAtPos(vMap, x, y, 0);
			// Solid blocks
			if (tile >= 331 && tile <= 334)
			{
				tile = ((tile - 331) ^ blockMask) + 331;
				VORT_SetTileAtPos(vMap, x, y, 0, tile);
			}
			// Jump-thru blocks
			if ((tile >= 178 && tile <= 181))
			{
				int colour = blockToJumpThruInvert[tile - 178]; 
				colour ^= blockMask;
				tile = 178 + blockToJumpThruMatrix[colour];
				VORT_SetTileAtPos(vMap, x, y, 0, tile);
			}
		}
	}
}

void K1_MungeKeys(VorticonsMap *vMap)
{
	int keyMask = rand()&3;

	for (int y = 0; y < vMap->h; ++y)
	{
		for (int x = 0; x < vMap->w; ++x)
		{
			uint16_t tile = VORT_GetTileAtPos(vMap, x, y, 0);
			// Keys
			if (tile >= 190 && tile <= 193)
			{
				tile = ((tile - 190) ^ keyMask) + 190;
				VORT_SetTileAtPos(vMap, x, y, 0, tile);
			}
			// Doors
			if ((tile >= 173 && tile <= 174) || (tile >= 195 && tile <= 200))
			{
				int topOrBot = 1 - (tile & 1);
				int colour = (tile <= 174)?0:((tile - 193) >> 1);
				colour ^= keyMask;
				tile = ((colour == 0)?(173):(195+(colour-1)*2)) + topOrBot;
				VORT_SetTileAtPos(vMap, x, y, 0, tile);
			}
		}
	}
}

// We remove the GARG scream for now, as there's just not enough
// room to fit it in the existing message space.
// A future version can patch it in elsewhere.
int hintLevels[] = {2, 6, 9, 10, 12, 15, -1};
int currentHint = 0;

// Level names
const char *levelNames[] = {
	NULL,
	"Border Town",
	"1st Red Rock Shrine",
	"Treasury",
	"Capital City",
	"Pogo Shrine",
	"2nd Red Rock Shrine",
	"Emerald City",
	"Ice City",
	"3rd Red Rock Shrine",
	"1st Ice Shrine",
	"4th Red Rock Shrine",
	"5th Red Rock Shrine",
	"Red Maze City",
	"Secret City",
	"2nd Ice Shrine",
	"Commander's Castle"
};

int hintTiles[] = {K1_T_POGOSTICK, K1_T_JOYSTICK, K1_T_BATTERY, K1_T_VACUUM, K1_T_EVERCLEAR, -1};
const char *hintTileNames[] = {"Pogo Stick", "Joystick", "Battery", "Vacuum", "Everclear", 0};

bool WriteHintHeaderToPatch(FILE *f)
{
	if (hintLevels[currentHint] == -1)
		return false;

	if (hintLevels[currentHint] != 11)
		fprintf(f, "%%level.hint %d\nA Yorpy Mind\nThought Bellows:\n", hintLevels[currentHint]);
	else
		fprintf(f, "%%level.hint %d\nA Gargish Scream\nEchoes In Your\nHead:\n", hintLevels[currentHint]);

	currentHint++;
	return true;
}

void WriteTileHintToPatch(FILE *f, int tile, int level)
{
	int hintId = 0;
	while (tile != hintTiles[hintId] && hintTiles[hintId] != -1)
		hintId++;
	if (hintTiles[hintId] == -1)
		return;

	if (!WriteHintHeaderToPatch(f))
		return;

	if (opt_useLevelNames)
		fprintf(f, "The %s is\nfound in the\n%s\n\n", hintTileNames[hintId], levelNames[level]);
	else
		fprintf(f, "The %s is\nfound in level %d\n\n", hintTileNames[hintId], level);
}

void WriteMapHintToPatch(FILE *f, int oldMap, int newMap)
{
	if (oldMap == newMap)
		return;
	if (!WriteHintHeaderToPatch(f))
		return;

	if (opt_useLevelNames)
		fprintf(f, "%s\nrests where the\n%s\nonce was...\n\n", levelNames[newMap], levelNames[oldMap]);
	else
		fprintf(f, "Level %d\nrests where\nlevel %d once\nwas...\n\n", newMap, oldMap);
}

void WritePatchHeader(FILE *f)
{
	fprintf(f, "%%ext ck1\n");
	fprintf(f, "%%version 1.31\n\n");

	// Load levels from RNDLV??.CK1
	fprintf(f, "%%patch $14D9C \"RNDLV\"\n");
	fprintf(f, "%%patch $14DA3 \"RNDLV\"\n\n");

	// Show the seed in the status screen
	fprintf(f, "%%patch $14E60 \" RANDOM SEED \"\n");
	fprintf(f, "%%patch $0FA7\t$B8 $%04XW\n", (opt_seed >> 16) & 0xFFFF);
	fprintf(f, "\t\t$BA $%04XW\n", opt_seed & 0xFFFF);
	fprintf(f, "\t\t$90 $90 $90 $90 $90 $90\n\n");
	
	if (opt_startPogo)
		fprintf(f, "%%patch $900E $01\n\n");

	if (opt_startAmmo)
		fprintf(f, "%%patch $9008 $%04XW\n\n", opt_startAmmo);
}

void WritePatchFooter(FILE *f)
{
	fprintf(f, "%%end\n");
}

void PrintBanner()
{
	printf("Keen 1 Randomiser\n");
	printf("\tv1.00a\n");
	printf("\tBy David Gow <david@davidgow.net>\n\n");
}

void PrintOptions()
{
	printf("Available options:\n");
	printf("\t/SEED <seed> -- set the random number seed.\n");
	printf("\t/? -- show this message\n");
	printf("\t/DEBUG -- show debug messages.\n");
	printf("\t/NOLEVELNAMES -- use level numbers instead of names in hints.\n");
	printf("\t/NOENEMYSHUFFLE -- don't shuffle enemy positions.\n");
	printf("\t/STARTPOGO -- start with the pogo stick.\n");
	printf("\t/STARTAMMO <n> -- start with <n> ammo.\n");
	printf("\t/EXTRAPOGO <n> -- hide <n> extra Pogo sticks around the game.\n");
}

void ParseOptions(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (!strcasecmp(argv[i], "/seed"))
		{
			opt_seed = atoi(argv[++i]);
		}
		else if (!strcasecmp(argv[i], "/nolevelnames"))
		{
			opt_useLevelNames = false;
		}
		else if (!strcasecmp(argv[i], "/noenemyshuffle"))
		{
			opt_shuffleEnemies = false;
		}
		else if (!strcasecmp(argv[i], "/startpogo"))
		{
			opt_startPogo = true;
		}
		else if (!strcasecmp(argv[i], "/startammo"))
		{
			opt_startAmmo = atoi(argv[++i]);
		}
		else if (!strcasecmp(argv[i], "/extrapogo"))
		{
			opt_extraPogo = atoi(argv[++i]);
		}
		else if (!strcasecmp(argv[i], "/debug"))
		{
			opt_debug = true;
		}
		else if (!strcasecmp(argv[i], "/?"))
		{
			PrintOptions();
			exit(0);
		}
		else
		{
			printf("Unknown argument \"%s\"\n",  argv[i]);
			PrintOptions();
			exit(1);
		}
	}
}

int main(int argc, char **argv)
{
	PrintBanner();
	// Default random seed.
	srand(time(0));
	// Clamp to 16-bit for ease of readability, and because I'm
	// not sure srand() actually accepts a 32-bit seed on all archs.
	opt_seed = rand() & 0xFFFF;
	ParseOptions(argc, argv);
	printf("Random seed: %d\n", opt_seed);
	srand(opt_seed);
	int curItemSlot = 0;

	// Add extra Pogo sticks if requested.
	K1_AddExtraPogos();

	PermuteArray(itemsPerSlot, 20);

	FILE *patchFile = fopen("RNDKEEN1.PAT", "w");
	WritePatchHeader(patchFile);

	// World map
	FILE *f = fopen("LEVEL80.CK1", "rb");
	VorticonsMap wm = VORT_LoadMap(f);
	fclose(f);
	K1_ShuffleLevelEntries(&wm);
	f = fopen("RNDLV80.CK1", "wb");
	VORT_SaveMap(&wm, f);
	fclose(f);

	for (int level = 1; level <= 16; level++)
	{
		char fname[16];
	       	sprintf(fname, "LEVEL%02d.CK1", level);
		debugf("Processing level %d (%s)\n", level, fname);
		FILE *f = fopen(fname, "rb");
		VorticonsMap vm = VORT_LoadMap(f);
		fclose(f);
		K1_MungeKeys(&vm);
		K1_MungeBlockColours(&vm);
		K1_ShuffleLollies(&vm);
		if (opt_shuffleEnemies)
			K1_ShuffleEnemies(&vm);
		while (slotsPerLevel[level-1]--)
		{
			// If we have the pogo at the start, or we have extra pogosticks,
			// don't generate hints for pogo stick locations.
			if (!(opt_startPogo || opt_extraPogo) || (itemsPerSlot[curItemSlot] != K1_T_POGOSTICK))
			{
				WriteTileHintToPatch(patchFile, itemsPerSlot[curItemSlot], level);
				if (itemsPerSlot[curItemSlot] != K1_T_GREYSKY)
					WriteMapHintToPatch(patchFile, level, mapLocations[level-1]);
			}
			K1_SetSpecialItem(&vm, itemsPerSlot[curItemSlot++], slotsPerLevel[level-1]);
		}
		sprintf(fname, "RNDLV%02d.CK1", level);
		f = fopen(fname, "wb");
		VORT_SaveMap(&vm, f);
		fclose(f);
	}

	WritePatchFooter(patchFile);
	fclose(patchFile);
	return 0;
}

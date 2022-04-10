#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <math.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>

// TODO
// Done: Give player lives. Draw a tie fighter for each live of the player. Don't respawn when you die
// Done: Immunity frames (allow player to be immune when he spawns back for a couple seconds) 
// Done: Make sure asteroids don't spawn where the player spawns (so the player is safe when starting)
// Done: New asteroid models

// Done: Give each asteroid a random angle when spawns (in create asteroid function)
// Done: Add angular velocity so they are spinning
// Done: Reset velocity on ship when death

// Done: Add menu screen
// Done: Text rendering
// Done: Toroidal collision detection
// - The collision on asteroids needs to be duplicated to all of the asteroids that are drawn. (all 9 per)
// Reset game properly on death
// Done: High scores (Writting to files)
// Multiple levels of difficulty (More spawn or they move faster)

// Review: FacingVector function (theta, cos, sin)

// Later down the road:
// Power Ups - Dropped from asteroid
// Special types of shooting (Missles that explode)
// Rotate the image of the laser and stretch it in one direction
// -> Center arguement for the SDL_RenderCopyEx (6th arguement)
// - - > SDL_RenderCopyEx(renderer, sprite.image.texture, NULL, &rect, sprite.angle + 90, NULL, SDL_FLIP_NONE);

// Function prototype
// This is what a header file is doing
float randomFloat(float min, float max);

const int RESOLUTION_X = 1200;
const int RESOLUTION_Y = 900;
const int MAX_BULLETS = 100;
const int INITIAL_ASTEROIDS = 10;
const int MAX_ASTEROIDS = 100;
const int MAX_DEPTH = 3;
const int LIVES = 1;
const int SHIPIMMUNITY = 3;
const int TOTALHIGHSCORES = 5;

double getTime() {
	return SDL_GetTicks() / 1000.0;
}

struct Vector {
	double x;
	double y;
};

struct Image {
	unsigned char* pixelData;
	SDL_Texture* texture;
	int w;
	int h;
};

Image text1;

struct Color {
	unsigned char r, g, b, a;
};

// struct is a group of variables of different types (good for refering to data of different types)
struct Sprite {
	// SDL_Rect rect;
	// SDL_Texture* texture;
	Image image;
	double angle;
	Vector position;
	// Vector stores x and y
	Vector velocity;
	int width;
	int height;
};

struct Ship {
	Sprite sprite;
	double radius;
	// ***DONE WITHOUT CHRIS***
	bool existing;
};

struct Asteroid {
	Sprite sprite;
	double radius;
	bool existing;
	// the 0 doesn't really matter
	int depth = 0;
	double angularVelocity;
};

struct Pixel {
	char r, g, b, a;
};

struct Score {
	std::string name;
	int score;
};

Asteroid asteroid[MAX_ASTEROIDS];

// Function that computes the unit vector
Vector facingDirection(double theta) {
	Vector result;
	// M_PI / 180 is converting from degrees to radians (we need to convert for SDL)
	// 90 = Pi / 2
	// The cos and sin of any angle is 1
	result.x = cos(theta * M_PI / 180);
	result.y = sin(theta * M_PI / 180);
	// Returns JUST a direction of that angle
	return result;
}

// it is a 'const' because the copy (source) doesn't change
// void is no type. It's a pointer so it's pointing to data
// You have a certain amount of data from the source that you want at the destination
void myMemcpy(void* destination, void const* source, size_t size) {
	// We want to go 1 byte at a time using char*
	char* d = (char*)destination;
	char* s = (char*)source;

	// For each byte in the source, you copy it into the dest
	for (int i = 0; i < size; i++)
	{
		// de-reference
		// all of memory is one long array
		// dereference means you are talking about the thing the pointer is pointing at, not the address itself
		// d is an address. the square bracket is saying we want to access the i variable after d.
		// Examples:
		// d[i] = s[i] is the same as *d = *s;
		// *(d + j) = *(s + i);
		// *d++ = *s++;
		/*
		*d = *s;
		d++;
		s++;
		*/

		d[i] = s[i];
	}
}

// Create after 'Sprite.' C++ compiles from top down
struct Bullet {
	Sprite sprite;
	double lifeTime;
	double radius;
};

// Enums are a list of elements (they are constants)
enum GameState {
	MENU = 0,
	GAMELOOP = 1,
};

GameState gameState = MENU;

// We want a function that creates a sprite
Sprite createSprite(Image image) {
	// {} clears the sprite to zero. Good habit
	Sprite result = {};
	SDL_QueryTexture(image.texture, NULL, NULL, &result.width, &result.height);
	// . accesses members of a composite type
	// In this case we are accessing a specific member of the sprite
	result.image = image;
	return result;
}

Color readPixel(Image image, int x, int y) {
	// This is a cast from 1 unsigned character to 4 unsigned characters
	Color* pixels = (Color*)image.pixelData;

	// The square brackets are derefencing the pointer
	// 3 * 6 + 6
	Color c = pixels[y * image.w + x];

	return c;
}

double distance(Vector a, Vector b) {
	Vector to_a = {};
	to_a.x = a.x - b.x;
	to_a.y = a.y - b.y;

	double result = sqrt(to_a.x * to_a.x + to_a.y * to_a.y);
	return result;
}

double toroidalDistance(Vector a, Vector b) {
	// We don't want it to be negative in the 
	// calculation so we take the absolute value
	double dx = abs(a.x - b.x);
	double dy = abs(a.y - b.y);

	if (dx > RESOLUTION_X / 2) {
		dx = RESOLUTION_X - dx;
	}
	if (dy > RESOLUTION_Y / 2) {
		dy = RESOLUTION_Y - dy;
	}

	double result = sqrt(dx * dx + dy * dy);
	return result;
}

double returnSpriteSize(Image image) {
	Vector center = {};
	center.x = (double) image.w / 2;
	center.y = (double) image.h / 2;

	Vector pixel = {};

	double maxDistance = 0;

	for (int i = 0; i < image.w; i++) {
		pixel.x = i;
		for (int j = 0; j < image.h; j++) {
			pixel.y = j;
			Color alphaTest = (readPixel(image, i, j));

			if (alphaTest.a > 0) {
				double distance1 = distance(center, pixel);
				if (distance1 > maxDistance) {
					maxDistance = distance1;
				}
			}
		}
	}

	return maxDistance;
}

Bullet createBullet(Image image) {
	Bullet bullet = {};
	bullet.sprite = createSprite(image);
	bullet.lifeTime = 0;
	bullet.radius = returnSpriteSize(image);
	return bullet;
}

void createAsteroid(Image image, Vector position, int depth) {
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		if (asteroid[i].existing == false) {
			asteroid[i].depth = depth;
			asteroid[i].sprite = createSprite(image);
			asteroid[i].radius = returnSpriteSize(image);

			// rand() - returns a random number
			asteroid[i].sprite.velocity.x = randomFloat(-80, 80);
			asteroid[i].sprite.velocity.y = randomFloat(-80, 80);
			asteroid[i].sprite.position = position;

			// Set a random angle for every newly created asteroid
			asteroid[i].sprite.angle = randomFloat(0, 360);

			asteroid[i].angularVelocity = randomFloat(-180, 180);

			// Set each asteroid's boolean value to true
			asteroid[i].existing = true;
			break;
		}
	}
}

void deleteAsteroids() {
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		if (asteroid[i].existing == true) {
			asteroid[i].existing = false;
		}
	}
}

Ship createShip(Image image) {
	Ship ship = {};
	ship.sprite = createSprite(image);
	ship.radius = returnSpriteSize(image);
	return ship;
}

// We want a function that draws a sprite
void drawSprite(SDL_Renderer* renderer, Sprite sprite) {
	SDL_Rect rect;
	// rect.x and y is an int. It is truncating the double when it is cast to an int -> (int)
	rect.w = sprite.width;
	rect.h = sprite.height;
	// changing the origin of the sprite's position
	rect.x = (int)sprite.position.x - rect.w / 2;
	rect.y = (int)sprite.position.y - rect.h / 2;

	// You are coping a image into another image
	SDL_RenderCopyEx(renderer, sprite.image.texture, NULL, &rect, sprite.angle + 90, NULL, SDL_FLIP_NONE);

	// This is for wrapping. You are drawing a image nine times for the wrapping (see notes)
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			SDL_Rect rectCopy = rect;
			Vector temp;
			rectCopy.x = rect.x - RESOLUTION_X + x * RESOLUTION_X;
			rectCopy.y = rect.y - RESOLUTION_Y + y * RESOLUTION_Y;
			temp.x = rectCopy.x;
			temp.y = rectCopy.y;
			SDL_RenderCopyEx(renderer, sprite.image.texture, NULL, &rectCopy, sprite.angle + 90, NULL, SDL_FLIP_NONE);
		}
	}
}
// SDL doesn't allow us to create circles. They just care about rectangles
// We just need lines to create a nice circle
void drawCircle(SDL_Renderer* renderer, Vector position, float radius) {
	// We are going through the C runtime library so we need to be talking in terms of radians
	// SDL talks in terms of degrees
	const int NUMPOINTS = 24;
	const float DELTA = (M_PI * 2) / NUMPOINTS;


	// 24 is 1 less of the number of lines we need to draw so add one
	for (int i = 0; i < NUMPOINTS + 1; i++) {
		// Where we are starting to draw
		float x1 = position.x + cos(DELTA * i) * radius;
		float y1 = position.y + sin(DELTA * i) * radius;
		// Where we are drawing to
		float x2 = position.x + cos(DELTA * (i + 1)) * radius;
		float y2 = position.y + sin(DELTA * (i + 1)) * radius;
		// Draw the line
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

}

// We want a function that draws a sprite
void drawSpriteRadius(SDL_Renderer* renderer, Sprite sprite, float radius) {
	SDL_Rect rect;
	// rect.x and y is an int. It is truncating the double when it is cast to an int -> (int)
	rect.w = sprite.width;
	rect.h = sprite.height;
	// changing the origin of the sprite's position
	rect.x = (int)sprite.position.x - rect.w / 2;
	rect.y = (int)sprite.position.y - rect.h / 2;

	// You are coping a image into another image
	SDL_RenderCopyEx(renderer, sprite.image.texture, NULL, &rect, sprite.angle + 90, NULL, SDL_FLIP_NONE);

	// This is for wrapping. You are drawing a image nine times for the wrapping (see notes)
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			SDL_Rect rectCopy = rect;
			Vector temp;
			rectCopy.x = rect.x - RESOLUTION_X + x * RESOLUTION_X;
			rectCopy.y = rect.y - RESOLUTION_Y + y * RESOLUTION_Y;
			temp.x = rectCopy.x;
			temp.y = rectCopy.y;
			SDL_RenderCopyEx(renderer, sprite.image.texture, NULL, &rectCopy, sprite.angle + 90, NULL, SDL_FLIP_NONE);
			// drawCircle(renderer, temp, radius);
		}
	}
}


Image loadFont(SDL_Renderer* renderer, const char* fileName) {
	Image result;

	int x, y, n;
	// The 4 tells stb we want 4 bytes per pixel (RGBA)
	// Stb is the library we are using to get pixel data
	// Stb is loading image data off of the disc, converting 
	// it from a compressed format to uncompressed RGBA and 
	// the giving it to you as the result of stbi_load
	// (View documentation in stb)
	// unsigned char* means you have an address to one unsigned byte
	// char is a number
	// unsigned can't be negative. signed can be negative
	// 1 byte (8 bits) is one unsigned char. 1 bit = 0,1,2 bits = 0,1,2,3 etc..
	// 2^0, 
	// unsigned = Range of 2^8 (256) is 0 to 255
	// signed = 8 bit (2^8) has 256 values with a range of -128 to 127
	// 32 bits of unsigned data is 4,294,967,296
	unsigned char* data = stbi_load(fileName, &x, &y, &n, 4);

	int totalPixels = x * y;

	unsigned char* read = data;
	for (int i = 0; i < (x * y); i++) {
		char r = read[0];
		char g = read[1];
		char b = read[2];
		char a = read[3];

		// r == 0 means it is black
		if (r == 0)
			read[3] = 0;

		// This advances the pointer to four bytes later
		read += 4;
	}

	// You are allocating vRam (video memory or ram on your video card)
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, x, y);

	int pitch;
	void* pixels;

	// A texture needs to be on the gpu
	// Locking the texture gives us a position in memory
	// While the texture is locked, you can write to the texture
	int lockTexture = SDL_LockTexture(texture, NULL, &pixels, &pitch);

	// There are four channels per pixel (RGBA) so we multiply it by four
	myMemcpy(pixels, data, (x * y * 4));

	// prevents leaking memory (memory is being leaked
	// we no longer want to free the memory
	// stbi_image_free(data);

	// When the texture is unlocked, you can no longer write to it
	SDL_UnlockTexture(texture);

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

	result.pixelData = data;
	result.texture = texture;
	result.w = x;
	result.h = y;

	return result;
}

// We make the type const char* because that is a type of any quoted text
Image loadImage(SDL_Renderer* renderer, const char* fileName) {
	Image result;

	int x, y, n;
	// The 4 tells stb we want 4 bytes per pixel (RGBA)
	// Stb is the library we are using to get pixel data
	// Stb is loading image data off of the disc, converting 
	// it from a compressed format to uncompressed RGBA and 
	// the giving it to you as the result of stbi_load
	// (View documentation in stb)
	// unsigned char* means you have an address to one unsigned byte
	// char is a number
	// unsigned can't be negative. signed can be negative
	// 1 byte (8 bits) is one unsigned char. 1 bit = 0,1,2 bits = 0,1,2,3 etc..
	// 2^0, 
	// unsigned = Range of 2^8 (256) is 0 to 255
	// signed = 8 bit (2^8) has 256 values with a range of -128 to 127
	// 32 bits of unsigned data is 4,294,967,296
	unsigned char* data = stbi_load(fileName, &x, &y, &n, 4);

	// You are allocating vRam (video memory or ram on your video card)
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, x, y);

	int pitch;
	void* pixels;

	// A texture needs to be on the gpu
	// Locking the texture gives us a position in memory
	// While the texture is locked, you can write to the texture
	int lockTexture = SDL_LockTexture(texture, NULL, &pixels, &pitch);

	// There are four channels per pixel (RGBA) so we multiply it by four
	myMemcpy(pixels, data, (x * y * 4));

	// prevents leaking memory (memory is being leaked
	// we no longer want to free the memory
	// stbi_image_free(data);

	// When the texture is unlocked, you can no longer write to it
	SDL_UnlockTexture(texture);

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

	result.pixelData = data;
	result.texture = texture;
	result.w = x;
	result.h = y;

	return result;
}

void updateSpritePosition(Sprite* sprite, double delta) {
	// This is where the player exists in the world at this frame
	// -> is short hand for dereferencing a member out of a pointer
	sprite->position.x += sprite->velocity.x * delta;
	sprite->position.y += sprite->velocity.y * delta;

	// Change from if to while incase the velocity exceeds the resolution
	while (sprite->position.x > RESOLUTION_X)
	{
		sprite->position.x -= RESOLUTION_X;
	}
	while (sprite->position.x < 0)
	{
		sprite->position.x += RESOLUTION_X;
	}
	while (sprite->position.y > RESOLUTION_Y)
	{
		sprite->position.y -= RESOLUTION_Y;
	}
	while (sprite->position.y < 0)
	{
		sprite->position.y += RESOLUTION_Y;
	}
}

float randomFloat(float min, float max) {
	float base = (float)rand() / (float)RAND_MAX;
	float range = max - min;
	float newRange = range * base + min;
	return newRange;
}
// Done without Chris
void wrappingCoor(float inputX, float inputY, float& outputX, float& outputY) {
	outputX = inputX;
	outputY = inputY;
	if (inputX < 0.0f)
		outputX = inputX + (float)RESOLUTION_X;
	if (inputY >= (float)RESOLUTION_Y);
		outputY = inputY - (float)RESOLUTION_Y;
}

// Text rendering
void drawString(SDL_Renderer* renderer, std::string string, int positionX, int positionY, bool center = true) {
	SDL_Rect sourceRect;
	sourceRect.w = 7;
	sourceRect.h = 9;
	int stringSize = string.size();
	int spacing = 0;
	for (int i = 0; i < stringSize; i++) {
		char glyph = string[i];
		int index = glyph - ' ';
		int row = index / 18;
		int col = index % 18;
		sourceRect.x = (col * 7);
		sourceRect.y = (row * 9);

		int stringWidth = stringSize * (7 * 4);

		SDL_Rect destinationRect;
		if (center) {
			destinationRect.x = (positionX + spacing) - (stringWidth / 2);
		} 
		else {
			destinationRect.x = (positionX + spacing);
		}

		destinationRect.y = positionY - ((9 * 4) / 2);
		destinationRect.w = 7 * 4;
		destinationRect.h = 9 * 4;

		SDL_RenderCopy(renderer, text1.texture, &sourceRect, &destinationRect);
		spacing += destinationRect.w;
	}

}

std::string frameHotName;
std::string nextFrameHotName;
bool mouseDownThisFrame;

bool button(SDL_Renderer* renderer, std::string text, int positionX, int positionY, int w, int h) {
	SDL_Rect buttonArea;
	buttonArea.x = positionX - (w / 2);
	buttonArea.y = positionY - (h / 2);
	buttonArea.w = w;
	buttonArea.h = h;
	SDL_Rect textArea;
	//textArea.x = 
	drawString(renderer, text, positionX, positionY);
	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	
	int x, y;
	// Think of mouse like an array
	Uint32 mouse = SDL_GetMouseState(&x, &y);
	bool buttonPressed = 0;

	bool wasHot = frameHotName == text;

	if (x >= buttonArea.x && x <= (buttonArea.x + buttonArea.w) 
		&& y >= buttonArea.y && y <= (buttonArea.y + buttonArea.h)) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		//if (mouse & SDL_BUTTON_LMASK) {
		//	buttonPressed = 1;
		//}
		if (mouseDownThisFrame) {
			nextFrameHotName = text;
		}
		else if (wasHot && !(mouse & SDL_BUTTON_LMASK)) {
			buttonPressed = 1;
		}
		else
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		}
	}
	if (wasHot && (mouse & SDL_BUTTON_LMASK)) {
		nextFrameHotName = text;
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	}

	SDL_RenderDrawRect(renderer, &buttonArea);
	return buttonPressed;
}

void spawnAsteroids(Image *asteroidImage) {
	for (int i = 0; i < INITIAL_ASTEROIDS; i++) {
		// This will spawn out of bounds (Y)
		double range = randomFloat((RESOLUTION_X / 5), RESOLUTION_X / 2);
		// Random distance
		Vector asteroidPosition = facingDirection(randomFloat(0, 360));


		asteroidPosition.x *= range;
		asteroidPosition.y *= range;

		asteroidPosition.x += RESOLUTION_X / 2;
		asteroidPosition.y += RESOLUTION_Y / 2;

		createAsteroid(asteroidImage[0], asteroidPosition, 0);
	}
}

std::vector<Score> readHighScores() {
	// rb (read binary)
	// When you open a file for writting, you are locking the file. 
	// This is so other programs can't write to the file. You need to close it. 
	FILE* file = fopen("scores.txt", "rb");
	std::vector<Score> scores;
	if (file != nullptr) {
		// fscanf tells the system what the content 
		// is that we are expecting to read back
		// %d = int , %s = string
		int score = 0;
		// passing an array is like passing a pointer
		char string[128] = "";
		while (fscanf(file, "%d %s", &score, string) == 2) {
			Score score1;
			score1.name = string;
			score1.score = score;
			scores.push_back(score1);
		}
		fclose(file);
	}
	return scores;
}

bool scoreCompare(const Score& score1, const Score& score2) {
	return (score1.score > score2.score);
}

std::vector<Score> highScores;

void addHighScore(std::string name, int score) {
	std::vector<Score> scores = readHighScores();
	Score paramValues;
	paramValues.name = name;
	paramValues.score = score;
	scores.push_back(paramValues);
	// scores.begin(), scores.end() is the range of elements (all the elements)
	// Lambda - inline function declaration
	// scoreCompare is a function pointer (you are giving it the address of the function
	std::sort(scores.begin(), scores.end(), scoreCompare);
	if (scores.size() > TOTALHIGHSCORES) {
		// Truncate
		scores.resize(TOTALHIGHSCORES);
	}
	highScores = scores;
	// wb = write binary (erases everything in the file)
	FILE* file = fopen("scores.txt", "wb");
	if (file != nullptr) {
		for (size_t i = 0; i < scores.size(); i++) {
			fprintf(file, "%d %s\n", scores[i].score, scores[i].name.c_str());
		}
		fclose(file);
	}
}

bool drawEngine = false;
bool running = true;
bool angleA = false;
bool angleD = false;
bool backwards = false;
bool fire = false;
double fireTime = 0;
double shipImmunity = 0;
int totalLives = LIVES;
int score = 0;
// Global array of 6 bytes
// Stores 5 characters and 1 null terminator
char scoreInput[16];

Ship player;

void resetVariables(Image *asteroidImage) {
	drawEngine = false;
	running = true;
	angleA = false;
	angleD = false;
	backwards = false;
	fire = false;
	fireTime = 0;
	shipImmunity = 0;
	player.existing = 1;
	totalLives = LIVES;
	scoreInput[0] = 0;
	score = 0;
	

	player.sprite.position.x = RESOLUTION_X / 2;
	player.sprite.position.y = RESOLUTION_Y / 2;

	deleteAsteroids();
	spawnAsteroids(&asteroidImage[0]);
}


int main(int argc, char** argv) {

	// std::vector<Score> resultVector = readHighScores();
	//addHighScore("Michael", 2000);
	//addHighScore("Chris", 2001);
	//addHighScore("John", 1999);

	printf("Hello World");
	//showmenu();
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Asteroid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RESOLUTION_X, RESOLUTION_Y, SDL_WINDOW_SHOWN);

	// -1 = default gpu
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	Image ship1 = loadImage(renderer, "Ship_4_TyFighter_64x64.png");
	Image asteroidImages[MAX_DEPTH];
	asteroidImages[0] = loadImage(renderer, "Asteroid_4_128x128.png");
	asteroidImages[1] = loadImage(renderer, "Asteroid_4.1_128x128.png");
	asteroidImages[2] = loadImage(renderer, "Asteroid_4.2_128x128.png");
	Image ship1Engine = loadImage(renderer, "Ship_4_TyFighter_64x64_Engine.png");
	Image map1 = loadImage(renderer, "Map_6.png");
	Image bullet1 = loadImage(renderer, "Bullet_6_TF.png");
	Image gameOver = loadImage(renderer, "GameOver_1.png");
	text1 = loadFont(renderer, "Text_1.png");

	// Create ship
	Ship player = createShip(ship1);

	// Draw lives like you drew the fonts. That way, you don't have to have a 
	// array of ships. You can just have a variable that draws the amount 
	// of ships you want displayed as lives.
	Ship lives[LIVES];
	for (int i = 0; i < LIVES; i++) {
		lives[i] = createShip(ship1);
	}

	Sprite engine = createSprite(ship1Engine);

	Bullet bullet[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullet[i] = createBullet(bullet1);
	}

	spawnAsteroids(&asteroidImages[0]);
	// 10 is how many asteroids spawn initially
	/*
	for (int i = 0; i < 10; i++) {
		// This will spawn out of bounds (Y)
		double range = randomFloat((RESOLUTION_X / 5), RESOLUTION_X / 2);
		Vector asteroidPosition = facingDirection(randomFloat(0, 360));

		asteroidPosition.x *= range;
		asteroidPosition.y *= range;

		asteroidPosition.x += RESOLUTION_X / 2;
		asteroidPosition.y += RESOLUTION_Y / 2;

		createAsteroid(asteroidImages[0], asteroidPosition, 0);

		
		//do {
		//	vector.x = randomFloat(0, RESOLUTION_X);
		//} while (vector.x >= 500 && vector.x <= 700);

		//do {
		//	vector.y = randomFloat(0, RESOLUTION_Y);
		//} while (vector.y >= 100 && vector.y <= 800);

		//createAsteroid(asteroidImages[0], vector, 0);
		
	}
	*/


	double lastFrameTime = getTime();

	player.sprite.position.x = RESOLUTION_X / 2;
	player.sprite.position.y = RESOLUTION_Y / 2;

	// ***DONE WITHOUT CHRIS***
	int spacing = 0;
	for (int i = 0; i < LIVES; i++) {
		// Overflow
		lives[i].sprite.position.x = (RESOLUTION_X / 16) + spacing;
		lives[i].sprite.position.y = (RESOLUTION_Y / 16);
		spacing += 75;
		lives[i].existing = true;
	}

	highScores = readHighScores();

	// Menu state & game state with a function that swaps the state
	while (running) {
		mouseDownThisFrame = false;
		SDL_Event event = {};		
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_TEXTINPUT:
				if (totalLives <= 0) {
					// strlen - length of current string up to nullptr
					// capacity - sizeof() - size of the variable in bytes
					for (int i = 0; i < strlen(event.text.text); i++) {
						if (event.text.text[i] == ' ') {
							// Goes to the next iteration of the for loop
							continue;
						}
						// Check if we have enough memory
						// + 2 refers to the new char position and the null terminator
						if (strlen(scoreInput) + 2 <= sizeof(scoreInput)) {
							int tempLength = strlen(scoreInput);
							scoreInput[tempLength] = event.text.text[i];
							// 0 = null terminator
							scoreInput[tempLength + 1] = 0;
						}
					}
					printf("Text input: %s\n", event.text.text);
					printf("High score array: %s\n", scoreInput);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == 1) {
					mouseDownThisFrame = true;
				}
				break;
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_w:
					drawEngine = false;
					break;
				case SDLK_a:
					angleA = false;
					break;
				case SDLK_d:
					angleD = false;
					break;
				case SDLK_s:
					backwards = false;
					break;
				case SDLK_SPACE:
					fire = false;
					break;
				}
				break;
			case SDL_KEYDOWN:
				if (totalLives > 0) {
					switch (event.key.keysym.sym) {
					case SDLK_a:
						angleA = true;
						break;
					case SDLK_d:
						angleD = true;
						break;
					case SDLK_w:
						drawEngine = true;
						break;
					case SDLK_s:
						backwards = true;
						break;
					case SDLK_SPACE:
						fire = true;
						break;
					}
					break;
				}
				else {
					switch (event.key.keysym.sym) {
					case SDLK_BACKSPACE:
						if (strlen(scoreInput) > 0)
							scoreInput[strlen(scoreInput) - 1] = 0;
						break;
					}
				}
			}
		}

		frameHotName = nextFrameHotName;
		nextFrameHotName = "";

		if (gameState == MENU) {
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, map1.texture, NULL, NULL);
			//drawString(renderer, "Menu", RESOLUTION_X / 2, RESOLUTION_Y / 2);
			drawString(renderer, "Asteroids", RESOLUTION_X / 2, RESOLUTION_Y / 3);
			if (button(renderer, "Play", RESOLUTION_X / 2, RESOLUTION_Y / 2 + 50, 135, 65)) {
				gameState = GAMELOOP;
				resetVariables(&asteroidImages[0]);
			}
			if (button(renderer, "Quit", RESOLUTION_X / 2, RESOLUTION_Y / 2 + 125, 135, 65)) {
				running = false;
			}
			
			// highScores
			for (size_t i = 0; i < highScores.size(); i++) {
				std::string displayScore = highScores[i].name + ": " + std::to_string(highScores[i].score);
				drawString(renderer, displayScore, 10, 50 + (i * 75), false);
			}
			SDL_RenderPresent(renderer);
		}

		else if (gameState == GAMELOOP) {
			double currentTime = getTime();

			double deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			fireTime -= deltaTime;
			shipImmunity -= deltaTime;

			if (angleA) {
				// Turn speed is not based on seconds
				player.sprite.angle -= 300 * deltaTime;
			}
			if (angleD) {
				player.sprite.angle += 300 * deltaTime;
			}

			double acceleration = 100;
			if (backwards) {
				Vector direction = facingDirection(player.sprite.angle);
				player.sprite.velocity.x -= direction.x * deltaTime * acceleration;
				player.sprite.velocity.y -= direction.y * deltaTime * acceleration;
			}
			if (drawEngine) {
				Vector direction = facingDirection(player.sprite.angle);
				player.sprite.velocity.x += direction.x * deltaTime * acceleration;
				player.sprite.velocity.y += direction.y * deltaTime * acceleration;
			}

			updateSpritePosition(&player.sprite, deltaTime);

			for (int i = 0; i < MAX_BULLETS; i++) {
				updateSpritePosition(&bullet[i].sprite, deltaTime);
			}

			for (int i = 0; i < MAX_ASTEROIDS; i++) {
				updateSpritePosition(&asteroid[i].sprite, deltaTime);

				asteroid[i].sprite.angle += asteroid[i].angularVelocity * deltaTime;
			}

			if (fire && fireTime <= 0 && totalLives > 0) {
				for (int i = 0; i < MAX_BULLETS; i++) {
					if (bullet[i].lifeTime <= 0) {
						bullet[i].lifeTime = 4;
						bullet[i].sprite.angle = player.sprite.angle;
						bullet[i].sprite.position = player.sprite.position;
						bullet[i].sprite.velocity = player.sprite.velocity;
						Vector bulletDirection = facingDirection(bullet[i].sprite.angle);
						bullet[i].sprite.velocity.x += bulletDirection.x * 2000;
						bullet[i].sprite.velocity.y += bulletDirection.y * 2000;
						fireTime = 0.2;
						break;
					}
				}
			}

			// Bullet Collision with asteroids
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullet[i].lifeTime > 0) {
					for (int j = 0; j < MAX_ASTEROIDS; j++) {
						if (asteroid[j].existing == false) {
							continue;
						}
						double distanceBetween = toroidalDistance(bullet[i].sprite.position, asteroid[j].sprite.position);
						double radiusSum = bullet[i].radius + asteroid[j].radius;
						if (distanceBetween < radiusSum) {
							bullet[i].lifeTime = 0;
							// Delete the asteroid when there is a collision
							score += 1;
							if (asteroid[j].depth < MAX_DEPTH - 1) {
								for (int x = 0; x < 3; x++) {
									Vector vector = asteroid[j].sprite.position;

									// * 0.5 tightens up the spawn radius
									vector.x += randomFloat(-asteroid[j].radius, asteroid[j].radius) * 0.5;
									vector.y += randomFloat(-asteroid[j].radius, asteroid[j].radius) * 0.5;

									createAsteroid(asteroidImages[asteroid[j].depth + 1], vector, asteroid[j].depth + 1);
								}
							}
							asteroid[j].existing = false;
							goto here;
						}
					}
				}
			here:;
			}

			// Ship Collision with asteroids
			if (shipImmunity <= 0) {
				for (int j = 0; j < MAX_ASTEROIDS; j++) {
					if (asteroid[j].existing == false) {
						continue;
					}
					double distanceBetween = toroidalDistance(player.sprite.position, asteroid[j].sprite.position);
					double radiusSum = player.radius + asteroid[j].radius;
					if (distanceBetween < radiusSum) {
						player.sprite.position.x = RESOLUTION_X / 2;
						player.sprite.position.y = RESOLUTION_Y / 2;
						if (totalLives > 0) {
							lives[totalLives - 1].existing = false;
						}
						totalLives -= 1;
						shipImmunity = SHIPIMMUNITY;
						// ***DONE WITHOUT CHRIS***
						player.sprite.velocity.x = 0;
						player.sprite.velocity.y = 0;
					}
				}
			}


			SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
			// Clear what we are drawing to
			// Anything done before render clear gets erased
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, map1.texture, NULL, NULL);

			// What we are giving it here is a C string. C strings automatically 
			// convert to C++ string

			// SDL_RenderCopy(renderer, asteroid1, NULL, NULL);
			for (int i = 0; i < MAX_ASTEROIDS; i++) {
				// Check to see if the boolean value is true when the asteroid was created. If it was, draw it.
				if (asteroid[i].existing == true) {
					drawSpriteRadius(renderer, asteroid[i].sprite, asteroid[i].radius);
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
					drawCircle(renderer, asteroid[i].sprite.position, asteroid[i].radius);
				}
			}

			// Draw engine
			if (drawEngine) {
				// Match the player's movement
				engine.position.x = player.sprite.position.x;
				engine.position.y = player.sprite.position.y;

				engine.angle = player.sprite.angle;
				// Engine
				drawSprite(renderer, engine);
			}

			// Draw bullet
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullet[i].lifeTime > 0) {
					drawSprite(renderer, bullet[i].sprite);
					bullet[i].lifeTime -= deltaTime;
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
					drawCircle(renderer, bullet[i].sprite.position, bullet[i].radius);
				}
			}

			// SDL_RenderCopy(renderer, ship1, NULL, NULL);
			// Again, this is just deleting the drawing of the shit and not the rendering of the ship.
			if (totalLives > 0) {
				drawSprite(renderer, player.sprite);
			}

			if (shipImmunity > 0) {
				if (totalLives > 0) {
					SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
					drawCircle(renderer, player.sprite.position, player.radius);
				}
			}

			if (shipImmunity <= 0) {
				if (totalLives > 0) {
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
					drawCircle(renderer, player.sprite.position, player.radius);
				}
			}

			// Lives
			// I am still creating all the images, but I am just removing the drawing. I'm not entirely sure how to remove the
			// created sprite. But this was still works. I created a int value (totalLives) above to count the lives.
			// Chris would just draw one sprite, three times. (If there is a power up that gives an extra life, it would overflow the array)
			for (int i = 0; i < LIVES; i++) {
				if (lives[i].existing == true) {
					drawSprite(renderer, lives[i].sprite);
				}
			}

			// Game Over sign
			if (totalLives <= 0) {
				player.sprite.position.x = RESOLUTION_X / 2;
				player.sprite.position.y = RESOLUTION_Y / 2;
				player.sprite.velocity.x = 0;
				player.sprite.velocity.y = 0;

				drawString(renderer, "Enter your name: ", RESOLUTION_X / 2, (RESOLUTION_Y / 2) - 225);
				drawString(renderer, scoreInput, RESOLUTION_X / 2, (RESOLUTION_Y / 2) - 150);
				if (button(renderer, "Submit high score", RESOLUTION_X / 2, (RESOLUTION_Y / 2) - 75, 490, 65)) {
					addHighScore(scoreInput, score);
					gameState = MENU;
				}
				drawString(renderer, "Game Over", RESOLUTION_X / 2, RESOLUTION_Y / 2);
				if (button(renderer, "Return to Menu", RESOLUTION_X / 2, (RESOLUTION_Y / 2) + 75, 410, 65)) {
					gameState = MENU;
				}
			}
			std::string scoreString = "Score: ";
			scoreString += std::to_string(score);
			drawString(renderer, scoreString, RESOLUTION_X - 150, 20);
			// RenderPresent is locking your ship's movement to the computer's refreshrate
			SDL_RenderPresent(renderer);

		}

		// When I died, I kept decrementing the array 
		// when a asteroid hit my dead ship.
		// size_t offset = offsetoffsetof(Ship, existing);
		// bool* p = &lives[-1].existing;
		}
	return 0;
}
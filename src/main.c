#ifndef WINDOWS
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#else
#include <windows.h>
#endif
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define FPS 40
#define PI 3.14159265

typedef struct {
	int x;
	int y;
} coord;

typedef struct {
	float x;
	float y;
} vec2;

typedef struct {
	float x;
	float y;
	float z;
} vec3;

typedef struct {
	vec3 start;
	vec3 end;
} line_t;

typedef struct {
	coord start;
	coord end;
} line_2d_t;

typedef struct {
	char* buffer;
	size_t buffer_size;
} screen_buffer_t;

coord screen_dimensions = {0};
screen_buffer_t screen_buffer = {0};

void update_screen_dimensions();
void update_screen_buffer();
size_t calculate_buffer_size();
void render();
void draw_screen_buffer();

coord map_rel_to_abs(vec2 pos);
vec2 project_relative(vec2 pos, float z);

void draw_char(char character, coord pos);
void draw_line_absolute(char character, coord start, coord end);
void draw_line(char character, line_t line);

vec3 rotate_around_axis(vec3 point, float angle_deg, vec3 axis);
line_t rotate_line(line_t line, float angle_deg, vec3 axis);
line_t move_line(line_t line, vec3 offset);
void normalize(vec3* point);

line_2d_t fix_aspect_ratio(line_2d_t line);

void sigint_handle(int signal);

int     cube_vertices_count     = 8;
int     triangle_vertices_count = 4;
vec3*   cube_vertices;
vec3*   triangle_vertices;

int     cube_lines_count     = 12;
int     triangle_lines_count = 6;
line_t* cube_lines;
line_t* triangle_lines;

vec3 rotation_pivot= (vec3){0.0f, 0.0f, 4.0f};

size_t counter;

#ifdef WINDOWS
HANDLE hConsole;
COORD buffer_size;
CONSOLE_SCREEN_BUFFER_INFO csbi;
#endif

int main(void) {
	#ifdef WINDOWS
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	buffer_size = csbi.dwSize;
	#endif
	counter = 0;
	signal(SIGINT, sigint_handle);
	screen_buffer.buffer = (char*)malloc(100000);
	screen_buffer.buffer_size = 100000;

	cube_vertices  = (vec3*)malloc(sizeof(vec3)*cube_vertices_count);
	triangle_vertices = (vec3*)malloc(sizeof(vec3)*triangle_vertices_count);
	// front
	cube_vertices[0] = (vec3){-1.0f,  -1.0f, 3.0f};
	cube_vertices[1] = (vec3){-1.0f,   1.0f, 3.0f};
	cube_vertices[2] = (vec3){ 1.0f,   1.0f, 3.0f};
	cube_vertices[3] = (vec3){ 1.0f,  -1.0f, 3.0f};

	// back
	cube_vertices[4] = (vec3){-1.0f, -1.0f, 5.0f};
	cube_vertices[5] = (vec3){-1.0f,  1.0f, 5.0f};
	cube_vertices[6] = (vec3){ 1.0f,  1.0f, 5.0f};
	cube_vertices[7] = (vec3){ 1.0f, -1.0f, 5.0f};

	cube_lines = (line_t*)malloc(sizeof(line_t)*cube_lines_count);
	triangle_lines = (line_t*)malloc(sizeof(line_t)*triangle_lines_count);
	// front
	cube_lines[0] = (line_t){cube_vertices[0], cube_vertices[1]};
	cube_lines[1] = (line_t){cube_vertices[1], cube_vertices[2]};
	cube_lines[2] = (line_t){cube_vertices[2], cube_vertices[3]};
	cube_lines[3] = (line_t){cube_vertices[3], cube_vertices[0]};

	// back
	cube_lines[4] = (line_t){cube_vertices[4], cube_vertices[5]};
	cube_lines[5] = (line_t){cube_vertices[5], cube_vertices[6]};
	cube_lines[6] = (line_t){cube_vertices[6], cube_vertices[7]};
	cube_lines[7] = (line_t){cube_vertices[7], cube_vertices[4]};

	// sides
	cube_lines[8] = (line_t){cube_vertices[0], cube_vertices[4]};
	cube_lines[9] = (line_t){cube_vertices[1], cube_vertices[5]};
	cube_lines[10] = (line_t){cube_vertices[2], cube_vertices[6]};
	cube_lines[11] = (line_t){cube_vertices[3], cube_vertices[7]};

	triangle_vertices[0] = (vec3){-1.0f,  1.0f,  0.0f}; // top-left
	triangle_vertices[1] = (vec3){ 1.0f,  1.0f,  0.0f}; // top_right
	triangle_vertices[2] = (vec3){ 0.0f, -1.0f, -1.0f}; // bottom_back
	triangle_vertices[3] = (vec3){ 0.0f, -1.0f,  1.0f}; // bottom_front
	
	triangle_lines[0] = (line_t){triangle_vertices[0], triangle_vertices[1]};
	triangle_lines[1] = (line_t){triangle_vertices[0], triangle_vertices[2]};
	triangle_lines[2] = (line_t){triangle_vertices[0], triangle_vertices[3]};
	triangle_lines[3] = (line_t){triangle_vertices[1], triangle_vertices[2]};
	triangle_lines[4] = (line_t){triangle_vertices[1], triangle_vertices[3]};
	triangle_lines[5] = (line_t){triangle_vertices[2], triangle_vertices[3]};

	update_screen_dimensions();
	for (int i = 0; i < screen_dimensions.y; ++i) {
		printf("\n");	
	}
	while (true) {
		update_screen_dimensions();
		update_screen_buffer();
		render();
		draw_screen_buffer();
		++counter;
		usleep(1000000/FPS);
	}
}


void update_screen_dimensions() {
	#ifndef WINDOWS
	struct winsize winsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
	screen_dimensions = (coord){ winsize.ws_col - 1, winsize.ws_row};
	#else
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

	if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
		int columns = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left;
		int rows = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top;
		screen_dimensions = (coord){ columns, rows };
	} else {
		perror("Can't get terminal size.");
		exit(1);
	}
	#endif
}

void update_screen_buffer() {
	size_t should_be = calculate_buffer_size();
	if (screen_buffer.buffer_size != should_be) {
		screen_buffer.buffer_size = should_be;
	}
	memset(screen_buffer.buffer, ' ', should_be);
	for (int i = 0; i < screen_dimensions.y; ++i) {
		screen_buffer.buffer[i * (screen_dimensions.x + 1)] = '\n';
	}
	screen_buffer.buffer[should_be - 1] = 0;
}

size_t calculate_buffer_size() {
	return ((screen_dimensions.x + 1) * (screen_dimensions.y - 2)) + 1;
}

void render() {
	// cubes
	rotation_pivot = (vec3){0.0f, 0.0f, 4.0f};
	for (int i = 0; i < cube_lines_count; ++i) {
		line_t line;
	
		// cube 1	
		line = cube_lines[i];
		line = rotate_line(line, counter*6, (vec3){0.0f, 0.4f, 1.0f});
		line = move_line(line, (vec3){
			cos((float)counter / 20) * 4,
			sin((float)counter / 20) * 4,
			cos((float)counter / 20) * 3 + 3
		});
		draw_line('a', line);

		// cube 2
		line = cube_lines[i];
		line = rotate_line(line, counter*6, (vec3){0.0f, 1.0f, 1.0f});
		line = move_line(line, (vec3){
			cos(((float)counter + 80) / 20) * 4,
			sin(((float)counter + 80) / 20) * 4, 
			cos((float)counter / 20) * 3 + 3
		});
		draw_line('a', line);
	}

	// triangles
	rotation_pivot = (vec3){0.5f, 0.5f, 3.0f};
	for (int i = 0; i < triangle_lines_count; ++i) {
		line_t line;
	
		// cube 1	
		line = triangle_lines[i];
		line = rotate_line(line, counter*6, (vec3){0.0f, 1.0f, 3.0f});
		line = move_line(line, (vec3){
			0,
			0,
			sin((float)counter / 60) * 10 + 6
		});
		draw_line('a', line);
	}
	char* text = "by @davidSimek";
	for (int i = 0; i < strlen(text); ++i) {
		draw_char(text[i], (coord){i + 2,screen_dimensions.y - 3});
	}
}

void draw_screen_buffer() {
	#ifndef WINDOWS
	printf("\x1b[2J\x1b[H");
	#else
	DWORD consoleSize, charsWritten;
	COORD topLeft = { 0, 0 };
	consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacter(hConsole, (TCHAR)' ', consoleSize, topLeft, &charsWritten);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, topLeft, &charsWritten);
	SetConsoleCursorPosition(hConsole, topLeft);
	#endif
	puts(screen_buffer.buffer);
}


void draw_char(char character, coord pos) {
	if (pos.x > screen_dimensions.x || pos.y > screen_dimensions.y || pos.x < 0 || pos.y < 0) {
		return;
	}
	screen_buffer.buffer[(screen_dimensions.x + 1) * pos.y + pos.x] = character;
}

void draw_line_absolute(char character, coord start, coord end) {
	start = (coord){ start.x * 2, start.y };
	end   = (coord){ end.x * 2,   end.y   };

	int dx = end.x - start.x; 
	int dy = end.y - start.y; 
  
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy); 
  
	float x_inc = dx / (float)steps; 
	float y_inc = dy / (float)steps; 
  
	float x = start.x; 
	float y = start.y; 
	for (int i = 0; i <= steps; i++) { 
		draw_char(character, (coord){ (int)x, (int)y });
		x += x_inc;
		y += y_inc;
	} 
} 

void draw_line(char character, line_t line) {
	line_2d_t new_line;
	new_line.start = map_rel_to_abs(project_relative((vec2){line.start.x, line.start.y}, line.start.z));
	new_line.end   = map_rel_to_abs(project_relative((vec2){line.end.x, line.end.y}, line.end.z));
	new_line = fix_aspect_ratio(new_line);
	draw_line_absolute('a', new_line.start, new_line.end);
}

coord map_rel_to_abs(vec2 pos) {
	int mx = (int)((pos.x + 1.0f) / 2.0f * (float)screen_dimensions.x) / 2;
	int my = (int)((pos.y + 1.0f) / 2.0f * (float)screen_dimensions.y);
	return (coord){mx, my};
}

void sigint_handle(int signal) {
	#ifndef WINDOWS
	printf("\x1b[2J\x1b[H");
	#else
	system("cls");
	#endif
	free(screen_buffer.buffer);
	free(cube_vertices);
	free(cube_lines);
	exit(0);
}

vec2 project_relative(vec2 pos, float z) {
	if (z == 0)
		return (vec2){0.0f, 0.0f};
	else
		return (vec2){pos.x/z, pos.y/z};
}


void normalize(vec3* point) {
    float length = sqrt(point->x * point->x + point->y * point->y + point->z * point->z);
    point->x /= length;
    point->y /= length;
    point->z /= length;
}

vec3 rotate_around_axis(vec3 point, float angle_deg, vec3 axis) {
    float angle_rad = angle_deg * (PI / 180.0);  // Convert degrees to radians
    float cos_theta = cos(angle_rad);
    float sin_theta = sin(angle_rad);
    point.x -= rotation_pivot.x;
    point.y -= rotation_pivot.y;
    point.z -= rotation_pivot.z;
    
    normalize(&axis);
    
    vec3 rotated;

    // Rodrigues' rotation formula
    rotated.x = point.x * cos_theta + (1 - cos_theta) * (axis.x * (axis.x * point.x + axis.y * point.y + axis.z * point.z))
                + sin_theta * (axis.y * point.z - axis.z * point.y);
    
    rotated.y = point.y * cos_theta + (1 - cos_theta) * (axis.y * (axis.x * point.x + axis.y * point.y + axis.z * point.z))
                + sin_theta * (axis.z * point.x - axis.x * point.z);
    
    rotated.z = point.z * cos_theta + (1 - cos_theta) * (axis.z * (axis.x * point.x + axis.y * point.y + axis.z * point.z))
                + sin_theta * (axis.x * point.y - axis.y * point.x);

    rotated.x += rotation_pivot.x;
    rotated.y += rotation_pivot.y;
    rotated.z += rotation_pivot.z;
    return rotated;
}

line_t rotate_line(line_t line, float angle_deg, vec3 axis) {
	line.start = rotate_around_axis(line.start, angle_deg, axis);
	line.end   = rotate_around_axis(line.end, angle_deg, axis);
	return line;
}


line_t move_line(line_t line, vec3 offset) {
	line.start.x += offset.x;
	line.start.y  += offset.y;
	line.start.z += offset.z;
	line.end.x   += offset.x;
	line.end.y   += offset.y;
	line.end.z   += offset.z;
	return line;
}


line_2d_t fix_aspect_ratio(line_2d_t line) {
	if (screen_dimensions.x == 0 || screen_dimensions.y == 0)
		return line;
	if (screen_dimensions.x > screen_dimensions.y) {
		float ratio = screen_dimensions.y/screen_dimensions.x;
		if (ratio == 0)
			return line;
		line.start.x /= ratio; 
		line.end.x   /= ratio;
		line.start.y /= ratio;
		line.end.y   /= ratio;
	} else {
		float ratio = screen_dimensions.x/screen_dimensions.y;
		if (ratio == 0)
			return line;
		line.start.x /= ratio;
		line.end.x   /= ratio;
		line.start.y /= ratio;
		line.end.y   /= ratio;
	}
}

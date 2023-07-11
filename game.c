#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M_PI 3.14109265358979323846

double circle_vector_x, circle_vector_y;
int center_x = 160, center_y = 100;
int rectangle_x_com = 175, rectangle_y_com = 235;
int radius = 10;
volatile int pixel_buffer_start = 0xc8000000;
int width = 60, height = 5;
volatile int *led_button = (0xff200000);
volatile int *player_control = (0xff200040);
int game_score = 0;
int game_ov = 0 ; 
void swap(int *x, int *y)
{
    int temp = *x;
    *x = *y;
    *y = temp;
}

double pow_new(double x, int n)
{
    if (n == 0)
    {
        return 1.0;
    }
    else if (n % 2 == 0)
    {
        double y = pow_new(x, n / 2);
        return y * y;
    }
    else
    {
        double y = pow_new(x, (n - 1) / 2);
        return x * y * y;
    }
    return 1;
}

double sqrt_new(double x)
{
    if (x <= 0)
        return 0; // if negative number throw an exception?
    int exp = 0;
    x = frexp(x, &exp); // extract binary exponent from x
    if (exp & 1)
    { // we want exponent to be even
        exp--;
        x *= 2;
    }
    double y = (1 + x) / 2; // first approximation
    double z = 0;
    while (y != z)
    { // yes, we CAN compare doubles here!
        z = y;
        y = (y + x / y) / 2;
    }
    return ldexp(y, exp / 2); // multiply answer by 2^(exp/2)
}

double ABS(double x)
{
    if (x >= 0)
        return x;
    else
        return -x;
}
void write_pixel(int x, int y, short colour)
{
    volatile short *vga_addr = (volatile short *)(pixel_buffer_start + (y << 10) + (x << 1));
    *vga_addr = colour;
}

void clear_screen()
{
    int x, y;
    for (x = 0; x < 320; x++)
    {
        for (y = 0; y < 240; y++)
        {
            write_pixel(x, y, 0);
        }
    }
}

void write_char(int x, int y, char c)
{
    volatile char *character_buffer = (char *)(0xc9000000 + (y << 7) + x);
    *character_buffer = c;
}

void draw_line(int x0, int y0, int x1, int y1, short int colour)
{
    // Bresenham's line algorithm.

    int is_steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (is_steep)
    {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1)
    {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = ABS(y1 - y0);
    int error = -(deltax / 2);

    int y = y0, x = x0;
    int y_step = 0;
    if (y0 < y1)
    {
        y_step = 1;
    }
    else
    {
        y_step = -1;
    }

    for (x = x0; x < x1; x++)
    {
        if (is_steep)
        {
            write_pixel(y, x, colour);
        }
        else
        {
            write_pixel(x, y, colour);
        }
        error = error + deltay;
        if (error > 0)
        {
            y = y + y_step;
            error = error - deltax;
        }
    }
}

void draw_rectangle(int x_com, int y_com, int width, int height, short int colour)
{
    // Calculate the coordinates of the four corners of the rectangle
    int x_top_left = x_com - width / 2;
    int x_top_right = x_com + width / 2;
    int y_top_left = y_com - height / 2;
    int y_top_right = y_top_left;
    int x_bottom_left = x_top_left;
    int y_bottom_left = y_com + height / 2;
    int x_bottom_right = x_top_right;
    int y_bottom_right = y_bottom_left;

    

    // Fill the rectangle with the specified color
    for (int y = y_top_left; y <= y_bottom_left; y++)
    {
        for (int x = x_top_left; x <= x_top_right; x++)
        {
            write_pixel(x, y, colour);
        }
    }
}

void draw_ball(int radius, int center_x, int center_y, short color)
{
    // Draw the outline of the ball
    for (int x = center_x - radius; x <= center_x + radius; x++)
    {
        for (int y = center_y - radius; y <= center_y + radius; y++)
        {
            int distance_from_center = sqrt_new(pow_new(x - center_x, 2) + pow_new(y - center_y, 2));
            if (distance_from_center >= radius - 1 && distance_from_center <= radius + 1)
            {
                write_pixel(x, y, color);
            }
        }
    }

    // Fill the ball with color
    for (int x = center_x - radius + 1; x < center_x + radius; x++)
    {
        for (int y = center_y - radius + 1; y < center_y + radius; y++)
        {
            int distance_from_center = sqrt(pow_new(x - center_x, 2) + pow_new(y - center_y, 2));
            if (distance_from_center <= radius - 1)
            {
                write_pixel(x, y, color);
            }
        }
    }
}

void collision_manager()
{

    int boundary_left = 10;
    int boundary_right = 310;
    int boundary_top = 20;
    int boundary_bottom = 230;

    // detect the right boundary,left boundary,top boundary and bottom boundary
    // and do the elastic collision by changing the circle_vector
    /*
   (0,0)................(320,0)
   .
   .
   .
   .
   .
   .
   .
   .
   (240,0).............(320,240)
    */

    // special collision  detection with rectangle
    int x_top_left = rectangle_x_com - width / 2;
    int x_top_right = rectangle_x_com + width / 2;
    if (center_x <= x_top_right && center_x >= x_top_left && (center_y + radius > rectangle_y_com - 20))
    {
        game_score += 10;
        // collsion occured
        circle_vector_y = -circle_vector_y;
    }
    else
    {
        printf(" Intial: Circle_X %f Circle_Y %f \n", circle_vector_x, circle_vector_y);
        printf(" Intial: Center_y %d center_x %d \n", center_y, center_x);

        if(center_x - radius < boundary_left && center_y - radius < boundary_top){
            circle_vector_x = -circle_vector_x;
            circle_vector_y = -circle_vector_y;
            
        }
        else if(center_x - radius < boundary_left && center_y + radius > boundary_bottom){
            circle_vector_x = -circle_vector_x;
            circle_vector_y = -circle_vector_y;
            
        }
        else if(center_x + radius > boundary_right && center_y + radius > boundary_bottom){
            circle_vector_x = -circle_vector_x;
            circle_vector_y = -circle_vector_y;
            
        }
        else if(center_x + radius > boundary_right && center_y - radius < boundary_top){
            circle_vector_x = -circle_vector_x;
            circle_vector_y = -circle_vector_y;
            
        }
        else if (center_x - radius < boundary_left)
        {
            printf("Left Boundary \n");
            circle_vector_x = -circle_vector_x;
        }
        else if (center_x + radius > boundary_right)
        {
            // printf(" Intial: Circle_X %f Circle_Y %f \n", circle_vector_x, circle_vector_y);
            circle_vector_x = -circle_vector_x;
            printf("Right Boundary \n");
            //    printf("After: Circle_X %f Circle_Y %f \n", circle_vector_x, circle_vector_y);
        }
        else if (center_y - radius < boundary_top)
        {
            circle_vector_y = -circle_vector_y;
            printf("Top Boundary \n");
        }
        else if (center_y + radius > boundary_bottom)
        {
            // game_over = 0;
            circle_vector_y = -circle_vector_y;
            game_ov = 1 ; 
            printf("Bottom Boundary \n");
        }
        
        

        double angle = atan(circle_vector_y / circle_vector_x);
        if (angle == 0 || angle == 90 || angle == 180 || angle == 270 || angle == 360 || angle == 45 || angle == 135 || angle == 225 || angle == 315)
        {
            circle_vector_x = 1;
            circle_vector_y = 2;
        }

        printf("After: Circle_X %f Circle_Y %f \n", circle_vector_x, circle_vector_y);
        printf(" After: Center_y %d center_x %d \n", center_y, center_x);
    }

    int sign_x, sign_y;
    if (circle_vector_x > 0)
        sign_x = 1;
    else
        sign_x = -1;

    if (circle_vector_y > 0)
        sign_y = 1;
    else
        sign_y = -1;

    printf(" SIgn_y %d \n", sign_y);


    double slope = circle_vector_y / circle_vector_x;
    double sin = slope / (sqrt(1 + slope * slope));
    double cos = 1 / (sqrt(1 + slope * slope));

    center_x += 20 * ABS(cos) * sign_x;
    center_y += 20 * ABS(sin) * sign_y;
}

void delay()
{

    volatile int *pixel_ctrl_ptr = (int *)0xff203020;
    int status;
    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);

    while ((status & 0x01) != 0)
    {

        status = *(pixel_ctrl_ptr + 3);
    }
}

void score()
{
    int temp_score = game_score;
    long long n = game_score;
    int count = 0;
    do
    {
        n /= 10;
        ++count;
    } while (n != 0);
    if(count==0) count++;
    char arr[count];
    int j = count - 1;
    // Till N becomes 0
      if(temp_score==0) arr[0]=0+'0';
    while (temp_score != 0)
    {

        // Extract the last digit of N
        int r = temp_score % 10;
        arr[j] = r + '0';
        j--;
        temp_score = temp_score / 10;
    }
      int pos = 0;
      write_char(pos++,10,'S');
      write_char(pos++,10,'C');
      write_char(pos++,10,'O');
      write_char(pos++,10,'R');
      write_char(pos++,10,'E');
      write_char(pos++,10,':');
    int i = 0;
    for (; i < count; i++)
    {
        write_char(pos++, 10, arr[i]);
    }
}

void clear_char ( )
{
    for ( int i = 0 ; i < 80 ; i ++ ){
        for ( int j = 0 ; j < 60 ; j ++ ){
            write_char ( i , j , 0 );
        }
    }
}
int main()
{
    int notplay = 0 ; 
    while ( 1 ){
        clear_screen();
        clear_char();
        game_ov = 0 ; 
        game_score = 0 ; 
        center_x = 160, center_y = 100;
        rectangle_x_com = 175, rectangle_y_com = 235;
        // circle_vector = rand() % 180;
        circle_vector_x = 1;
        circle_vector_y = 3;

        // printf(" Angle: --> %d", circle_vector);
        clear_screen();
        
        draw_rectangle(rectangle_x_com, rectangle_y_com, width, height, 2000);
        draw_ball(radius, center_x, center_y, 0xFC00);
        while (1)
        {

            delay();
            delay();
            delay();
            draw_ball(radius, center_x, center_y, 0);

            collision_manager();
            score();
            if (*player_control == 1)
            {
                (*led_button) = (*player_control);
                delay();
                if(rectangle_x_com>260) rectangle_x_com=260;
                draw_rectangle(rectangle_x_com, rectangle_y_com, 60, 5, 0);
                rectangle_x_com += 30;
                draw_rectangle(rectangle_x_com, rectangle_y_com, 60, 5, 2000);
                // move right
            }
            else if (*player_control == 2)
            {
                (*led_button) = (*player_control);
                delay();

                if(rectangle_x_com<60)rectangle_x_com=60;
                draw_rectangle(rectangle_x_com, rectangle_y_com, 60, 5, 0);

                rectangle_x_com -= 30;
                draw_rectangle(rectangle_x_com, rectangle_y_com, 60, 5, 2000);
                // move left
            }

            draw_ball(radius, center_x, center_y, 0xFC00);
            if ( game_ov == 1 ){
                clear_screen ( );
                char * type = "game over you missed";
                int x = 35 ; 
                while (  * type ){
                    write_char ( x , 30 , * type );
                    type ++ ; 
                    x ++ ; 
                }
                for(int i=0;i<64;i++){
                    delay();
                }
                x = 0 ; 
                while ( x < 80 ){
                    write_char ( x , 30 , 0 );
                    x ++ ; 
                }
                char * tp = "select switch number 2 to retry else leave ";
                x = 20 ;
                while (  * tp ){
                    write_char ( x , 30 , * tp );
                    tp ++ ; 
                    x ++ ; 
                }
                for(int i=0;i<84;i++){
                    delay();
                }
                x = 0 ; 
                while ( x < 80 ){
                    write_char ( x , 30 , 0 );
                    x ++ ; 
                }
                if ( * player_control != 4){
                    notplay = 1 ; 
                }
                break ; 
            }
        }
        if ( notplay == 1 ){
            break ;
        }
    }
    return 0  ;
}
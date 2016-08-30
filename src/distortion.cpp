#include <GL/gl.h>

#include "distortion.h"

distortion::distortion()
{
    GLfloat points[4][4][3] = 
    {
       {{-1.5, -1.5, 0.0}, {-0.5, -1.5, 0.0}, {0.5, -1.5, 0.0}, {1.5, -1.5, 0.0}}, 
       {{-1.5, -0.5, 0.0}, {-0.5, -0.5, 0.0}, {0.5, -0.5, 0.0}, {1.5, -0.5, 0.0}}, 
       {{-1.5,  0.5, 0.0}, {-0.5,  0.5, 0.0}, {0.5,  0.5, 0.0}, {1.5,  0.5, 0.0}}, 
       {{-1.5,  1.5, 0.0}, {-0.5,  1.5, 0.0}, {0.5,  1.5, 0.0}, {1.5,  1.5, 0.0}}
    }; 
    
    for ( int i = 0; i < (4*4*3); i++)
    {
        *((GLfloat*)ctrlpoints + i) = *((GLfloat*)points + i);
    }
    
    barrel_dis = 0.0;
    eye_dis    = 0.0;
    fill_barrel_distortion();
}

void distortion::init(GLfloat barrel, GLfloat eye)
{
    barrel_dis = barrel;
    eye_dis    = eye;
    fill_barrel_distortion();
}


distortion::~distortion()
{
} 
   
void distortion::fill_barrel_distortion()
{
    int   index_x = 0;
    int   index_y = 0;
    double value_x = 0;
    double value_y = 0; 
    // Ax+B=C
    double value_A = barrel_dis;
    double value_B = eye_dis;
    
    value_y = -1.5;
    for ( index_y = 0 ; index_y < 4 ; index_y++ )
    {
       value_x = -1.5;
 
       for ( index_x = 0 ; index_x < 4 ; index_x++ )
       {
           ctrlpoints[index_y][index_x][0] = value_x;
           ctrlpoints[index_y][index_x][1] = value_y;
           value_x = value_x + 1.0;
       }
       value_y = value_y + 1.0;     
    }
     
         
    for ( index_y = 0 ; index_y < 4 ; index_y++ )
    {
        value_y = value_A *  ctrlpoints[index_y][1][1] + value_B;
        ctrlpoints[index_y][1][1] = ctrlpoints[index_y][1][1] + value_y;

        value_y = value_A *  ctrlpoints[index_y][2][1] + value_B;
        ctrlpoints[index_y][2][1] = ctrlpoints[index_y][2][1] + value_y;
    }
    
    value_B = 0.00;
    for ( index_x = 0 ; index_x < 4 ; index_x++ )
    {
        value_x = value_A *  ctrlpoints[1][index_x][0] + value_B;
        ctrlpoints[1][index_x][0] = ctrlpoints[1][index_x][0] + value_x;
  
        value_x = value_A * ctrlpoints[2][index_x][0] + value_B;
        ctrlpoints[2][index_x][0] = ctrlpoints[2][index_x][0] + value_x;
    }    
}
 
    
void distortion::addBarrelDisto(GLfloat dis )
{
    barrel_dis = barrel_dis + dis;
    fill_barrel_distortion();
}

void distortion::addEyeDis(GLfloat dis )
{
    eye_dis = eye_dis + dis;
    fill_barrel_distortion();
}
   
   
GLfloat distortion::getBarrelDisto()
{
    return barrel_dis;
}

GLfloat distortion::getEyeDist()
{
    return eye_dis;
}   
GLfloat* distortion::getDistortion()
{
   return (GLfloat*)ctrlpoints;
}
 
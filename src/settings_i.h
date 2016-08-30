#include <GL/gl.h>

class settings;

class settings_i 
{   
   public: enum VIDEO_FORMAT {PAL,NTSC};
   
   private:
      settings *pSettings;
      
   public:
      settings_i();

      VIDEO_FORMAT getVideoFormat();
      GLfloat      getBarrelDisto();
      GLfloat      getEyeDisto();
      GLfloat      getImageZoom();
      GLfloat      getImagePosition();
      int          getDeinterlaceThres();
      
      bool         getDeinterlace();
      void save();

      void SetImagePosition(float position);
      void SetImageZoom(float zoom);
      void SetBarrelDisto(float disto);
      void SetEyeDisto(float disto);
      void setDeinterlaceThres(int thres);
};

static settings_i Settings;

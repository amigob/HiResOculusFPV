
class license;

class license_i 
{   
   private:
      license *pLicense;
      
   public:
      license_i();
      
      bool         getDeinterlace();
      bool         getVideo3D();
      bool         getVideoRecording();
      std::string  getName();
      void         createSignature();      
};

static license_i License;

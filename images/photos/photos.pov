                  
//-------------------------------------------------------------------------                  
// PHOTOS.POV
//    Renders a parameterized set of orthographic images for a scene 
//    containing texture-mapped planes. 
//
//-------------------------------------------------------------------------

// Define included files.    
#include "textures.inc"
#include "colors.inc"

// Enable/disable gamma correction (uncomment following line to enable).
// Note: Gamma-compress output image, using the value of the "Display_Gamma" INI settings.
//       The display gamma can be measured here: http://perso.telecom-paristech.fr/~brettel/TESTS/Gamma/Gamma.html
global_settings { assumed_gamma 1 }
    
// Define display and lenticular sheet properties.
// Note: Screen resolution should match rendering resolution.
#declare Display_Width_CM  = 43.344; // display width (cm)
#declare Display_Height_CM = 27.090; // display height (cm)          
#declare Lens_Width_CM     = 0.127;  // lens width (cm) (e.g., 0.1265 or 0.254)
#declare Lens_H_FoV        = 48.0;   // horizontal field of view of each lens (degrees) (e.g., 29 or 48)    
#declare H_Angles          = 10;     // number of horizontal views 

// Define additional options.
// Note: Set near clipping plane to a negative value to allow objects in front of the display plane.
#declare Near_Clip_Plane = -100;  // position of orthographic camera (along z-axis)
#declare Current_H_Angle = clock; // index for current horizontal view (on [1,Display_H_Angles])

// Manually define view (if clock is not enabled).
#if (clock_on = 0)
   #declare Current_H_Angle = (H_Angles+1)/2;
#end
               
// Evaluate derived display and lenticular sheet properties.
#declare Lens_Focal_Length_CM = Lens_Width_CM/(2*tan((pi/180)*Lens_H_FoV/2));
#declare H_Shift_CM           = -(Lens_Width_CM/H_Angles)*(Current_H_Angle-(H_Angles+1)/2);  

// Define the background color.
background { rgb <0, 0, 0> }                          
                         
// Define the orthographic camera.
// Note: Objects cannot appear closer than the camera, therefore the near 
//       clipping plane must be set using the "Near_Clip_Plane" declaration.
camera {       
   orthographic
   location <0, 0, 0>
   direction <H_Shift_CM, 0, Lens_Focal_Length_CM>
   right Display_Width_CM*x
   up Display_Height_CM*y
   translate <(Near_Clip_Plane*H_Shift_CM)/Lens_Focal_Length_CM, 0, Near_Clip_Plane>
}
                      
// Define texture-mapped planes.
mesh {
   triangle {
      <-0.5,-0.5,0>, <0.5,-0.5,0>, <0.5,0.5,0>
      uv_vectors <0,0>, <1,0>, <1,1>
   }
   triangle {
      <-0.5,-0.5,0>, <0.5,0.5,0>, <-0.5,0.5,0>
      uv_vectors <0,0>, <1,1>, <0,1>
   }
   texture {
      uv_mapping pigment {
         image_map {
            jpeg "merlion.jpg"
         }
      }
      finish { ambient 1.0 }
   }
   scale <((1/3)*Display_Width_CM), (600/800)*((1/3)*Display_Width_CM)>
   translate <-(1/4)*Display_Width_CM, 0, 0>
}
mesh {
   triangle {
      <-0.5,-0.5,0>, <0.5,-0.5,0>, <0.5,0.5,0>
      uv_vectors <0,0>, <1,0>, <1,1>
   }
   triangle {
      <-0.5,-0.5,0>, <0.5,0.5,0>, <-0.5,0.5,0>
      uv_vectors <0,0>, <1,1>, <0,1>
   }
   texture {
      uv_mapping pigment {
         image_map {
            jpeg "academy.jpg"
         }
      }
      finish { ambient 1.0 }
   }
   scale <((1/3)*Display_Width_CM), (600/800)*((1/3)*Display_Width_CM)>
   translate <(1/4)*Display_Width_CM, 0, -3.0>
}
mesh {
   triangle {
      <-0.5,-0.5,0>, <0.5,-0.5,0>, <0.5,0.5,0>
      uv_vectors <0,0>, <1,0>, <1,1>
   }
   triangle {
      <-0.5,-0.5,0>, <0.5,0.5,0>, <-0.5,0.5,0>
      uv_vectors <0,0>, <1,1>, <0,1>
   }
   texture {
      uv_mapping pigment {
         image_map {
            jpeg "arch.jpg"
         }
      }
      finish { ambient 1.0 }
   }
   scale <(1.05*Display_Width_CM), (600/800)*(1.05*Display_Width_CM)>
   translate <0, 0, 6.0>
}
   

// Define the light sources.
global_settings { ambient_light rgb <1, 1, 1> }

// Print debug statements.
#debug concat("Rendering view ", str(Current_H_Angle,1,0),"...\n")
                                                                          
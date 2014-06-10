
//-------------------------------------------------------------------------
// TEAPOT.POV
//    Renders a parameterized set of orthographic images for a scene 
//    containing the Utah (Newell) teapot.
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
#declare Lens_H_FoV        = 48.0/2; // horizontal field of view of each lens (degrees) (e.g., 29 or 48)    
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
                      
// Define the Utah (Newell) teapot.
// See rendered textures here: http://texlib.povray.org/metal-browsing_1.html
#declare Teapot_Texture = texture {
   pigment { rgb <1, 0 ,0> }   
   finish {
      ambient    0.2
      diffuse    0.3
      reflection 0.0
      specular   0.5
      roughness  0.05
   }            
};
#declare Teapot_Scale       = 5.0;       
//#declare Teapot_Orientation = <-120, 10, -5>;
//#declare Teapot_Orientation = <50, 135, 145>;
#declare Teapot_Orientation = <60, 145, 150>;
#declare Teapot_Translation = <0, -5, 5>;
#include "teapot.inc"
    
// Define walls.
// Note: Set clipping plane in display plane (otherwise objects cannot be in front of display).
union{
   plane {  x, -1.2*(Display_Width_CM/2) }  // left wall
   plane { -x, -1.2*(Display_Width_CM/2) }  // right wall
   plane { -z, -20 }                        // back wall
   plane {  y, -1.2*(Display_Height_CM/2) } // floor
   pigment {
      checker color Black color White
      scale 7.5
   }
   finish {
      ambient    0.2
      diffuse    0.6
      reflection 0.0
      specular   0.25
      roughness  0.1
   }
   clipped_by{ plane{ -z, 0} }
}

// Define the light sources.
global_settings { ambient_light rgb <1, 1, 1> }
light_source {
   <30, 30, -200>
   color rgb <1, 1, 1>
}

// Print debug statements.
#debug concat("Rendering view ", str(Current_H_Angle,1,0),"...\n")
                                                                          
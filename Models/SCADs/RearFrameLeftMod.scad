module rearFrameL(){
//import("/mnt/Black/Impresion3D/Modelos/Robotica/AutoRC/files/Rear_Frame_L.stl"); 
   import("G:\\Impresion3D\\Modelos\\Robotica\\AutoRC\\files\\Rear_Frame_L.stl"); 
}

module rearFrameR(){
//import("/mnt/Black/Impresion3D/Modelos/Robotica/AutoRC/files/Rear_Frame_R.stl"); 
   import("G:\\Impresion3D\\Modelos\\Robotica\\AutoRC\\files\\Rear_Frame_R.stl"); 
}


//difference(){
translate([100,0,20])
cube([10,10,10]);

rotate([180,0,180]) translate([-175,0,-37])     rearFrameR();  
    
//}
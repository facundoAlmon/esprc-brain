module gear(){
#import("/mnt/Black/Impresion3D/Modelos/Robotica/AutoRC/files/Gear_1.stl"); 
   import("G:\\Impresion3D\\Modelos\\Robotica\\AutoRC\\files\\Gear_1.stl"); 
    cylinder(r=3,h=11.5,$fn=100);
}

module eje_pololu(){
difference(){
translate ([0,0,0]) cylinder(r=3.2/2,h=20,$fn=100);

translate ([1.2,-1.50,-1]) cube([40,40,22]);
    
}
}

module gearMod() {
difference(){
gear();
eje_pololu();
}

}
gearMod();
//eje_pololu();